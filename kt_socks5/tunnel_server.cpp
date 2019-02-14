#include "tunnel_server.h"
#include "tunnel_session.h"


tunnel_server::tunnel_server(boost::asio::io_context & io, tcp::endpoint& ep,
	std::shared_ptr<control_session>& cs,
	std::shared_ptr<socket_queue>& q)
	:acceptor_(io, ep),
	io_context_(io),
	cs_(cs),
	q_(q)
{
	std::cout << "[tunnel] listening port: " << ep.port() << std::endl;
	Accept();
}

tunnel_server::~tunnel_server()
{
}

void tunnel_server::Accept()
{
	auto accept_handler = [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer) {
		if (!ec) {

			if (cs_->ready())
			{
				//std::cout << "[tunnel] accept socket" << endl;
				tcp::socket local(io_context_);
				if (q_->dequeue(local))
				{
					std::shared_ptr<tunnel_session> trans = std::make_shared<tunnel_session>(std::move(local), std::move(peer), count_);
					trans->start();
					count_++;
				}
				
			}
			else
			{
				cs_->start_as_server(std::move(peer));
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
