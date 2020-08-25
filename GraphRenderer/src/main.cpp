
#include "Engine.h"
#include <exception>
#include <iostream>

int main() {

	Engine::init();

	Engine engine;
	try {
		engine.run();
	}
	catch (const std::exception& exc) {
		std::cerr << exc.what() << std::endl;
		return 1;
	}

	Engine::terminate();
	return 0;
}