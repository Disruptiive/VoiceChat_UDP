#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <deque>
#include <set>
#include <unordered_set>
#include "ringbuffer.hpp"
#include "constants.hpp"

using boost::asio::ip::udp;

struct PacketInfo {
    std::vector<unsigned char> buf;
    uint64_t timestamp{};
    uint64_t bytes{};
};

class Server {
public:
    Server(boost::asio::io_context& io_context)
        : m_socket(io_context, udp::endpoint(udp::v4(), 6000)), m_strand(io_context) {
        udp::resolver resolver(io_context);
        start_receiving();
    }

private:
    static constexpr size_t MAX_PACKET_SIZE = 65507; // Maximum UDP packet size

    void start_receiving() {
        auto buffer = std::make_shared<std::vector<unsigned char>>(constants::DUMMY_NUM);
        auto endpoint = std::make_shared<udp::endpoint>();

        m_socket.async_receive_from(
            boost::asio::buffer(*buffer),
            *endpoint,
            boost::asio::bind_executor(m_strand,
                [this, buffer, endpoint](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                    if (!ec){
                        udp::endpoint new_endpoint((*endpoint).address(), 6001);
                        endpoints.insert(new_endpoint);
                        std::cout << "Amount of endpoints: " << endpoints.size() << "\n";
                        handle_receive(buffer, bytes_transferred, *endpoint);
                    }
                    else {
                        handle_error(ec, "receive", *endpoint);
                        start_receiving(); // Start receiving next packet immediately
                    }
                }
            )
        );
    }

    void handle_receive(std::shared_ptr<std::vector<unsigned char>> buffer,
        std::size_t bytes_transferred,
        const udp::endpoint& sender_endpoint) {
        if (bytes_transferred <= sizeof(uint64_t) * 2) { // Minimum packet size (sequence, timestamp, data size)
            std::cout << "Got: " << bytes_transferred << " .Received incomplete packet\n";
            return;
        }

        auto packet = std::make_shared<PacketInfo>();

        std::memcpy(&(packet->bytes), buffer->data(), sizeof(uint64_t));
        std::memcpy(&(packet->timestamp), buffer->data() + sizeof(uint64_t), sizeof(uint64_t));
        if (packet->bytes >= 300) {
            std::cout << "Data size mismatch.\n";
            return;
        }
        std::cout << "Received: " << packet->bytes << " with timestamp: " << packet->timestamp << "\n";

        for (auto& e : endpoints) {
            std::cout << "Sending to endpoint: " << e.address().to_string() << "\n";
            start_sending(buffer, bytes_transferred, e);
        }
        
    }

    void start_sending(std::shared_ptr<std::vector<unsigned char>> buffer, std::size_t bytes_to_send, const udp::endpoint& target_endpoint) {
        m_socket.async_send_to(
            boost::asio::buffer(*buffer, bytes_to_send),
            target_endpoint,
            boost::asio::bind_executor(m_strand,
                [this,buffer, target_endpoint](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                    std::cout << "Sent: " << bytes_transferred << " bytes.\n";
                    if (ec) {
                        handle_error(ec, "send", target_endpoint);
                    }
                    start_receiving();
                }
            )
        );
    }

    void handle_error(const boost::system::error_code& ec, const std::string& operation, const udp::endpoint& target_endpoint) {
        udp::endpoint new_endpoint(target_endpoint.address(), 6001);
        endpoints.erase(new_endpoint);
        std::cout << "Error during " << operation << ": " << ec.message() << "\n";
    }


    boost::asio::io_context::strand m_strand;
    udp::socket m_socket;
    std::set<udp::endpoint> endpoints;
};