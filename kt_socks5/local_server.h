#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include "control_session.h"
#include "socket_queue.h"

using namespace std;
using boost::asio::ip::tcp;

class local_server
{
public:
	local_server(boost::asio::io_context& io, tcp::endpoint& ep,
		std::shared_ptr<control_session>& cs,
		std::shared_ptr<socket_queue>& q);
	~local_server();

private:
	void Accept();
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::io_context& io_context_;
	std::shared_ptr<control_session> cs_;
	std::shared_ptr<socket_queue> q_;
};

