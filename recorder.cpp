#include <iostream>
#include "recorder.h"
#include "constants.h"
#include "helpers.h"


int Recorder::paRecordCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    (void)outputBuffer;
    Recorder* rec = (Recorder*)userData;
    const float* in = (float*)inputBuffer;
    int ec = opus_encode_float(rec->encoder, in, constants::FRAMES_PER_BUFFER, rec->data, constants::MAX_PACKET_SIZE);
    if (ec < 0) {
        std::cout << ec << " ERROR CODE WHILE ENCODING" << "\n";
        exit(EXIT_FAILURE);
    }
    else {
        //std::cout << ec << " bytes transferred" << "\n";
        rec->m_server->send_message(ec, rec->data);
    }
    return 0;
}

void Recorder::initializeStream(int device_id)
{
    if (device_id == -1) {
        device_id = Pa_GetDefaultInputDevice();
        std::cout << "Default Input Device: \n";
        helpers::printDevices(device_id);
    }

    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.device = device_id;
    inputParameters.channelCount = channel_c;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(device_id)->defaultLowInputLatency;

    int err = Pa_OpenStream(
        &stream,
        &inputParameters,
        nullptr,
        constants::SAMPLERATE,
        constants::FRAMES_PER_BUFFER,
        paNoFlag,
        paRecordCallBack,
        (void*) this
    );

    helpers::checkError(err);
}

Recorder::Recorder(Recorder&& other) noexcept{
    encoder = other.encoder;
    other.encoder = nullptr;

    stream = other.stream;
    other.stream = nullptr;

    data = other.data;
    other.data = nullptr;

    channel_c = other.channel_c;
    bytes_per_frame = other.bytes_per_frame;

    m_server = other.m_server;
    other.m_server = nullptr;
}

Recorder& Recorder::operator=(Recorder&& other) noexcept {
    if (this != &other) {

        opus_encoder_destroy(encoder);
        encoder = other.encoder;
        other.encoder = nullptr;

        delete stream;
        stream = other.stream;
        other.stream = nullptr;

        delete[] data;
        data = other.data;
        other.data = nullptr;

        channel_c = other.channel_c;
        bytes_per_frame = other.bytes_per_frame;

        m_server = other.m_server;
        other.m_server = nullptr;
    }
    return *this;
}
