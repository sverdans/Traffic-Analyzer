#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/detail/config.hpp>

#include "PcapLiveDeviceList.h"
#include "SystemUtils.h"

namespace app
{
	/**
	 *	\brief Структура, в которой хранятся аргументы запуска программы
	 *
	 */
	struct ProgramOptions
	{
		bool shouldClose{false};
		int updatePeriod{5};
		int executionTime{60};
		std::string interfaceIPAddr{"127.0.0.1"};
	};

	void onApplicationInterrupted(void *cookie)
	{
		bool *shouldStop = static_cast<bool *>(cookie);
		BOOST_LOG_TRIVIAL(info) << "onApplicationInterrupted handled";
		*shouldStop = true;
	}

	void printInterfaces()
	{
		const std::vector<pcpp::PcapLiveDevice *> &devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

		printf("Network interfaces:\n");

		for (auto iter = devList.begin(); iter != devList.end(); iter++)
			printf("  -> Name: '%-25s IP address: %s\n",
				   ((*iter)->getName() + "'").c_str(),
				   (*iter)->getIPv4Address().toString().c_str());
	}

	void setupLogger()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::stringstream logDate;
		logDate << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d.%X");
		namespace logging = boost::log;

		logging::add_file_log(
			logging::keywords::file_name = "../logs/" + logDate.str() + "[%N].log",
			logging::keywords::rotation_size = 1 * 1024 * 1024,
			logging::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
		logging::add_common_attributes();
	}

	ProgramOptions parseComandLine(int argc, char **argv)
	{
		bool shouldClose = false;
		std::string interfaceIPAddr = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList()[0]->getIPv4Address().toString();

		namespace po = boost::program_options;

		po::variables_map vm;
		po::options_description description("Allowed Options");

		description.add_options()("help,h", "Produce help message.")("list-interfaces,l", "Print the list of interfaces.")("ip,i", po::value<std::string>()->default_value(interfaceIPAddr), "Use the specified interface.")("exe-time,t", po::value<int>()->default_value(std::numeric_limits<int>::max()), "Program execution time (in sec).")("update-time,u", po::value<int>()->default_value(5), "Terminal update frequency (in sec).");

		po::store(po::parse_command_line(argc, argv, description), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			std::cout << description << std::endl;
			shouldClose = true;
		}

		if (vm.count("list-interfaces"))
		{
			printInterfaces();
			shouldClose = true;
		}

		int executionTime = vm["exe-time"].as<int>();
		int updatePeriod = vm["update-time"].as<int>();
		interfaceIPAddr = vm["ip"].as<std::string>();

		if (executionTime < 0)
			throw std::runtime_error("executionTime was negative.");

		if (updatePeriod < 0)
			throw std::runtime_error("updatePeriod was negative.");

		return {shouldClose, updatePeriod, executionTime, interfaceIPAddr};
	}
}
