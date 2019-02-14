#pragma once
#include <memory>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;


class control_session : public std::enable_shared_from_this<control_session>
{
public:
	control_session(boost::asio::io_context& io_context);

	void start_as_client(string ip, uint16_t port);
	void start_as_server(tcp::socket s);
	bool ready() { return established; }
	void get_tunnel_session();
	void process_command();

private:
	
	void do_connect();
	void client_side_handshake();
	void server_side_handshake();

	void client_handshake_ok();
	void server_handshake_ok();

	void close(const boost::system::error_code & ec);

	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::socket sock_;
	tcp::resolver::iterator endpoint_iterator;

	int handshake_num;
	int count = 0;
	std::array<char, 256> recv_buff;
	
	bool established = false;
};