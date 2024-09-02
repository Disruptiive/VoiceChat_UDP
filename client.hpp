#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <chrono>
#include "constants.hpp"

using boost::asio::ip::udp;

struct UnpackedPacket {
    std::vector<unsigned char> buf;
    uint64_t timestamp{};
    uint64_t bytes{};
};

class ClientReceiver{
public:
    ClientReceiver(boost::asio::io_context& io_context) : m_socket(io_context, udp::endpoint(udp::v4(), 6001)) {
        receive_bytes();
    }

    void receive_bytes() {
        using ReceivingPacket = std::vector<unsigned char>;
        auto pkt = std::make_shared<ReceivingPacket>(constants::DUMMY_NUM);
        m_socket.async_receive_from(
            boost::asio::buffer(*pkt), m_remote_endpoint,
            [this, pkt](const boost::system::error_code& ec, std::size_t l) {
                if (ec) {
                    std::cerr << "Error receiving info: " << ec.message() << "\n";
                    receive_bytes(); // Try again
                    return;
                }

                auto packet = std::make_shared<UnpackedPacket>();
                std::memcpy(&(packet->bytes), pkt->data(), sizeof(uint64_t));
                std::memcpy(&(packet->timestamp), pkt->data()+ sizeof(uint64_t), sizeof(uint64_t));
                std::cout << "Received: " << packet->bytes << " with timestamp: " << packet->timestamp << "\n";
                if (packet->bytes < 300) {
                    packet->buf = std::vector<unsigned char>(packet->bytes);
                    std::memcpy(packet->buf.data(), pkt->data() + 2 * sizeof(uint64_t), packet->bytes);
                    {
                        std::lock_guard<std::mutex> lk(mtx);
                        packet_q.push(packet);
                    }
                    cv.notify_one();
                    receive_bytes();
                }
                else {
                    receive_bytes();
                }
            }
        );
    }

    std::vector<std::shared_ptr<UnpackedPacket>> getData() {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [this] { return !packet_q.empty(); });
        std::vector<std::shared_ptr<UnpackedPacket>> v;


        while (!packet_q.empty() && (v.empty() || (packet_q.top()->timestamp - v.front()->timestamp) < delay)) {
            
            
            auto pkt = packet_q.top();
            packet_q.pop();

            v.push_back(pkt);
        }
        
        
        return std::move(v);
    }

private:
    const int delay{ constants::SAMPLERATE / constants::FRAMES_PER_BUFFER };
    std::mutex mtx;
    std::condition_variable cv;
    udp::socket m_socket;
    udp::endpoint m_remote_endpoint;

    std::priority_queue <
        std::shared_ptr<UnpackedPacket>,
        std::vector<std::shared_ptr<UnpackedPacket>>,
        decltype([](const auto& p1, const auto& p2) { return p1->timestamp > p2->timestamp; })
    > packet_q;
    
};


class ClientSender{
public:
    using SendingPacket = std::vector<unsigned char>;
    ClientSender(boost::asio::io_context& io_context): m_socket(io_context, udp::endpoint(udp::v4(), 7996)) {
        udp::resolver resolver(io_context);
        m_remote_endpoint = *resolver.resolve(udp::v4(), "", "fredo").begin();
    }

    void send_message(int bytes, unsigned char* buf) {
        auto pkt = std::make_shared<SendingPacket>();
        const auto now = std::chrono::system_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        uint64_t uint_bytes = static_cast<uint64_t> (bytes);
        pkt->resize(sizeof(unsigned char) * bytes+sizeof(uint64_t)*2);

        std::memcpy(pkt->data(), &uint_bytes, sizeof(uint64_t));
        std::memcpy(pkt->data() + sizeof(uint64_t), &timestamp, sizeof(uint64_t));
        std::memcpy(pkt->data() + 2 * sizeof(uint64_t), buf, sizeof(buf[0]) * bytes);

        send_buffer(pkt);
    }

    void send_buffer(std::shared_ptr<SendingPacket> pkt) {
        m_socket.async_send_to(
            boost::asio::buffer(pkt->data(), pkt->size() * sizeof(unsigned char)), m_remote_endpoint,
            [this, pkt](const boost::system::error_code& ec, std::size_t l) {
                if (ec) {
                    std::cout << ec.what() << "\n";
                    exit(EXIT_FAILURE);
                }

            }
        );
    }

private:
    udp::socket m_socket;
    udp::endpoint m_remote_endpoint;
};