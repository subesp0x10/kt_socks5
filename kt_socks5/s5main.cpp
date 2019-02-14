#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "s5server.h"
#include "control_session.h"
#include "tunnel_server.h"
#include "socket_queue.h"
#include "local_server.h"

using namespace std;


void reverse_socks_server(uint16_t port_tunnel, uint16_t port_socks5)
{
	try
	{
		boost::asio::io_context io_context;

		auto sq = std::make_shared<socket_queue>();
		std::shared_ptr<control_session> cs = std::make_shared<control_session>(io_context);

		boost::asio::ip::tcp::endpoint ep_tunnel(boost::asio::ip::tcp::v4(), port_tunnel);
		boost::asio::ip::tcp::endpoint ep_local(boost::asio::ip::tcp::v4(), port_socks5);

		tunnel_server t(io_context, ep_tunnel, cs , sq);
		local_server l(io_context, ep_local, cs, sq);

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

}

void reverse_socks_client(string ip, uint16_t port)
{
	std::cout << "reverse client, target " << ip << ":" << port << std::endl;


	boost::asio::io_context io_context;

	//boost::asio::ip::tcp::resolver resolver(io_context);
	//auto endpoints = resolver.resolve(ip, boost::lexical_cast<string>(port));

	try
	{
		auto cs = std::make_shared<control_session>(io_context);
		cs->start_as_client(ip, port);

		io_context.run();
	}

	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void forward_socks(uint16_t port)
{
	try
	{
		std::cout << "forward socks, port:" << port << std::endl;
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);
		s5server s(io_context, ep);

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return;

}
