#pragma once
#include <portaudio.h>

namespace helpers {
	void checkError(PaError err);

	void printDevices(int i);

	void getAndPrintAllDevices();
}