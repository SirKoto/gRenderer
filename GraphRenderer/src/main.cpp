
#include "Engine.h"
#include "utils/grjob.h"

int main() {

	gr::Engine::init();

	gr::grjob::createSystem(6);

	gr::grjob::Job mainJob([]() {

		gr::Engine engine;

		engine.run();
		
		gr::grjob::stopRunningJobSystem();
	});



	gr::grjob::runJobOnMainThread(mainJob, nullptr, true);

	gr::grjob::startRunningJobSystem();


	gr::Engine::terminate();

	gr::grjob::destroySystem();

	return 0;
}