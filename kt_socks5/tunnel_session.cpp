#include "tunnel_session.h"



tunnel_session::tunnel_session(tcp::socket l, tcp::socket t, int id)
	: s_local(std::move(l)),
	s_tunnel(std::move(t)),
	id_(id)
{
}

tunnel_session::~tunnel_session()
{
}

void tunnel_session::start()
{
	std::cout << "[trans][" << id_ << "]" << " start" << std::endl;
	local_read();
	tunnel_read();
}

void tunnel_session::local_read()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			tunnel_write(length);
		}
		else {
			
			close(ec);
		}
	};

	s_local.async_read_some(
		boost::asio::buffer(local_buf_.data(), local_buf_.size()), handler);
}

void tunnel_session::tunnel_read()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			local_write(length);
		}
		else {

			close(ec);
		}
	};

	s_tunnel.async_read_some(
		boost::asio::buffer(tunnel_buf_.data(), tunnel_buf_.size()), handler);
}

void tunnel_session::local_write(std::size_t length)
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			tunnel_read();
		}
		else {
			
			close(ec);
		}
	};

	boost::asio::async_write(s_local,
		boost::asio::buffer(tunnel_buf_.data(), length), boost::asio::transfer_all(), handler);
}

void tunnel_session::tunnel_write(std::size_t length)
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			local_read();
		}
		else {

			close(ec);
		}
	};

	boost::asio::async_write(s_tunnel,
		boost::asio::buffer(local_buf_.data(), length), boost::asio::transfer_all(), handler);
}

void tunnel_session::close(const boost::system::error_code & ec)
{
	std::cout << "[trans][" << id_ << "]" << " exit" << std::endl;
	if (s_local.is_open())
	{
		s_local.close();
	}
	if (s_tunnel.is_open())
	{
		s_tunnel.close();
	}
}
