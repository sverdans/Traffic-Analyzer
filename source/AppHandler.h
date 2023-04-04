#include <iostream>
#include <vector>

#include "PcapLiveDeviceList.h"

class AppHandler
{
public:
	static void printUsage() noexcept
	{
	}

	static void listInterfaces()
	{
		const std::vector<pcpp::PcapLiveDevice *> &devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

		std::cout << std::endl
				  << "Network interfaces:" << std::endl;

		for (auto iter = devList.begin(); iter != devList.end(); iter++)
		{
			std::cout << "    -> Name: '" << (*iter)->getName() << "'   IP address: " << (*iter)->getIPv4Address().toString() << std::endl;
		}
	}
};