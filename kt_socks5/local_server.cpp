#include "local_server.h"



local_server::local_server(boost::asio::io_context & io, tcp::endpoint& ep,
	std::shared_ptr<control_session>& cs,
	std::shared_ptr<socket_queue>& q)
	:acceptor_(io, ep),
	io_context_(io),
	cs_(cs),
	q_(q)
{
	std::cout << "[local] listening port: " << ep.port() << std::endl;
	Accept();
}


local_server::~local_server()
{
}

void local_server::Accept()
{
	auto accept_handler = [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer)
	{
		if (!ec)
		{
			if (cs_->ready() == false)
			{
				std::cout << "[local] Error: tunnel has not established" << std::endl;
			}
			else
			{
				//std::cout << "[local] Accept a socket" << endl;
				q_->enqueue(std::move(peer));
				cs_->get_tunnel_session();
			}
		}
		else
		{
			std::cout << ec.message() << std::endl;
		}

		Accept();
	};

	acceptor_.async_accept(accept_handler);
}
