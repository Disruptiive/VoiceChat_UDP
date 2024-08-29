#pragma once

#include "constants.hpp"
#include <iostream>
#include <mutex>
class SharedMemory {
public:
    SharedMemory() : data(new unsigned char[constants::MAX_PACKET_SIZE])
    {}

    ~SharedMemory() {
        delete[] data;
    }

    SharedMemory& operator= (const SharedMemory& other) = delete;

    SharedMemory(const SharedMemory& other) = delete;

    SharedMemory(SharedMemory&& other) = delete;

    SharedMemory& operator= (SharedMemory&& other) = delete;

    void write(unsigned char* in, int bytes) {
        std::unique_lock lk(mtx);
        bool status = cv.wait_for(lk, std::chrono::seconds(1), [&] {return !toRead || done; });
        if (!status) {
            std::cout << "Write Timeout" << "\n";
            exit(EXIT_FAILURE);
            
        }
        if (done) {
            return;
        }
        std::copy_n(in, bytes, data);
        m_bytes = bytes;
        toRead = true;
        cv.notify_one();
    }

    int read(unsigned char* out) {
        std::unique_lock lk(mtx);
        bool status = cv.wait_for(lk, std::chrono::seconds(1), [&] {return toRead || done; });
        if (!status) {
            std::cout << "Read Timeout" << "\n";
            exit(EXIT_FAILURE);
        }

        if (done) {
            return 0;
        }
        std::copy_n(data, m_bytes, out);
        toRead = false;
        cv.notify_one();
        return m_bytes;
    }

    void set_done() {
        done = true;
    }
private:
    std::mutex mtx;
    unsigned char* data;
    int m_bytes{};
    std::condition_variable cv;
    bool toRead{ false };
    bool done{ false };
};