#include "s5session.h"

#include <boost/lexical_cast.hpp>

s5session::s5session(boost::asio::io_context & io_context , tcp::socket socket)
	: io_context_(io_context),
	downstream_socket_(std::move(socket)),
	upstream_socket_(io_context)
{
	
}

s5session::s5session(boost::asio::io_context & io_context)
	: io_context_(io_context),
	downstream_socket_(io_context),
	upstream_socket_(io_context)
{
	
}

s5session::~s5session()
{
	
}

void s5session::do_proxy(int id)
{
	id_ = id;
	s5_auth();
}

void s5session::do_proxy(uint16_t port, const char * username, const char * password, int id)
{
	auth_username.assign(username);
	auth_password.assign(password);
	auth_required = true;

	id_ = id;

	s5_auth();
}

void s5session::do_reverse_proxy(char * ip, uint16_t port, int n)
{
	reverse_header = { 'a','s','d','f','q','w','e','r' };

	auto resolver = std::make_shared<tcp::resolver>(io_context_);
	auto self(shared_from_this());
	auto handler = [this, self, resolver, n](const boost::system::error_code& ec, tcp::resolver::iterator it) {
		if (!ec)
		{
			do_reverse_proxy(it, n);
		}
		else
		{
			close(ec);
		}
	};

	tcp::resolver::query q(ip, boost::lexical_cast<string>(port));

	resolver->async_resolve(q, handler);
}

void s5session::do_reverse_proxy(tcp::resolver::iterator&  ep_iterator, int n)
{
	id_ = n;
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator it) {
		if (!ec)
		{
			s5_auth();
		}
		else
		{
			std::cout << "[s5session] connect : " << ec.message() << std::endl;
			close(ec);
		}
	};

	boost::asio::async_connect(downstream_socket_, ep_iterator, handler);
	
}

void s5session::s5_auth()
{
	boost::array<unsigned char, 256> auth_buf;
	boost::array<unsigned char, 2> responce;
	boost::system::error_code ec;

	do 
	{
		/*
		+----+----------+----------+
		|VER | NMETHODS | METHODS  |
		+----+----------+----------+
		| 1  |    1     | 1 to 255 |
		+----+----------+----------+
		*/

		int i = 0;
		std::size_t n = boost::asio::read(downstream_socket_, boost::asio::buffer(auth_buf), boost::asio::transfer_exactly(2), ec);
		if (ec)
		{
			std::cout << ec.message() << std::endl;
		}

		if (auth_buf[0] != 0x05)
		{
			break;
		}

		int num_method = auth_buf[1];
		n = boost::asio::read(downstream_socket_, boost::asio::buffer(auth_buf), boost::asio::transfer_exactly(num_method), ec);


		for (i = 0; i < num_method; i++)
		{
			if (auth_buf[i] == static_cast<unsigned char>(s5_auth::username_password))
			{
				break;
			}
		}

		/*
		+----+-----------------+
		|VER | METHOD CHOSSED  |
		+----+-----------------+
		| 1  |    1 to 255     |
		+----+-----------------+


		o  X'00' NO AUTHENTICATION REQUIRED
		o  X'01' GSSAPI
		o  X'02' USERNAME/PASSWORD
		o  X'03' to X'7F' IANA ASSIGNED
		o  X'80' to X'FE' RESERVED FOR PRIVATE METHODS
		o  X'FF' NO ACCEPTABLE METHODS

		*/
		responce[0] = 0x05;
		if (auth_required)
		{
			if (i == num_method)
			{
				responce[1] = static_cast<unsigned char>(s5_auth::no_acceptable_methods);
			}
			else
			{
				responce[1] = static_cast<unsigned char>(s5_auth::username_password);
			}
		}
		else
		{
			responce[1] = static_cast<unsigned char>(s5_auth::no_auth);
		}

		boost::asio::write(downstream_socket_, boost::asio::buffer(responce), boost::asio::transfer_all(), ec);


		if (responce[1] == static_cast<unsigned char>(s5_auth::no_acceptable_methods)) break;

		if (responce[1] == static_cast<unsigned char>(s5_auth::no_auth)) {
			//NO AUTHENTICATION REQUIRED
			s5_read_request();
		}
		else{

			/*
			+----+------+----------+------+----------+
			|VER | ULEN |  UNAME   | PLEN |  PASSWD  |
			+----+------+----------+------+----------+
			| 1  |  1   | 1 to 255 |  1   | 1 to 255 |
			+----+------+----------+------+----------+
			*/
			n = boost::asio::read(downstream_socket_, boost::asio::buffer(auth_buf), boost::asio::transfer_exactly(2), ec);
			int len = auth_buf[1];
			n = boost::asio::read(downstream_socket_, boost::asio::buffer(auth_buf), boost::asio::transfer_exactly(len), ec);
			auth_buf[len] = 0;
			std::string username(auth_buf.begin(), auth_buf.end());
			n = boost::asio::read(downstream_socket_, boost::asio::buffer(auth_buf), boost::asio::transfer_exactly(1), ec);
			len = auth_buf[0];
			n = boost::asio::read(downstream_socket_, boost::asio::buffer(auth_buf), boost::asio::transfer_exactly(len), ec);
			auth_buf[len] = 0;
			std::string password(auth_buf.begin(), auth_buf.end());

			/*
			+----+--------+
			|VER | STATUS |
			+----+--------+
			| 1  |   1    |
			+----+--------+
			*/
			responce[0] = 0x05;
			if (boost::equal(auth_username, username) && boost::equal(auth_password, password))
			{
				responce[1] = static_cast<unsigned char>(s5_replies::succeeded);
			}
			else
			{
				responce[1] = static_cast<unsigned char>(s5_replies::general_socks_server_failure);
			}
			boost::asio::write(downstream_socket_, boost::asio::buffer(responce), boost::asio::transfer_all(), ec);
			if (responce[1] == static_cast<unsigned char>(s5_replies::succeeded)) {
				s5_read_request();
			}
			else {
				break;
			}
		}


		return;

	} while (0);


	close(ec);

}

void s5session::s5_read_request()
{
	/*
	+----+-----+-------+------+----------+----------+
	|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.gListenSocket |
	+----+-----+-------+------+----------+----------+
	| 1  |  1  | X'00' |  1   | Variable |    2     |
	+----+-----+-------+------+----------+----------+

	o  VER    protocol version: X'05'
	o  CMD
	o  CONNECT X'01'
	o  BIND X'02'
	o  UDP ASSOCIATE X'03'
	o  RSV    RESERVED
	o  ATYP   address type of following address
	o  IP V4 address: X'01'
	o  DOMAINNAME: X'03'
	o  IP V6 address: X'04'
	o  DST.ADDR       desired destination address
	o  DST.PORT desired destination port in network octet order

	*/

	boost::system::error_code ec;

	std::size_t n = boost::asio::read(downstream_socket_, 
		boost::asio::buffer(downstream_buf_.data() + downstream_bytes_read_ , downstream_buf_.size() - downstream_bytes_read_),
		boost::asio::transfer_exactly(offsetof(struct sock5_msg, dmn_name) + 1), 
		ec);

	if (ec)
	{
		close(ec);
		return;
	}

	downstream_bytes_read_ += n;

	sock5_msg *p = (sock5_msg *)downstream_buf_.data();
	std::size_t request_size;
	std::size_t bytes_left;
	
	request_size = s5_get_request_size();
	if (0 == request_size )
	{
		s5_reply(s5_replies::address_type_not_supported);
		
		return;
	}

	bytes_left = request_size - downstream_bytes_read_;


	n = boost::asio::read(downstream_socket_, 
		boost::asio::buffer(downstream_buf_.data() + downstream_bytes_read_, downstream_buf_.size() - downstream_bytes_read_), 
		boost::asio::transfer_exactly(bytes_left), ec);
	if (ec)
	{
		close(ec);
		return;
	}

	downstream_bytes_read_ += n;

	s5_process_request();

}

void s5session::s5_process_request()
{
	switch ( static_cast<s5_command>(downstream_buf_[1]) )
	{
	case s5_command::connect: 
		s5_reslove();
		break;

	case s5_command::bind: 
	case s5_command::udp_associate: 
	default:
		s5_reply(s5_replies::command_not_supported);
		
		break;
	}

}

void s5session::s5_reslove()
{
	string target_host;
	string target_port;

	switch (s5_get_address_type()) //
	{
	case s5_addr_type::ipv4:
		target_host = boost::asio::ip::address_v4(ntohl(*((uint32_t*)&downstream_buf_[4]))).to_string();
		target_port = std::to_string(ntohs(*((uint16_t*)&downstream_buf_[8])));

		
		break;
	case s5_addr_type::ipv6:
		//close(boost::system::errc::make_error_code(boost::system::errc::success));
		s5_reply(s5_replies::address_type_not_supported);
		return;
		break;
	case s5_addr_type::domain_name:
		target_host = std::string((char*)&downstream_buf_[5], s5_get_domain_size());
		target_port = std::to_string(ntohs(*((uint16_t*)&downstream_buf_[5 + s5_get_domain_size()])));
		break;
	default:

		break;
		
	}

	std::cout << "[" << id_ << "] " << "target " << target_host << ":" << target_port << std::endl;

	auto self(shared_from_this());
	auto resolver = std::make_shared<tcp::resolver>(downstream_socket_.get_io_context());

	auto handler = [this, self, resolver](const boost::system::error_code& ec, tcp::resolver::iterator ep_iterator) {
		if (ec)
		{
			s5_reply(s5_replies::host_unreachable);
			
			return;
		}
		else
		{
			s5_connect(ep_iterator);
		}
	};


	tcp::resolver::query q{ target_host,target_port };
	resolver->async_resolve(q, handler);
}

void s5session::s5_connect(tcp::resolver::iterator& it)
{
	auto self(shared_from_this());

	auto handler = [this, self](const boost::system::error_code& ec,
		tcp::resolver::iterator it) {
		if (ec) {
			s5_reply(s5_replies::network_unreachable);
			return;
		}

		s5_reply(s5_replies::succeeded);


		
	};

	boost::asio::async_connect(upstream_socket_, it, handler);
}

void s5session::s5_reply(s5_replies r)
{
	auto self(shared_from_this());

	auto handler = [this, self, r](const boost::system::error_code& ec, std::size_t) {
		if (ec) {
			close(ec);
			return;
		}

		if (r == s5_replies::succeeded)
		{
			s5_upstream_read();
			s5_downstream_read();
		}
		else
		{
			close(boost::system::errc::make_error_code(boost::system::errc::protocol_error));
		}
	};

	downstream_buf_[0] = 0x05;
	downstream_buf_[1] = static_cast<unsigned char>(r);
	downstream_buf_[2] = 0x00;
	downstream_buf_[3] = static_cast<unsigned char>(s5_addr_type::ipv4); // if this value is domain_name, then BND.ADDR and BND.PORT have to be set

	const std::size_t response_size = 10;  // TODO : BND.ADDR and BND.PORT
	for (std::size_t i = 4; i < response_size; ++i) {
		downstream_buf_.at(i) = 0x00;
	}

	boost::asio::async_write(downstream_socket_,
		boost::asio::buffer(downstream_buf_.data(), response_size), boost::asio::transfer_all(), handler);
}

void s5session::s5_upstream_read()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			//std::cout << "[" << id_ << "] " << "---> " << length << " bytes" << endl;
			s5_downstream_write(length);
		}
		else {
			//std::cout << "[" << id_ << "] " << "---> " << ec.message() << endl;
			close(ec);
		}
	};

	upstream_socket_.async_read_some(
		boost::asio::buffer(upstream_buf_.data(), upstream_buf_.size()), handler);
}

void s5session::s5_downstream_read()
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			//std::cout << "[" << id_ << "] " << "    <--- " << length << " bytes" << endl;
			s5_upstream_write(length);
		}
		else {
			//std::cout << "[" << id_ << "] " << "    <--- " << ec.message() << endl;
			close(ec);
		}
	};

	downstream_socket_.async_read_some(
		boost::asio::buffer(downstream_buf_.data(), downstream_buf_.size()), handler);
}

void s5session::s5_upstream_write(std::size_t length)
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			//std::cout << "[" << id_ << "] " << "<--- " << length << " bytes" << endl;
			s5_downstream_read();
		}
		else {
			//std::cout << "[" << id_ << "] " << "<--- " << ec.message() << endl;
			close(ec);
		}
	};

	boost::asio::async_write(upstream_socket_,
		boost::asio::buffer(downstream_buf_.data(), length), boost::asio::transfer_all(), handler);
}

void s5session::s5_downstream_write(std::size_t length)
{
	auto self(shared_from_this());
	auto handler = [this, self](const boost::system::error_code& ec, std::size_t length) {
		if (!ec) {

			//std::cout << "[" << id_ << "] " << "    ---> " << length << " bytes" << endl;
			s5_upstream_read();
		}
		else {
			//std::cout << "[" << id_ << "] " << "    ---> " << ec.message() << endl;
			close(ec);
		}
	};

	boost::asio::async_write(downstream_socket_,
		boost::asio::buffer(upstream_buf_.data(), length), boost::asio::transfer_all(), handler);
}

s5_addr_type s5session::s5_get_address_type() const
{
	return static_cast<s5_addr_type>(downstream_buf_[3]);
}

std::size_t s5session::s5_get_domain_size() const
{
	return static_cast<unsigned char>(downstream_buf_[4]);
}

std::size_t s5session::s5_get_request_size() const
{
	switch (s5_get_address_type()) {
	case s5_addr_type::ipv4: {
		return 10;
		break;
	}
	case s5_addr_type::domain_name: {
		return 7 + s5_get_domain_size();
		break;
	}
	case s5_addr_type::ipv6: {
		return 24;
		break;
	}
	}

	return 0;
}

void s5session::close(const boost::system::error_code& ec)
{

	if (downstream_socket_.is_open()) {
		downstream_socket_.close();
	}

	if (upstream_socket_.is_open()) {
		upstream_socket_.close();
	}
}
