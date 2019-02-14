#pragma once
#include <queue>
#include <mutex>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class socket_queue
{
public:
	socket_queue();
	~socket_queue();

	void enqueue(boost::asio::ip::tcp::socket sock);
	bool dequeue(boost::asio::ip::tcp::socket& sock);

private:
	queue<boost::asio::ip::tcp::socket> queue_;
	std::mutex mut_;
};

