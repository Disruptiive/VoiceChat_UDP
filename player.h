#pragma once
#include <iostream>
#include "constants.h"
#include "helpers.h"
#include "sharedmemory.h"
#include <opus/opus.h>
#include <portaudio.h>

class Player {
public:
	Player(SharedMemory* sm, int channels = 1, int gain = 0) :decoder(opus_decoder_create(constants::SAMPLERATE, channels, &opusError)), stream(nullptr), data(new unsigned char[constants::MAX_PACKET_SIZE]), channel_c(channels), m_sm(sm) {
		if (opusError != OPUS_OK) {
			std::cout << "OPUS ERROR CODE: " << opusError << "\n";
			exit(EXIT_FAILURE);
		}

		opus_decoder_ctl(decoder, OPUS_SET_GAIN(gain));
	}

	Player& operator= (const Player& other) = delete;

	Player(const Player& other) = delete;

	Player(Player&& other) noexcept;

	Player& operator= (Player&& other) noexcept;

	~Player() {
		delete[] data;
		opus_decoder_destroy(decoder);
		//closeStream();
	}

	void initializeStream(int device_id = -1);

	void startStream() {
		int err = Pa_StartStream(stream);
		helpers::checkError(err);
	}

	void stopStream() {
		int err = Pa_StopStream(stream);
		helpers::checkError(err);
	}

	void closeStream() {
		int err = Pa_CloseStream(stream);
		helpers::checkError(err);
	}

	static int paPlayCallBack(
		const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

private:
	OpusDecoder* decoder;
	PaStream* stream;
	unsigned char* data;
	int bytes_per_frame{};
	int channel_c{};
	int opusError{};

	SharedMemory* m_sm{ nullptr };
};