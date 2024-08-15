#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include "constants.h"

using boost::asio::ip::udp;

class Client {
public:
	Client(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v6(), 6000)), m_buf(new unsigned char[constants::MAX_PACKET_SIZE])
	{
		receive_bytes();
	}
	void receive_bytes(){
		m_socket.async_receive_from(
			boost::asio::buffer(m_bytes,1), m_remote_endpoint,
			[&](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				receive_buffer(ec, l); 
			}
		);
	}

	void receive_buffer(const boost::system::error_code& error, std::size_t l) {
		m_socket.async_receive_from(
			boost::asio::buffer(m_buf, m_bytes[0]), m_remote_endpoint,
			[&](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				m_fresh = true;
				cv.notify_one();
				receive_bytes();
			}
		);
	}

	int getData(unsigned char* out) {
		std::unique_lock lk(mtx);
		cv.wait(lk, [&] {return m_fresh; });
		std::memcpy(out, m_buf, m_bytes[0]);
		resetFresh();
		return m_bytes[0];
	}

	bool isFresh() { return m_fresh; }
	void resetFresh() { m_fresh = false; }

	~Client() {
		delete[] m_buf;
	}

private:
	std::mutex mtx;
	std::condition_variable cv;
	udp::socket m_socket;
	udp::endpoint m_remote_endpoint;
	bool m_fresh{ false };
	unsigned char* m_buf;
	int m_bytes[1]{ 0 };
};