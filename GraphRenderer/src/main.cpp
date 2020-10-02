
#include "Engine.h"
#include <exception>
#include <iostream>

int main() {

	gr::Engine::init();

	gr::Engine engine;
	try {
		engine.run();
	}
	catch (const std::exception& exc) {
		std::cerr << exc.what() << std::endl;
		return 1;
	}

	gr::Engine::terminate();
	return 0;
}