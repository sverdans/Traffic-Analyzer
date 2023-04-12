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

#include "PcapLiveDeviceList.h"
#include "PayloadLayer.h"
#include "PacketUtils.h"
#include "SystemUtils.h"
#include "Packet.h"

#include "HttpStatsCollector.h"

class AppHandler
{
private:
	static int updatePeriod;
	static int executionTime;

	static std::string interfaceIPAddr;
	static pcpp::PcapLiveDevice *dev;
	static std::mutex collectorMutex;

	static void onApplicationInterrupted(void *cookie)
	{
		bool *shouldStop = (bool *)cookie;
		*shouldStop = true;
	}

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		if (!packet)
			std::cout << "packet = nullptr";

		std::lock_guard<std::mutex> guard(collectorMutex);

		// extract the stats object form the cookie
		HttpStatsCollector *stats = static_cast<HttpStatsCollector *>(cookie);

		stats->addPacket(pcpp::Packet(packet));
	}

	static void printInterfaces()
	{
		const std::vector<pcpp::PcapLiveDevice *> &devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

		printf("Network interfaces:\n");

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
		description.add_options()("help,h", "Produce help message.")("list-interfaces,l", "Print the list of interfaces.")("ip,i", po::value<std::string>()->default_value(interfaceIPAddr), "Use the specified interface.")("update-time,u", po::value<int>()->default_value(5), "Terminal update frequency (in sec).")("exe-time,t", po::value<int>()->default_value(std::numeric_limits<int>::max()), "Program execution time (in sec).");

		try
		{
			// parse arguments
			po::store(po::parse_command_line(argc, argv, description), vm);
			po::notify(vm);
		}
		catch (std::exception &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Exception in comand line parsing: " << e.what();
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

		executionTime = vm["exe-time"].as<int>();
		updatePeriod = vm["update-time"].as<int>();
		interfaceIPAddr = vm["ip"].as<std::string>();

		BOOST_LOG_TRIVIAL(debug) << "AppHandler initial state: " << std::endl
								 << "\tinterfaceIpAddr: " << interfaceIPAddr << std::endl
								 << "\tupdatePeriod:    " << updatePeriod << std::endl
								 << "\texecutionTime:     " << executionTime << std::endl;

		if (executionTime < 0)
		{
			BOOST_LOG_TRIVIAL(error) << "executionTime was negative.";
			return false;
		}
		if (updatePeriod < 0)
		{
			BOOST_LOG_TRIVIAL(error) << "updatePeriod was negative.";
			return false;
		}

		return true;
	}

	static bool initialize()
	{
		dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
		if (!dev)
		{
			BOOST_LOG_TRIVIAL(error) << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
			return false;
		}

		BOOST_LOG_TRIVIAL(info) << "Interface info:" << std::endl
								<< "\tInterface name:        " << dev->getName() << std::endl					   // get interface name
								<< "\tInterface description: " << dev->getDesc() << std::endl					   // get interface description
								<< "\tMAC address:           " << dev->getMacAddress().toString() << std::endl	   // get interface MAC address
								<< "\tDefault gateway:       " << dev->getDefaultGateway().toString() << std::endl // get default gateway
								<< "\tInterface MTU:         " << dev->getMtu() << std::endl;					   // get interface MTU

		if (!dev->open())
		{
			BOOST_LOG_TRIVIAL(error) << "Cannot open device" << std::endl;
			return false;
		}

		pcpp::ProtoFilter filter(pcpp::HTTP);

		dev->setFilter(filter);

		return true;
	}

	static void start()
	{
		BOOST_LOG_TRIVIAL(info) << "Starting capture in async mode" << std::endl;

		HttpStatsCollector stats(interfaceIPAddr);
		dev->startCapture(onPacketArrives, &stats);

		bool shouldStop = false;
		pcpp::ApplicationEventHandler::getInstance().onApplicationInterrupted(onApplicationInterrupted, &shouldStop);

		while (!shouldStop && executionTime > 0)
		{
			pcpp::multiPlatformSleep(std::min(updatePeriod, executionTime));

			std::lock_guard<std::mutex> guard(collectorMutex);
			stats.print();
			executionTime -= updatePeriod;
		}

		BOOST_LOG_TRIVIAL(info) << "Capturing stoped" << std::endl;

		dev->stopCapture();
		dev->close();

		stats.printWithStat();
	}
};

int AppHandler::updatePeriod;
int AppHandler::executionTime;

std::mutex AppHandler::collectorMutex;
std::string AppHandler::interfaceIPAddr{"192.168.0.3"};
pcpp::PcapLiveDevice *AppHandler::dev;
