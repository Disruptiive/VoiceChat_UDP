#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include "constants.h"

using boost::asio::ip::udp;

class Server {
public:
	Server(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v6(),6001)), m_buf(new unsigned char[constants::MAX_PACKET_SIZE]){
		udp::resolver resolver(io_context);
		m_remote_endpoint = *resolver.resolve(udp::v6(), "", "fredo").begin();
		std::cout << m_remote_endpoint.port() <<"\n";
	}

	void send_message(int bytes, unsigned char* buf) {
		std::memcpy(m_buf, buf, bytes);
		m_bytes[0] = bytes;
		send_bytes();
	}

	void send_bytes() {
		m_socket.async_send_to(
			boost::asio::buffer(m_bytes, 1), m_remote_endpoint,
			[&](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				send_buffer(ec, l);
			}
		);
	}

	void send_buffer(const boost::system::error_code& error, std::size_t l) {
		m_socket.async_send_to(
			boost::asio::buffer(m_buf, m_bytes[0]), m_remote_endpoint,
			[&](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << " Sent bytes : " << l << "\n";
			}
		);
	}

	~Server() {
		delete[] m_buf;
	}
private:
	udp::socket m_socket;
	udp::endpoint m_remote_endpoint;
	unsigned char* m_buf;
	int m_bytes[1]{ 0 };
};