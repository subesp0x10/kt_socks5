#pragma once
#include <iostream>
#include <boost/asio.hpp>

#include "socket_queue.h"
#include "control_session.h"

using namespace std;
using boost::asio::ip::tcp;


class tunnel_server
{
public:
	tunnel_server(boost::asio::io_context& io, tcp::endpoint& ep,
		std::shared_ptr<control_session>& cs,
		std::shared_ptr<socket_queue>& q);
	~tunnel_server();

private:
	void Accept();
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_context& io_context_;
	std::shared_ptr<control_session> cs_;
	std::shared_ptr<socket_queue> q_;

	int count_ = 0;
};

