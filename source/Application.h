#pragma once
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/detail/config.hpp>

#include <served/served.hpp>

#include "PcapLiveDeviceList.h"
#include "SystemUtils.h"

#include "TrafficAnalyzer.h"

class Application
{
private:
	bool shouldStop{false};
	int updatePeriod;
	int executionTime;

	std::string interfaceIPAddr;

	TrafficAnalyzer *httpAnalyzer{nullptr};
	served::net::server *server{nullptr};

	static void onApplicationInterrupted(void *cookie)
	{
		Application *app = static_cast<Application *>(cookie);
		app->shouldStop = true;
	}

	void printInterfaces()
	{
		const std::vector<pcpp::PcapLiveDevice *> &devList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

		printf("Network interfaces:\n");

		for (auto iter = devList.begin(); iter != devList.end(); iter++)
			printf("    -> Name: '%-20s IP address: %s\n",
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
			logging::keywords::file_name = "../logs/" + logDate.str() + ".log",
			logging::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
		logging::add_common_attributes();

		BOOST_LOG_TRIVIAL(info) << "Logger setuped.";
	}

	bool parseComandLine(int argc, char **argv)
	{
		interfaceIPAddr = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList()[0]->getIPv4Address().toString();

		namespace po = boost::program_options;

		po::variables_map vm;
		po::options_description description("Allowed Options");

		// declare options
		description.add_options()("help,h", "Produce help message.")("list-interfaces,l", "Print the list of interfaces.")("ip,i", po::value<std::string>()->default_value(interfaceIPAddr), "Use the specified interface.")("exe-time,t", po::value<int>()->default_value(std::numeric_limits<int>::max()), "Program execution time (in sec).")("update-time,u", po::value<int>()->default_value(5), "Terminal update frequency (in sec).");

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
								 << "\texecutionTime:   " << executionTime << std::endl
								 << "\tupdatePeriod:    " << updatePeriod << std::endl;

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

public:
	int run(int argc, char **argv)
	{
		setupLogger();

		if (!parseComandLine(argc, argv))
			return 0;

		pcpp::ApplicationEventHandler::getInstance().onApplicationInterrupted(onApplicationInterrupted, this);

		try
		{
			httpAnalyzer = new TrafficAnalyzer(interfaceIPAddr, pcpp::HTTP);
		}
		catch (std::exception &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Cannot create httpAnalyzer. Exception: " << e.what() << std::endl;
			return -1;
		}

		served::multiplexer mux;
		mux.handle("/stat").get(
			[&](served::response &res, const served::request &req)
			{
				res.set_header("content-type", "application/json");
				BOOST_LOG_TRIVIAL(debug) << "server " << std::endl;
				res << httpAnalyzer->getJsonStat();
			});

		server = new served::net::server("127.0.0.1", "8080", mux, false);
		server->run(2, false);

		std::cout << "Use this to get statistics in JSON format: curl \"http://localhost:8080/stat\"" << std::endl;

		httpAnalyzer->startCapture();

		while (!shouldStop && executionTime > 0)
		{
			pcpp::multiPlatformSleep(std::min(updatePeriod, executionTime));
			printf("%s", httpAnalyzer->getPlaneTextStat().c_str());
			printf("--------------------------------------------------------------------------------------------------------------------------------\n");
			executionTime -= updatePeriod;
		}

		server->stop();
		delete server;

		httpAnalyzer->stopCapture();
		printf("-------------------------------------------------------------RESULTS------------------------------------------------------------\n");
		printf("%s", httpAnalyzer->getPlaneTextStat().c_str());
		printf("-----------------------------------------------------------JSON-RESULTS---------------------------------------------------------\n");
		printf("%s", httpAnalyzer->getJsonStat().c_str());

		delete httpAnalyzer;

		return 0;
	}
};