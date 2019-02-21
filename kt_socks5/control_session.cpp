#include <memory>
#include <iostream>
#include <random>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "control_session.h"
#include "s5session.h"


control_session::control_session(boost::asio::io_context & io_context) 
	: io_context_(io_context),
		sock_(io_context)
{
	
}


void control_session::start_as_client(string ip, uint16_t port)
{
	auto resolver = std::make_shared<tcp::resolver>(io_context_);
	auto self(shared_from_this());
	auto handler = [this, self, resolver](const boost::system::error_code& ec, tcp::resolver::iterator it) {
		if (!ec)
		{
			endpoint_iterator = it;
			do_connect();
		}
		else
		{
			close(ec);
		}
	};

	
	tcp::resolver::query q(ip, boost::lexical_cast<string>(port));
	resolver->async_resolve(q, handler);
	
}

void control_session::start_as_server(tcp::socket s)
{
	sock_ = std::move(s);

	server_side_handshake();
}

void control_session::get_tunnel_session()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {
			
		}
		else {

			close(ec);
		}
	};

	//std::cout << "[get_tunnel_session] handshake=" << handshake_num << endl;

	boost::asio::async_write(sock_,
		boost::asio::buffer(&handshake_num,sizeof(int)), boost::asio::transfer_all(), handler);
}

void control_session::process_command()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {

		if (!ec)
		{

			//create s5sessions
			std::shared_ptr<s5session> s5 = std::make_shared<s5session>(io_context_);
			s5->do_reverse_proxy(endpoint_iterator, count);
			count++;

			process_command();
		}
		else
		{
			
			close(ec);
		}
	};

	boost::asio::async_read(sock_, boost::asio::buffer(recv_buff, recv_buff.size()), boost::asio::transfer_exactly(sizeof(int)), handler);
}

void control_session::do_connect()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator it) {
		if (!ec)
		{
			client_side_handshake();
		}
		else
		{
			close(ec);
		}

	};

	boost::asio::async_connect(sock_, endpoint_iterator, handler);

	//https://stackoverflow.com/questions/42132104/how-to-call-boost-async-connect-as-member-function-using-lamba-as-connect-handle
}

void control_session::client_side_handshake()
{
	boost::system::error_code ec;

	//write a random integer
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 999);

	handshake_num = dist(mt);
	int temp;

	do 
	{
		boost::asio::write(sock_, boost::asio::buffer(&handshake_num, sizeof(int)), boost::asio::transfer_all(), ec);
		if (ec)
		{
			break;
		}
		boost::asio::read(sock_, boost::asio::buffer(&temp,sizeof(int)), boost::asio::transfer_exactly(sizeof(int)), ec);
		if (ec)
		{
			break;
		}

		handshake_num++;
		if (temp != handshake_num)
		{
			ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
			close(ec);
		}

		handshake_num++;
		boost::asio::write(sock_, boost::asio::buffer(&handshake_num, sizeof(int)), boost::asio::transfer_all(), ec);
		if (ec)
		{
			break;
		}

		client_handshake_ok();
		return;

	} while (0);

	close(ec);
}

void control_session::server_side_handshake()
{
	boost::system::error_code ec;
	int temp;

	do 
	{
		boost::asio::read(sock_, boost::asio::buffer(&temp, sizeof(int)), boost::asio::transfer_exactly(sizeof(int)), ec);
		if (ec)
		{
			break;
		}

		temp++;
		handshake_num = temp;

		boost::asio::write(sock_, boost::asio::buffer(&handshake_num, sizeof(int)), boost::asio::transfer_all(), ec);
		if (ec)
		{
			break;
		}

		boost::asio::read(sock_, boost::asio::buffer(&temp, sizeof(int)), boost::asio::transfer_exactly(sizeof(int)), ec);
		if (ec)
		{
			break;
		}

		handshake_num++;
		if (temp != handshake_num )
		{
			ec = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
			break;
		}

		server_handshake_ok();
		return;

	} while (0);

	close(ec);
}


void control_session::client_handshake_ok()
{
	std::cout << "[client] control session established, handshake=" << handshake_num << std::endl;
	process_command();

	
}

void control_session::server_handshake_ok()
{
	std::cout << "[server] control session established, handshake=" << handshake_num << std::endl;
	established = true;
}

//nothing to read, just check wether this session is disconnected
void control_session::server_read()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {

		if (!ec)
		{
			server_read();
		}
		else
		{
			close(ec);
		}
	};

	boost::asio::async_read(sock_, boost::asio::buffer(recv_buff, recv_buff.size()), boost::asio::transfer_exactly(sizeof(int)), handler);
}

void control_session::close(const boost::system::error_code& ec)
{
	std::cout << "[control session]  disconnected : " << ec.message() << std::endl;

	established = false;
	if (sock_.is_open())
	{
		sock_.close();
	}

	
}
