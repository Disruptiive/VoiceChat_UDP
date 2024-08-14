#include <iostream>
#include <portaudio.h>
#include <opus/opus.h>
#include "constants.h"
#include "recorder.h"
#include "player.h"
#include "sharedmemory.h"
#include "helpers.h"
#include <mutex>


void recorder(SharedMemory* sm) {
    auto rec{ Recorder(sm,1) };

    rec.initializeStream();
    rec.startStream();
    //Pa_Sleep(constants::SECONDS * 1000);
    while (1) {}
    rec.stopStream();
    rec.closeStream();
}

void receiver(SharedMemory* sm) {
    auto plr{ Player(sm,1) };

    plr.initializeStream();
    plr.startStream();
    //Pa_Sleep(constants::SECONDS * 1000);
    while (1) {}
    plr.stopStream();
    plr.closeStream();

}

int main() {
    PaError err = paNoError;

    err = Pa_Initialize();
    helpers::checkError(err);

    auto sm{ SharedMemory() };

    std::thread t(receiver,&sm);
    
    recorder(&sm);
    sm.set_done();
    t.join();
    
    err = Pa_Terminate();
    helpers::checkError(err);


    return EXIT_SUCCESS;

}