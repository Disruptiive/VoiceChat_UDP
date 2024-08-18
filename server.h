#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <set>
#include "RingBuffer.h"
#include "constants.h"

using boost::asio::ip::udp;

struct Packet {
	unsigned char* buf;
	uint64_t timestamp;
	int bytes;
};

class Server {
public:
	Server(boost::asio::io_context& io_context) : rb(RingBuffer<unsigned char, 30, 512>()), m_socket(io_context, udp::endpoint(udp::v6(), 6000)) {
		udp::resolver resolver(io_context);
		m_remote_endpoint = *resolver.resolve(udp::v6(), "", "fredo-chat").begin();
		start_receiving();
	}
private:
	void start_receiving() {
		auto new_packet = std::make_shared<Packet>();
		udp::endpoint remote_endpoint;
		std::cout << "Q sz: " << packet_q.size() << "\n";

		m_socket.async_receive_from(
			boost::asio::buffer(info,sizeof(info)), remote_endpoint,
			[&,new_packet](const boost::system::error_code& ec, std::size_t l) {
				if (ec) {
					std::cout << ec.what() << "\n";
					exit(EXIT_FAILURE);
				}
				new_packet->bytes = info[0];
				new_packet->timestamp = info[1];
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
				/*
				while (!packet_q.empty() && packet->timestamp - packet_q.front()->timestamp > 20) {
					//std::cout << "Popped one\n";
					packets_to_process.push_back(packet_q.front());
					packet_q.pop_front();
				}
				packet_q.push_back(packet);
				control_send();
				*/

				if (packet_q.empty()) {
					packet_q.push_back(packet);
					start_receiving();
				}
				else{
					packet_q.push_back(packet);
					auto p = packet_q.front();
					packet_q.pop_front();
					start_sending(p);
				}
				//start_sending(packet);
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
			boost::asio::buffer(&(pkt->bytes), sizeof(pkt->bytes)), m_remote_endpoint,
			[&,pkt](const boost::system::error_code& ec, std::size_t l) {
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
	uint64_t info[2]{ 0,0 };
	udp::endpoint m_remote_endpoint;
	RingBuffer<unsigned char, 30, 512> rb;
	std::deque<std::shared_ptr<Packet>> packet_q;
	udp::socket m_socket;
	std::set<udp::endpoint> m_remote_endpoint_set;
	std::vector<std::shared_ptr<Packet>> packets_to_process;
};


