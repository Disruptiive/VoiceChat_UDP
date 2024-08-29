#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <chrono>
#include <queue>
#include "constants.hpp"
#include "ringbuffer.hpp"

using boost::asio::ip::udp;

struct ReceivingPacket {
	unsigned char* buf;
	uint64_t timestamp;
	uint64_t bytes;
	uint64_t info[2]{ 0,0 };
};
class ClientReceiver {
public:
	ClientReceiver(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v6(), 6001)), m_buf(new unsigned char[constants::MAX_PACKET_SIZE]), rb(RingBuffer<unsigned char, 500, 512>())
	{
		receive_bytes();
	}
	void receive_bytes() {
		auto pkt{ std::make_shared<ReceivingPacket>() };
		m_socket.async_receive_from(
			boost::asio::buffer(pkt->info, sizeof(pkt->info)), m_remote_endpoint,
			[&, pkt](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				pkt->bytes = pkt->info[0];
				pkt->timestamp = pkt->info[1];
				pkt->buf = rb.getChunk();
				std::cout << "Will receive: " << pkt->bytes << " Timestamp: " << pkt->timestamp << "\n";

				receive_buffer(ec, l, pkt);
			}
		);
	}

	void receive_buffer(const boost::system::error_code& error, std::size_t l, std::shared_ptr<ReceivingPacket> pkt) {
		m_socket.async_receive_from(
			boost::asio::buffer(pkt->buf, pkt->bytes), m_remote_endpoint,
			[&, pkt](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << " Received bytes : " << l << "\n";
				std::unique_lock lk(mtx);
				packet_q.push(pkt);
				cv.notify_one();
				receive_bytes();
			}
		);
	}

	int getData(unsigned char* out) {
		std::unique_lock lk(mtx);
		std::cout << packet_q.size() << "\n";
		cv.wait(lk, [&] {return !packet_q.empty(); });
		auto pkt = packet_q.front();
		std::memcpy(out, pkt->buf, pkt->bytes);
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		packet_q.pop();
		rb.returnChunks(1);
		return pkt->bytes;
	}
	~ClientReceiver() {
		delete[] m_buf;
	}

private:
	std::mutex mtx;
	std::condition_variable cv;
	udp::socket m_socket;
	udp::endpoint m_remote_endpoint;
	unsigned char* m_buf;
	RingBuffer<unsigned char, 500, 512> rb;
	std::queue<std::shared_ptr<ReceivingPacket>> packet_q;
};
class ClientSender {
public:
	ClientSender(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v6(), 7996)), m_buf(new unsigned char[constants::MAX_PACKET_SIZE]) {
		udp::resolver resolver(io_context);
		m_remote_endpoint = *resolver.resolve(udp::v6(), "", "fredo").begin();
		std::cout << m_remote_endpoint.port() << "\n";
	}

	void send_message(int bytes, unsigned char* buf) {
		std::memcpy(m_buf, buf, bytes);
		auto pkt = std::make_shared<ReceivingPacket>();
		const auto now = std::chrono::steady_clock::now();
		pkt->info[0] = bytes;
		pkt->info[1] = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		pkt->buf = buf;
		send_bytes(pkt);
	}

	void send_bytes(std::shared_ptr<ReceivingPacket> pkt) {
		m_socket.async_send_to(
			boost::asio::buffer(pkt->info, sizeof(pkt->info)), m_remote_endpoint,
			[&, pkt](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << "Sent timestamp: " << pkt->info[1] << "\n";
				send_buffer(ec, l, pkt);
			}
		);
	}

	void send_buffer(const boost::system::error_code& error, std::size_t l, std::shared_ptr<ReceivingPacket> pkt) {
		m_socket.async_send_to(
			boost::asio::buffer(pkt->buf, pkt->info[0]), m_remote_endpoint,
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
};