#include "server.h"

int main() {
	boost::asio::io_context io_context;
	auto server{ Server(io_context) };
	auto work_guard = boost::asio::make_work_guard(io_context);

	io_context.run();
}