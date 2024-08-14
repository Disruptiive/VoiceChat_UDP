#include <iostream>
#include <portaudio.h>
#include <opus/opus.h>
#include "constants.h"

struct AudioData {
    OpusEncoder* encoder;
    OpusDecoder* decoder;
    unsigned char* data;
    int* bytes_per_frame;
    int cnt;
    int idx;
};

static void checkError(PaError err) {
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

static int paRecordCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{   
    (void)outputBuffer;
    AudioData* audio_data = (AudioData*)userData;
    OpusEncoder* encoder = audio_data->encoder;
    const float* in = (float*)inputBuffer;
    int ec = opus_encode_float(encoder, in, constants::FRAMES_PER_BUFFER, &(audio_data->data[audio_data->cnt]), constants::MAX_PACKET_SIZE);
    if (ec < 0) {
        std::cout << ec << " ERROR CODE WHILE ENCODING" << "\n";
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << ec << " bytes transferred" << "\n";
        audio_data->bytes_per_frame[audio_data->idx++] = ec;
        audio_data->cnt += ec;
    }
    return 0;
}

static int paPlayCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
    AudioData* audio_data = (AudioData*)userData;
    OpusDecoder* decoder = audio_data->decoder;
    float* out = (float*)outputBuffer;

    
    int chunk_start = audio_data->cnt;
    int curr_idx = audio_data->idx++;
    audio_data->cnt += audio_data->bytes_per_frame[curr_idx];

    opus_decode_float(decoder, &(audio_data->data[chunk_start]), audio_data->bytes_per_frame[curr_idx], out, constants::FRAMES_PER_BUFFER, 0);


    return 0;
}




int main() {
    PaError err = paNoError;
    int opusErrorCE;
    int opusErrorCD;

    OpusEncoder* enc;
    OpusDecoder* dec;
    enc = opus_encoder_create(constants::SAMPLERATE, 2, OPUS_APPLICATION_VOIP, &opusErrorCE);
    dec = opus_decoder_create(constants::SAMPLERATE, 2, &opusErrorCD);

    if (opusErrorCE != OPUS_OK) {
        std::cout << "OPUS ERROR CODE: " << opusErrorCE << "\n";
        exit(EXIT_FAILURE);
    }

    if (opusErrorCD != OPUS_OK) {
        std::cout << "OPUS ERROR CODE: " << opusErrorCD << "\n";
        exit(EXIT_FAILURE);
    }
    
    opus_encoder_ctl(enc, OPUS_SET_BITRATE(156000));

    AudioData au_data{ .encoder = enc, .decoder = dec, .data = new unsigned char[constants::SAMPLERATE * constants::SECONDS], .bytes_per_frame = new int[constants::SECONDS*(constants::SAMPLERATE/constants::FRAMES_PER_BUFFER)] ,.cnt{0}};

    err = Pa_Initialize();
    checkError(err);

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
        printDevices(i);
    }

    int input_device_id = Pa_GetDefaultInputDevice();
    std::cout << "Default Input Device: \n";
    printDevices(input_device_id);

    int output_device_id = Pa_GetDefaultOutputDevice();
    std::cout << "Default Output Device: \n";
    printDevices(output_device_id);

    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.device = input_device_id;
    inputParameters.channelCount = 2;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(input_device_id)->defaultLowInputLatency;


    PaStream* stream;
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        nullptr,
        constants::SAMPLERATE,
        constants::FRAMES_PER_BUFFER,
        paNoFlag,
        paRecordCallBack,
        (void*) &au_data
    );
    checkError(err);

    err = Pa_StartStream(stream);
    checkError(err);

    Pa_Sleep(constants::SECONDS * 1000);

    err = Pa_StopStream(stream);
    checkError(err);

    err = Pa_CloseStream(stream);
    checkError(err);


    PaStreamParameters outputParameters;
    memset(&outputParameters, 0, sizeof(outputParameters));
    outputParameters.device = output_device_id;
    outputParameters.channelCount = 2;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(output_device_id)->defaultLowInputLatency;


    au_data.idx = 0;
    au_data.cnt = 0;
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        &outputParameters,
        constants::SAMPLERATE,
        constants::FRAMES_PER_BUFFER,
        paNoFlag,
        paPlayCallBack,
        (void*)&au_data
    );
    checkError(err);

    int gain;
    opus_decoder_ctl(au_data.decoder, OPUS_SET_GAIN(1000));

    err = Pa_StartStream(stream);
    checkError(err);

    Pa_Sleep(constants::SECONDS * 1000);

    err = Pa_StopStream(stream);
    checkError(err);

    err = Pa_CloseStream(stream);
    checkError(err);

    err = Pa_Terminate();
    checkError(err);

    opus_encoder_destroy(au_data.encoder);
    opus_decoder_destroy(au_data.decoder);
    delete[] au_data.data;
    delete[] au_data.bytes_per_frame;

    return EXIT_SUCCESS;

}