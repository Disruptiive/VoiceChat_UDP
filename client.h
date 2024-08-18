#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <chrono>
#include "constants.h"

using boost::asio::ip::udp;

class ClientReceiver {
public:
	ClientReceiver(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v6(), 6001)), m_buf(new unsigned char[constants::MAX_PACKET_SIZE])
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
				std::cout << " Received bytes : " << l << "\n";
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

	~ClientReceiver() {
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

class ClientSender {
public:
	ClientSender(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v6(), 7999)), m_buf(new unsigned char[constants::MAX_PACKET_SIZE]) {
		udp::resolver resolver(io_context);
		m_remote_endpoint = *resolver.resolve(udp::v6(), "", "fredo").begin();
		std::cout << m_remote_endpoint.port() << "\n";
	}

	void send_message(int bytes, unsigned char* buf) {
		std::memcpy(m_buf, buf, bytes);
		const auto now = std::chrono::steady_clock::now();
		m_bytes[0] = bytes;
		m_bytes[1] = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		send_bytes();
	}

	void send_bytes() {
		m_socket.async_send_to(
			boost::asio::buffer(m_bytes, sizeof(m_bytes)), m_remote_endpoint,
			[&](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << "Sent L: " << l << "\n";
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

	~ClientSender() {
		delete[] m_buf;
	}
private:
	udp::socket m_socket;
	udp::endpoint m_remote_endpoint;
	unsigned char* m_buf;
	uint64_t m_bytes[2]{ 0,0 };
};