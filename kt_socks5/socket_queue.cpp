#include "socket_queue.h"



socket_queue::socket_queue()
{
}


socket_queue::~socket_queue()
{
}

void socket_queue::enqueue(boost::asio::ip::tcp::socket sock)
{
	std::lock_guard<std::mutex> lk(mut_);
	queue_.push(std::move(sock));
}

bool socket_queue::dequeue(boost::asio::ip::tcp::socket& sock)
{
	std::lock_guard<std::mutex> lk(mut_);
	if (queue_.empty())
	{
		return false;
	}
	sock = std::move(queue_.front());
	queue_.pop();
	return true;
}
