#include <iostream>
#include "stdlib.h"
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>

int main(int argc, char **argv)
{
	// IPv4 address of the interface we want to sniff
	std::string interfaceIPAddr = "192.168.0.3";

	// find the interface by IP address
	pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
	if (dev == NULL)
	{
		std::cerr << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
		return 1;
	}

	std::cout
		<< "Interface info:" << std::endl
		<< "   Interface name:        " << dev->getName() << std::endl			 // get interface name
		<< "   Interface description: " << dev->getDesc() << std::endl			 // get interface description
		<< "   MAC address:           " << dev->getMacAddress() << std::endl	 // get interface MAC address
		<< "   Default gateway:       " << dev->getDefaultGateway() << std::endl // get default gateway
		<< "   Interface MTU:         " << dev->getMtu() << std::endl;			 // get interface MTU

	if (dev->getDnsServers().size() > 0)
		std::cout << "   DNS server:            " << dev->getDnsServers().at(0) << std::endl;

	return 0;
}
