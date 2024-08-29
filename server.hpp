#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <set>
#include "ringbuffer.hpp"
#include "constants.hpp"

using boost::asio::ip::udp;

struct Packet {
	unsigned char* buf;
	uint64_t timestamp;
	uint64_t bytes;
	uint64_t info[2]{ 0,0 };
	udp::endpoint remote_endpoint;
};

class Server {
public:
	Server(boost::asio::io_context& io_context) : rb(RingBuffer<unsigned char, 500, 512>()), m_socket(io_context, udp::endpoint(udp::v6(), 6000)) {
		udp::resolver resolver(io_context);
		m_remote_endpoint = *resolver.resolve(udp::v6(), "", "fredo-chat").begin();
		start_receiving();
	}
private:
	void start_receiving() {
		auto new_packet = std::make_shared<Packet>();
		std::cout << "Q sz: " << packet_q.size() << "\n";
		m_socket.async_receive_from(
			boost::asio::buffer(new_packet->info, sizeof(new_packet->info)), new_packet->remote_endpoint,
			[&, new_packet](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				new_packet->bytes = new_packet->info[0];
				new_packet->timestamp = new_packet->info[1];
				new_packet->buf = rb.getChunk();
				//m_remote_endpoint_set.insert(remote_endpoint);
				receive_data(new_packet);

			}
		);
	}

	void receive_data(std::shared_ptr<Packet> packet) {
		udp::endpoint remote_endpoint;
		m_socket.async_receive_from(
			boost::asio::buffer(packet->buf, packet->bytes), remote_endpoint,
			[&, packet](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << "Received: " << l << " buf, timestamp: " << packet->timestamp << "\n";

				start_sending(packet);
			}
		);
	}

	void control_send() {
		int sz{ static_cast<int> (packets_to_process.size()) };
		for (auto p : packets_to_process) {
			start_sending(p);
		}
		rb.returnChunks(sz);
		start_receiving();
	}

	void start_sending(std::shared_ptr<Packet> pkt) {
		m_socket.async_send_to(
			boost::asio::buffer(pkt->info, sizeof(pkt->info)), m_remote_endpoint,
			[&, pkt](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}

				send_buffer(pkt);
			}
		);
	}

	void send_buffer(std::shared_ptr<Packet> pkt) {
		m_socket.async_send_to(
			boost::asio::buffer(pkt->buf, pkt->bytes), m_remote_endpoint,
			[&](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << " Sent bytes : " << l << "\n";
				rb.returnChunks(1);
				start_receiving();
			}
		);
	}
	udp::endpoint m_remote_endpoint;
	RingBuffer<unsigned char, 500, 512> rb;
	std::deque<std::shared_ptr<Packet>> packet_q;
	udp::socket m_socket;
	std::set<udp::endpoint> m_remote_endpoint_set;
	std::vector<std::shared_ptr<Packet>> packets_to_process;
};


