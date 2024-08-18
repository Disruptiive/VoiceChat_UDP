#include "server.h"

int main() {
	boost::asio::io_context io_context;
	auto server{ Server(io_context) };

	io_context.run();
}