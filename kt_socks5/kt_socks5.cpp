// kt_socks5.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>

using namespace std;
namespace po = boost::program_options;



extern void reverse_socks_server(uint16_t port_tunnel, uint16_t port_socks5);
extern void reverse_socks_client(string ip, uint16_t port);
extern void forward_socks(uint16_t port);

bool valid_port(uint16_t p)
{
	return (0 < p && p <= 65535);
}

int main(int argc, char *argv[])
{
	po::options_description desc("kt_socks");
	desc.add_options()
		("help,h", "produce help message")
		("port,p", po::value<uint16_t>(), "port of forward socks server")
		("client,c", po::value<string>(), "endpoint of tunnel, e.g. 127.0.0.1:1080")
		("reverse-local,l", po::value<uint16_t>(), "local port of reverse server")
		("reverse-tunnel,t", po::value<uint16_t>(), "tunnel port of reverse server")
		;

	po::variables_map vm;

	try
	{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 0;
	}

	if (vm.count("help"))
	{
		std::cout << desc << std::endl;
	}
	else if (vm.count("port"))
	{
		uint16_t p = vm["port"].as<uint16_t>();
		if (valid_port(p))
		{
			forward_socks(p);
		}
		
	}
	else if (vm.count("client"))
	{
		string dst = vm["client"].as<string>();

		size_t colonPos = dst.find(':');
		if (colonPos != std::string::npos)
		{
			std::string s_host = dst.substr(0, colonPos);
			std::string s_port = dst.substr(colonPos + 1);

			uint16_t port = boost::lexical_cast<uint16_t>(s_port);
			if (valid_port(port))
			{
				reverse_socks_client(s_host, port);
			}
		}
	}
	else if (vm.count("reverse-local") && vm.count("reverse-tunnel"))
	{
		uint16_t p_local = vm["reverse-local"].as<uint16_t>();
		uint16_t p_tunnel = vm["reverse-tunnel"].as<uint16_t>();
		if (valid_port(p_local) && valid_port(p_tunnel))
		{
			reverse_socks_server(p_tunnel, p_local);
		}
		
	}

    return 0;
}

