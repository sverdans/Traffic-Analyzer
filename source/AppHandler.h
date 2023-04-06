#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>

#include <getopt.h>

#include "PcapLiveDeviceList.h"
#include "HttpLayer.h"
#include "TcpLayer.h"
#include "IPv4Layer.h"
#include "PayloadLayer.h"
#include "PacketUtils.h"
#include "SystemUtils.h"

#include "Packet.h"
#include "EthLayer.h"
#include "IPv4Layer.h"
#include "TcpLayer.h"
#include "HttpLayer.h"
#include "PcapFileDevice.h"

#include "HttpStatsCollector.h"

class AppHandler
{
private:
	static int updatePeriod;
	static int processTime;

	static std::string interfaceIPAddr;
	static pcpp::PcapLiveDevice *dev;
	static std::mutex collectorMutex;

	static constexpr struct option AppOptions[] =
		{
			{"interface", required_argument, 0, 'i'},
			{"rate-calc-period", required_argument, 0, 'r'},
			{"list-interfaces", no_argument, 0, 'l'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}};

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		std::lock_guard<std::mutex> guard(collectorMutex);

		// extract the stats object form the cookie
		HttpStatsCollector *stats = static_cast<HttpStatsCollector *>(cookie);
		auto parsedPacket = pcpp::Packet(packet);

		if (parsedPacket.isPacketOfType(pcpp::HTTP))
			stats->addPacket(pcpp::Packet(packet));
	}

public:
	static void printUsage() noexcept
	{
	}

	static void printInterfaces()
	{
		const std::vector<pcpp::PcapLiveDevice *> &devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

		std::cout << std::endl
				  << "Network interfaces:" << std::endl;

		for (auto iter = devList.begin(); iter != devList.end(); iter++)
		{
			std::cout << "    -> Name: '" << (*iter)->getName() << "'   IP address: " << (*iter)->getIPv4Address().toString() << std::endl;
		}
	}

	static bool initialize(int argc, char **argv)
	{
		int optionIndex{0}, opt{0};

		while ((opt = getopt_long(argc, argv, "i:r:hl", AppHandler::AppOptions, &optionIndex)) != -1)
		{
			switch (opt)
			{
			case 0:
				break;
			case 'i':
				interfaceIPAddr = optarg;
				break;
			case 'r':
				updatePeriod = atoi(optarg);
				break;
			case 'h':
				AppHandler::printUsage();
				return false;
				break;
			case 'l':
				AppHandler::printInterfaces();
				return false;
				break;
			default:
				AppHandler::printUsage();
				return false;
			}
		}

		dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
		if (dev == NULL)
		{
			std::cerr << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
			return false;
		}

		std::cout << "Interface info:" << std::endl
				  << "   Interface name:        " << dev->getName() << std::endl		   // get interface name
				  << "   Interface description: " << dev->getDesc() << std::endl		   // get interface description
				  << "   MAC address:           " << dev->getMacAddress() << std::endl	   // get interface MAC address
				  << "   Default gateway:       " << dev->getDefaultGateway() << std::endl // get default gateway
				  << "   Interface MTU:         " << dev->getMtu() << std::endl;		   // get interface MTU

		if (dev->getDnsServers().size() > 0)
			std::cout << "   DNS server:            " << dev->getDnsServers().at(0) << std::endl;

		if (!dev->open())
		{
			std::cerr << "Cannot open device" << std::endl;
			return false;
		}

		// create a filter instance to capture only HTTP traffic
		pcpp::ProtoFilter protocolFilter(pcpp::HTTP);
		dev->setFilter(protocolFilter);

		return true;
	}

	static void start()
	{
		std::cout << std::endl
				  << "Starting capture in async mode..." << std::endl;

		HttpStatsCollector stats;
		dev->startCapture(onPacketArrives, &stats);

		while (processTime > 0)
		{
			pcpp::multiPlatformSleep(std::min(updatePeriod, processTime));

			std::lock_guard<std::mutex> guard(collectorMutex);
			stats.print();
			stats.clear();

			processTime -= updatePeriod;
		}
	}

	static void terminate()
	{
		dev->stopCapture();
		dev->close();
	}
};

int AppHandler::updatePeriod{5};
int AppHandler::processTime{30};

std::mutex AppHandler::collectorMutex;
std::string AppHandler::interfaceIPAddr{"192.168.0.3"};
pcpp::PcapLiveDevice *AppHandler::dev;
