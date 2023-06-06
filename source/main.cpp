#include <iostream>
#include <vector>
#include <algorithm>

#include <boost/log/trivial.hpp>
#include <served/served.hpp>

#include <SystemUtils.h>

#include <App.h>
#include <TrafficAnalyzer.h>
#include <HttpTrafficStats.h>

int main(int argc, char **argv)
{
	app::setupLogger();

	app::ProgramOptions options;

	try
	{
		options = app::parseComandLine(argc, argv);
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(fatal) << "parseComandLine exception: " << e.what();
		return -1;
	}

	if (options.shouldClose)
		return 0;

	BOOST_LOG_TRIVIAL(debug) << "App initial state: "
							 << "{ interfaceIpAddr: " << options.interfaceIpAddr << ", "
							 << "executionTime: " << options.executionTime << ", "
							 << "updatePeriod: " << options.updatePeriod << " }";

	pcpp::ApplicationEventHandler::getInstance().onApplicationInterrupted(app::onApplicationInterrupted, &options.shouldClose);

	TrafficAnalyzer httpAnalyzer;

	std::vector<pcpp::GeneralFilter *> portFilterVec = {
		new pcpp::PortFilter(80, pcpp::SRC_OR_DST),
		new pcpp::PortFilter(443, pcpp::SRC_OR_DST)};

	std::string httpAnalyzerInitInfo;
	if (!httpAnalyzer.initializeAs<HttpTrafficStats>(options.interfaceIpAddr, portFilterVec, httpAnalyzerInitInfo))
	{
		BOOST_LOG_TRIVIAL(fatal) << "TrafficAnalyzer initialization error: " << httpAnalyzerInitInfo << std::endl;
		for (auto it : portFilterVec)
			delete it;

		httpAnalyzer.finalize();
		return -1;
	}

	served::multiplexer mux;
	mux.handle("/stat").get(
		[&httpAnalyzer](served::response &res, const served::request &req)
		{
			res.set_header("content-type", "application/json");
			BOOST_LOG_TRIVIAL(debug) << "Server received a request GET /stat" << std::endl;
			res << httpAnalyzer.getJsonStat();
		});

	auto server = served::net::server("127.0.0.1", "8080", mux, false);
	server.run(2, false);

	printf("Use this to get statistics in JSON format: curl \"http://localhost:8080/stat\"\n");

	httpAnalyzer.startCapture();

	while (!options.shouldClose && options.executionTime > 0)
	{
		pcpp::multiPlatformSleep(std::min(options.updatePeriod, options.executionTime));
		printf("%s", httpAnalyzer.getPlaneTextStat().c_str());
		printf("----------------------------------------------------------------------------------------------------------------------------------\n");
		options.executionTime -= options.updatePeriod;
	}

	server.stop();
	httpAnalyzer.stopCapture();

	printf("--------------------------------------------------------------RESULTS-------------------------------------------------------------\n");
	printf("%s", httpAnalyzer.getPlaneTextStat().c_str());
	printf("-----------------------------------------------------------JSON-RESULTS-----------------------------------------------------------\n");
	printf("%s", httpAnalyzer.getJsonStat().c_str());

	httpAnalyzer.finalize();

	for (auto it : portFilterVec)
		delete it;

	return 0;
}
