#pragma once
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <chrono>
#include <mutex>

#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#include "pcapplusplus/PcapLiveDeviceList.h"
#include "pcapplusplus/HttpLayer.h"
#include "pcapplusplus/TcpLayer.h"
#include "pcapplusplus/IPv4Layer.h"
#include "pcapplusplus/PayloadLayer.h"
#include "pcapplusplus/PacketUtils.h"
#include "pcapplusplus/SystemUtils.h"

#include "pcapplusplus/Packet.h"
#include "pcapplusplus/EthLayer.h"
#include "pcapplusplus/IPv4Layer.h"
#include "pcapplusplus/TcpLayer.h"
#include "pcapplusplus/HttpLayer.h"
#include "pcapplusplus/PcapFileDevice.h"

#include "HttpStatsCollector.h"

class AppHandler
{
private:
	static int updatePeriod;
	static int processTime;

	static std::string interfaceIPAddr;
	static pcpp::PcapLiveDevice *dev;
	static std::mutex collectorMutex;

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		std::lock_guard<std::mutex> guard(collectorMutex);

		// extract the stats object form the cookie
		HttpStatsCollector *stats = static_cast<HttpStatsCollector *>(cookie);

		//	stats->addPacket_old(pcpp::Packet(packet));
		stats->addPacket_old(pcpp::Packet(packet));
	}

	static void printInterfaces()
	{
		const std::vector<pcpp::PcapLiveDevice *> &devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

		std::cout << "Network interfaces:" << std::endl;

		for (auto iter = devList.begin(); iter != devList.end(); iter++)
			printf("    -> Name: '%-20s IP address: %s\n",
				   ((*iter)->getName() + "'").c_str(),
				   (*iter)->getIPv4Address().toString().c_str());
	}

public:
	static void setupLogger()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::stringstream logDate;
		logDate << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d.%X");
		std::cout << logDate.str() << std::endl;
		namespace logging = boost::log;

		logging::add_file_log(
			logging::keywords::file_name = "../logs/" + logDate.str() + ".log",
			logging::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
		logging::add_common_attributes();

		BOOST_LOG_TRIVIAL(info) << "Logger setuped.";
	}

	static bool parseComandLine(int argc, char **argv)
	{
		namespace po = boost::program_options;

		po::variables_map vm;
		po::options_description description("Allowed Options");

		// declare options
		description.add_options()("help,h", "produce help message")("list-interfaces,l", "Print the list of interfaces.")("ip,i", po::value<std::string>()->default_value(interfaceIPAddr), "Use the specified interface.")("update-time,u", po::value<int>()->default_value(5), "Terminal update frequency.")("process-time,t", po::value<int>()->default_value(60), "Program execution time.");

		try
		{
			// parse arguments
			po::store(po::parse_command_line(argc, argv, description), vm);
			po::notify(vm);
		}
		catch (std::exception &e)
		{
			std::cout << "Error: " << e.what() << std::endl;
			std::cout << description << std::endl;
			return false;
		}

		if (vm.count("help"))
		{
			std::cout << description << std::endl;
			return false;
		}

		if (vm.count("list-interfaces"))
		{
			printInterfaces();
			return false;
		}

		processTime = vm["process-time"].as<int>();
		updatePeriod = vm["update-time"].as<int>();
		interfaceIPAddr = vm["ip"].as<std::string>();

		if (processTime < 0 || updatePeriod < 0)
			return false;

		return true;
	}

	static bool initialize()
	{
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
		//	pcpp::ProtoFilter protocolFilter(pcpp::HTTP);
		//	dev->setFilter(protocolFilter);

		pcpp::PortFilter httpPortFilter(80, pcpp::SRC_OR_DST);
		dev->setFilter(httpPortFilter);

		return true;
	}

	static void start()
	{
		std::cout << std::endl
				  << "Starting capture in async mode..." << std::endl;

		HttpStatsCollector stats(interfaceIPAddr);
		dev->startCapture(onPacketArrives, &stats);

		while (processTime > 0)
		{
			pcpp::multiPlatformSleep(std::min(updatePeriod, processTime));

			std::lock_guard<std::mutex> guard(collectorMutex);
			stats.print();
			processTime -= updatePeriod;
		}
	}

	static void terminate()
	{
		dev->stopCapture();
		dev->close();
	}
};

int AppHandler::updatePeriod{3};
int AppHandler::processTime{50};

std::mutex AppHandler::collectorMutex;
std::string AppHandler::interfaceIPAddr{"192.168.0.3"};
pcpp::PcapLiveDevice *AppHandler::dev;
