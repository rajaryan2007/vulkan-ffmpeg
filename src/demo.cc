#include "demo.hh"
#include "Application.hh"
#include "Logger.h"

Demo::Demo() {
	LOG("Demo constructor called");
}

void Demo::run() {
	LOG("Demo run method called");
	app.run();
}

Demo::~Demo() {
	LOG("Demo destructor called");
}


