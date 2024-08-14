#include <iostream>
#include "player.h"
#include "constants.h"
#include "helpers.h"


int Player::paPlayCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{

    Player* plr = (Player*)userData;
    float* out = (float*)outputBuffer;

    int bytes = plr->m_sm->read(plr->data);
    opus_decode_float(plr->decoder, plr->data, bytes, out, constants::FRAMES_PER_BUFFER, 0);


    return 0;
}

void Player::initializeStream(int device_id)
{
    if (device_id == -1) {
        device_id = Pa_GetDefaultOutputDevice();
        std::cout << "Default Output Device: \n";
        helpers::printDevices(device_id);
    }

    PaStreamParameters outputParameters;
    memset(&outputParameters, 0, sizeof(outputParameters));
    outputParameters.device = device_id;
    outputParameters.channelCount = channel_c;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(device_id)->defaultLowInputLatency;

    int err = Pa_OpenStream(
        &stream,
        nullptr,
        &outputParameters,
        constants::SAMPLERATE,
        constants::FRAMES_PER_BUFFER,
        paNoFlag,
        paPlayCallBack,
        (void*) this
    );

    helpers::checkError(err);
}

Player::Player(Player&& other) noexcept {
    decoder = other.decoder;
    other.decoder = nullptr;

    stream = other.stream;
    other.stream = nullptr;

    data = other.data;
    other.data = nullptr;

    channel_c = other.channel_c;
    bytes_per_frame = other.bytes_per_frame;

    m_sm = other.m_sm;
    other.m_sm = nullptr;
}

Player& Player::operator=(Player&& other) noexcept {
    if (this != &other) {

        opus_decoder_destroy(decoder);
        decoder = other.decoder;
        other.decoder = nullptr;

        delete stream;
        stream = other.stream;
        other.stream = nullptr;

        delete[] data;
        data = other.data;
        other.data = nullptr;

        channel_c = other.channel_c;
        bytes_per_frame = other.bytes_per_frame;

        m_sm = other.m_sm;
        other.m_sm = nullptr;
    }
    return *this;
}
