#include <iostream>
#include "player.hpp"
#include "constants.hpp"
#include "helpers.hpp"


int Player::paPlayCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{

    Player* plr = (Player*)userData;
    float* out = (float*)outputBuffer;
    auto packets = plr->m_client->getData();
    if (packets.size() == 1) {
        opus_decode_float(plr->decoder, packets.front()->buf.data(), packets.front()->bytes, out, constants::FRAMES_PER_BUFFER, 0);
    }
    else {
        opus_decode_float(plr->decoder, packets.front()->buf.data(), packets.front()->bytes, plr->data, constants::FRAMES_PER_BUFFER, 0);

        float max_amplitude{ 0.0f };
        int cnt{ 1 };
        for (size_t i = 1; i < packets.size(); ++i) {
            opus_decode_float(plr->decoder, packets[i]->buf.data(), packets[i]->bytes, plr->tmp_buf, constants::FRAMES_PER_BUFFER, 0);
            ++cnt;
            for (int j = 0; j < constants::FRAMES_PER_BUFFER; ++j) {
                plr->data[j] += plr->tmp_buf[j];
                max_amplitude = std::max(max_amplitude, std::abs(plr->data[j]));
            }

        }
        if (max_amplitude > 1.0f) {
            float scaling_factor = 0.99f / max_amplitude;  // Leave a small headroom
            for (int j = 0; j < constants::FRAMES_PER_BUFFER; ++j) {
                plr->data[j] *= scaling_factor;
            }
        }
        else {
            // If no normalization is needed, we still need to average the samples
            for (int j = 0; j < constants::FRAMES_PER_BUFFER; ++j) {
                plr->data[j] /= cnt;
            }
        }

        const float threshold = 0.95f;
        const float knee = 0.05f;
        for (int j = 0; j < constants::FRAMES_PER_BUFFER; ++j) {
            float x = std::abs(plr->data[j]);
            if (x > threshold - knee) {
                if (x > threshold + knee) {
                    plr->data[j] *= threshold / x;
                }
                else {
                    float t = (x - (threshold - knee)) / (2 * knee);
                    float gain = 1.0f - t * t * (1.0f - (threshold / x));
                    plr->data[j] *= gain;
                }
            }
        }


        std::memcpy(out, plr->data, constants::FRAMES_PER_BUFFER * sizeof(float));
        std::cout << "Mixed " << cnt << " packets" << std::endl;
    }

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

    m_client = other.m_client;
    other.m_client = nullptr;
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

        m_client = other.m_client;
        other.m_client = nullptr;
    }
    return *this;
}
