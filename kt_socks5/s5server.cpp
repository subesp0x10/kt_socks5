#include "s5server.h"

s5server::s5server(boost::asio::io_context& io, tcp::endpoint& ep)
	: acceptor_(io, ep)
{
	//std::cout << "s5server constructor" << endl;
	Accept();
}

s5server::~s5server()
{
	//std::cout << "s5server destructor" << endl;
}

void s5server::Accept()
{
	//std::shared_ptr<s5session> s5 = std::make_shared<s5session>(acceptor_.get_io_context());

	auto accept_handler = [this](const boost::system::error_code& ec , boost::asio::ip::tcp::socket peer) {
		if (!ec) {

			//std::cout << "accept " << peer.remote_endpoint().address().to_string() << peer.remote_endpoint().port() << endl;

			std::shared_ptr<s5session> s5 = std::make_shared<s5session>(peer.get_io_context(), std::move(peer));
			s5->do_proxy(count_);
			count_++;
		}

		//std::cout << ec.message() << endl;
		Accept();
	};

	acceptor_.async_accept( accept_handler);
}
