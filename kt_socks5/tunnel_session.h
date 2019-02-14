#pragma once

#include <iostream>
#include <memory>
#include <boost/asio.hpp>


using namespace std;
using boost::asio::ip::tcp;

class tunnel_session : public std::enable_shared_from_this<tunnel_session>
{
public:
	tunnel_session(tcp::socket l,tcp::socket t,int id);
	~tunnel_session();

	void start();

private:

	void local_read();
	void tunnel_read();
	void local_write(std::size_t length);
	void tunnel_write(std::size_t length);
	void close(const boost::system::error_code& ec);

	tcp::socket s_local;
	tcp::socket s_tunnel;

	std::array<char, 8192> local_buf_;
	std::array<char, 8192> tunnel_buf_;

	int id_;
};

