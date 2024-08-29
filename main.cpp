#include <iostream>
#include <boost/asio.hpp>
#include "server.hpp"
#include "client.h"
#include <portaudio.h>
#include <opus/opus.h>
#include "constants.hpp"
#include "recorder.hpp"
#include "player.hpp"
#include "sharedmemory.hpp"
#include "helpers.hpp"
#include <mutex>


void recorder(ClientSender* server) {
    auto rec{ Recorder(server,1,constants::BITRATE) };

    rec.initializeStream();
    rec.startStream();
    //Pa_Sleep(constants::SECONDS * 1000);
    while (1) {}
    rec.stopStream();
    rec.closeStream();
}

void receiver(ClientReceiver* client) {
    auto plr{ Player(client,1) };

    plr.initializeStream();
    plr.startStream();
    //Pa_Sleep(constants::SECONDS * 1000);
    while (1) {}
    plr.stopStream();
    plr.closeStream();

}

int main() {
    PaError err = paNoError;
    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);
    err = Pa_Initialize();
    helpers::checkError(err);

    helpers::getAndPrintAllDevices();
    auto client{ ClientReceiver(io_context) };

    auto server{ ClientSender(io_context) };
    

   

    
    std::thread io_thread([&io_context]() {
        try {
            io_context.run();
        }
        catch (std::exception& e) {
            std::cout << e.what() << "\n";
        }
        
    });
    
    std::thread t(receiver,&client);
    recorder(&server);
    t.join();
    io_thread.join();
    
    err = Pa_Terminate();
    helpers::checkError(err);


    return EXIT_SUCCESS;

}