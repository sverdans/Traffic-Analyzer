#include <iostream>
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
#include "HttpTrafficStats.h"

struct ProgramOptions
{
	bool shouldClose{false};
	int updatePeriod;
	int executionTime;
	std::string interfaceIPAddr;
};

void onApplicationInterrupted(void *cookie)
{
	bool *shouldStop = static_cast<bool *>(cookie);
	*shouldStop = true;
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
		logging::keywords::file_name = "../logs/" + logDate.str() + "[%N].log",
		logging::keywords::rotation_size = 1 * 1024 * 1024,
		logging::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
	logging::add_common_attributes();

	BOOST_LOG_TRIVIAL(info) << "Logger setuped.";
}

bool parseComandLine(int argc, char **argv, ProgramOptions &options)
{
	options.interfaceIPAddr = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList()[0]->getIPv4Address().toString();

	namespace po = boost::program_options;

	po::variables_map vm;
	po::options_description description("Allowed Options");

	description.add_options()("help,h", "Produce help message.")("list-interfaces,l", "Print the list of interfaces.")("ip,i", po::value<std::string>()->default_value(options.interfaceIPAddr), "Use the specified interface.")("exe-time,t", po::value<int>()->default_value(std::numeric_limits<int>::max()), "Program execution time (in sec).")("update-time,u", po::value<int>()->default_value(5), "Terminal update frequency (in sec).");

	try
	{
		po::store(po::parse_command_line(argc, argv, description), vm);
		po::notify(vm);
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(fatal) << "Exception in comand line parsing: " << e.what();
		return false;
	}

	if (vm.count("help"))
	{
		std::cout << description << std::endl;
		options.shouldClose = true;
	}

	if (vm.count("list-interfaces"))
	{
		printInterfaces();
		options.shouldClose = true;
	}

	options.executionTime = vm["exe-time"].as<int>();
	options.updatePeriod = vm["update-time"].as<int>();
	options.interfaceIPAddr = vm["ip"].as<std::string>();

	BOOST_LOG_TRIVIAL(debug) << "AppHandler initial state: " << std::endl
							 << "\tinterfaceIpAddr: " << options.interfaceIPAddr << std::endl
							 << "\texecutionTime:   " << options.executionTime << std::endl
							 << "\tupdatePeriod:    " << options.updatePeriod << std::endl;

	if (options.executionTime < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "executionTime was negative.";
		return false;
	}
	if (options.updatePeriod < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "updatePeriod was negative.";
		return false;
	}

	return true;
}

int main(int argc, char **argv)
{
	setupLogger();

	ProgramOptions options;

	if (!parseComandLine(argc, argv, options))
		return 1;

	if (options.shouldClose)
		return 0;

	pcpp::ApplicationEventHandler::getInstance().onApplicationInterrupted(onApplicationInterrupted, &options.shouldClose);

	TrafficAnalyzer<HttpTrafficStats> *httpAnalyzer{nullptr};

	try
	{
		httpAnalyzer = new TrafficAnalyzer<HttpTrafficStats>(options.interfaceIPAddr, pcpp::HTTP);
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(fatal) << "Cannot create httpAnalyzer. Exception: " << e.what() << std::endl;
		delete httpAnalyzer;
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

	auto server = served::net::server("127.0.0.1", "8080", mux, false);
	server.run(2, false);

	printf("Use this to get statistics in JSON format: curl \"http://localhost:8080/stat\"\n");

	httpAnalyzer->startCapture();

	while (!options.shouldClose && options.executionTime > 0)
	{
		pcpp::multiPlatformSleep(std::min(options.updatePeriod, options.executionTime));
		printf("%s", httpAnalyzer->getPlaneTextStat().c_str());
		printf("--------------------------------------------------------------------------------------------------------------------------------\n");
		options.executionTime -= options.updatePeriod;
	}

	server.stop();
	httpAnalyzer->stopCapture();

	printf("-------------------------------------------------------------RESULTS------------------------------------------------------------\n");
	printf("%s", httpAnalyzer->getPlaneTextStat().c_str());
	printf("-----------------------------------------------------------JSON-RESULTS---------------------------------------------------------\n");
	printf("%s", httpAnalyzer->getJsonStat().c_str());

	delete httpAnalyzer;

	return 0;
}
