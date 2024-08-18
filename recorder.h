#pragma once
#include "sharedmemory.h"
#include "server.h"
#include "client.h"
#include "helpers.h"
#include "constants.h"
#include <opus/opus.h>
#include <portaudio.h>
#include <iostream>

class Recorder {
public:
	Recorder(ClientSender* server, int channels = 1, int bitrate = 156000) :encoder(opus_encoder_create(constants::SAMPLERATE, channels, OPUS_APPLICATION_VOIP, &opusError)), stream(nullptr), data(new unsigned char[constants::MAX_PACKET_SIZE]), channel_c(channels), m_server(server) {
		if (opusError != OPUS_OK) {
			std::cout << "OPUS ERROR CODE: " << opusError << "\n";
			exit(EXIT_FAILURE);
		}

		opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
	}

	Recorder& operator= (const Recorder& other) = delete;

	Recorder(const Recorder& other) = delete;

	Recorder(Recorder&& other) noexcept;
	
	Recorder& operator= (Recorder&& other) noexcept;

	~Recorder() {
		delete[] data;
		opus_encoder_destroy(encoder);
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

	static int paRecordCallBack(
		const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

private:
	OpusEncoder* encoder;
	PaStream* stream;
	unsigned char* data;
	int bytes_per_frame{};
	int channel_c{};
	int opusError{};

	ClientSender* m_server{ nullptr };
};