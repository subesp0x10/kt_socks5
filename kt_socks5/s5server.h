#pragma once
#include <memory>

#include <boost/asio.hpp>

#include "s5session.h"

using namespace std;
using boost::asio::ip::tcp;


class s5server {
public:
	s5server(boost::asio::io_context& io, tcp::endpoint& ep);
	~s5server();

private:
	void Accept();
	int count_ = 0;
private:
	boost::asio::ip::tcp::acceptor acceptor_;
};