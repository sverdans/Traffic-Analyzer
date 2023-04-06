#include <iostream>
#include "stdlib.h"

#include "AppHandler.h"

int main(int argc, char **argv)
{
	if (!AppHandler::initialize(argc, argv))
		return 0;

	AppHandler::start();
	AppHandler::terminate();

	return 0;
}
