#include <iostream>
#include "stdlib.h"

#include "AppHandler.h"

int main(int argc, char **argv)
{
	AppHandler::setupLogger();

	if (!AppHandler::parseComandLine(argc, argv))
		return 0;

	if (!AppHandler::initialize())
		return 0;

	AppHandler::start();
	AppHandler::terminate();

	return 0;
}
