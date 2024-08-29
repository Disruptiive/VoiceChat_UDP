#include "helpers.hpp"
#include <iostream>

namespace helpers {
    void checkError(PaError err) {
        if (err != paNoError) {
            std::cout << Pa_GetErrorText(err) << "\n";
            exit(EXIT_FAILURE);
        }
    }

    void printDevices(int i) {
        const PaDeviceInfo* deviceinfo;
        deviceinfo = Pa_GetDeviceInfo(i);
        std::cout << "Device: " << i << " name: " << deviceinfo->name << " maxInputChannels: " << deviceinfo->maxInputChannels << " maxOutputChannels: " << deviceinfo->maxOutputChannels << " default sample rate: " << deviceinfo->defaultSampleRate << "\n";
    }

    void getAndPrintAllDevices() {
        int numDevices{ Pa_GetDeviceCount() };
        if (numDevices < 0) {
            std::cout << "Couldn't get device count!\n";
            exit(EXIT_FAILURE);
        }
        else if (numDevices == 0) {
            std::cout << "No availlable devices in this machine!\n";
            exit(EXIT_SUCCESS);
        }
        std::cout << "Number of Devices found: " << numDevices << "\n";

        for (int i = 0; i < numDevices; ++i) {
            helpers::printDevices(i);
        }
    }
}