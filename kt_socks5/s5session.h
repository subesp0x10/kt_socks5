#pragma once
#include <iostream>
#include <memory>
#include <string>

#include <boost/asio.hpp>


using namespace std;
using boost::asio::ip::tcp;

 #define MAX_METHODS			255
 
 #define SOCK5_MAX_DMN_NAME_LEN		0xFF


enum class s5_auth : unsigned char {
	no_auth = 0x00,
	gssapi = 0x01,
	username_password = 0x02,
	no_acceptable_methods = 0xFF,
};

enum class s5_command : unsigned char {
	connect = 0x01,
	bind = 0x02,
	udp_associate = 0x03,
};


enum class s5_addr_type : unsigned char {
	ipv4 = 0x01,
	domain_name = 0x03,
	ipv6 = 0x04,
};

enum class s5_replies : unsigned char {
	succeeded = 0x00,
	general_socks_server_failure = 0x01,
	connection_not_allowed_by_ruleset = 0x02,
	network_unreachable = 0x03,
	host_unreachable = 0x04,
	connection_refused = 0x05,
	ttl_expired = 0x06,
	command_not_supported = 0x07,
	address_type_not_supported = 0x08,
};


struct sock_ver_id_msg {
	uint8_t ver;
	uint8_t nmethods;
	uint8_t methods[MAX_METHODS];
};

struct sock_method_sel_msg {
	uint8_t ver;
	uint8_t method;
};

struct sock5_dmn_name {
	uint8_t len;
	char name[SOCK5_MAX_DMN_NAME_LEN];
};

struct sock5_msg {
	uint8_t ver;
	union {
		uint8_t cmd;
		uint8_t rep;
	};
	uint8_t rsv;
	uint8_t atyp;
	union {
		uint8_t ipv4_addr[4];
		uint8_t ipv6_addr[16];
		struct sock5_dmn_name dmn_name;
	};
	uint16_t dst_port;
};


class s5session : public std::enable_shared_from_this<s5session>
{
public:
	s5session(boost::asio::io_context & io_context, tcp::socket socket);
	s5session(boost::asio::io_context & io_context);
	~s5session();
	void do_proxy(int id);
	void do_proxy(uint16_t port, const char* username, const char* password, int id);
	void do_reverse_proxy(char *ip, uint16_t port, int n);
	void do_reverse_proxy(tcp::resolver::iterator&  ep_iterator, int n);


	tcp::socket& AcceptorSocket() { return downstream_socket_; }

private:

	void s5_auth();
	void s5_read_request();
	void s5_process_request();
	void s5_reslove();
	void s5_connect(tcp::resolver::iterator& it);
	void s5_reply(s5_replies r);
	void s5_upstream_read();
	void s5_downstream_read();
	void s5_upstream_write(std::size_t length);
	void s5_downstream_write(std::size_t length);

	s5_addr_type s5_get_address_type() const;
	std::size_t s5_get_domain_size() const;
	std::size_t s5_get_request_size() const;

	void close(const boost::system::error_code & ec);

	bool auth_required = false;
	string auth_username;
	string auth_password;

	boost::asio::io_context& io_context_;

	std::size_t downstream_bytes_read_ = 0;
	tcp::socket downstream_socket_;
	tcp::socket upstream_socket_;

	std::array<char, 8192> upstream_buf_;
	std::array<char, 8192> downstream_buf_;

	std::array<char, 8> reverse_header;

	int id_ = 0;
};