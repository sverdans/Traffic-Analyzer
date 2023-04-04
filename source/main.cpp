#include <iostream>
#include <map>

#include <getopt.h>

#include "stdlib.h"

#include "PcapLiveDeviceList.h"
#include "HttpLayer.h"
#include "TcpLayer.h"
#include "IPv4Layer.h"
#include "PayloadLayer.h"
#include "PacketUtils.h"
#include "SystemUtils.h"

#include "AppHandler.h"

struct PacketStats
{
	int ethPacketCount;
	int ipv4PacketCount;
	int ipv6PacketCount;
	int tcpPacketCount;
	int udpPacketCount;
	int dnsPacketCount;
	int httpPacketCount;
	int sslPacketCount;

	void clear()
	{
		ethPacketCount = 0;
		ipv4PacketCount = 0;
		ipv6PacketCount = 0;
		tcpPacketCount = 0;
		udpPacketCount = 0;
		tcpPacketCount = 0;
		dnsPacketCount = 0;
		httpPacketCount = 0;
		sslPacketCount = 0;
	}

	PacketStats() { clear(); }

	void consumePacket(pcpp::Packet &packet)
	{
		if (packet.isPacketOfType(pcpp::Ethernet))
			ethPacketCount++;
		if (packet.isPacketOfType(pcpp::IPv4))
			ipv4PacketCount++;
		if (packet.isPacketOfType(pcpp::IPv6))
			ipv6PacketCount++;
		if (packet.isPacketOfType(pcpp::TCP))
			tcpPacketCount++;
		if (packet.isPacketOfType(pcpp::UDP))
			udpPacketCount++;
		if (packet.isPacketOfType(pcpp::DNS))
			dnsPacketCount++;
		if (packet.isPacketOfType(pcpp::HTTP))
			httpPacketCount++;
		if (packet.isPacketOfType(pcpp::SSL))
			sslPacketCount++;
	}

	void printToConsole()
	{
		std::cout
			<< "Ethernet packet count: " << ethPacketCount << std::endl
			<< "IPv4 packet count:     " << ipv4PacketCount << std::endl
			<< "IPv6 packet count:     " << ipv6PacketCount << std::endl
			<< "TCP packet count:      " << tcpPacketCount << std::endl
			<< "UDP packet count:      " << udpPacketCount << std::endl
			<< "DNS packet count:      " << dnsPacketCount << std::endl
			<< "HTTP packet count:     " << httpPacketCount << std::endl
			<< "SSL packet count:      " << sslPacketCount << std::endl;
	}
};

struct Rate
{
	double currentRate; // periodic rate
	double totalRate;	// overlal rate

	void clear()
	{
		currentRate = 0;
		totalRate = 0;
	}
};

struct HttpGeneralStats
{
	int numOfHttpFlows;							// total number of HTTP flows
	Rate httpFlowRate;							// rate of HTTP flows
	int numOfHttpPipeliningFlows;				// total number of HTTP flows that contains at least on HTTP pipelining transaction
	int numOfHttpTransactions;					// total number of HTTP transactions
	Rate httpTransactionsRate;					// rate of HTTP transactions
	double averageNumOfHttpTransactionsPerFlow; // average number of HTTP transactions per flow
	int numOfHttpPackets;						// total number of HTTP packets
	Rate httpPacketRate;						// rate of HTTP packets
	double averageNumOfPacketsPerFlow;			// average number of HTTP packets per flow
	int amountOfHttpTraffic;					// total HTTP traffic in bytes
	double averageAmountOfDataPerFlow;			// average number of HTTP traffic per flow
	Rate httpTrafficRate;						// rate of HTTP traffic
	double sampleTime;							// total stats collection time

	void clear()
	{
		numOfHttpFlows = 0;
		httpFlowRate.clear();
		numOfHttpPipeliningFlows = 0;
		numOfHttpTransactions = 0;
		httpTransactionsRate.clear();
		averageNumOfHttpTransactionsPerFlow = 0;
		numOfHttpPackets = 0;
		httpPacketRate.clear();
		averageNumOfPacketsPerFlow = 0;
		amountOfHttpTraffic = 0;
		averageAmountOfDataPerFlow = 0;
		httpTrafficRate.clear();
		sampleTime = 0;
	}
};

struct HttpMessageStats
{
	int numOfMessages;				 // total number of HTTP messages of that type (request/response)
	Rate messageRate;				 // rate of HTTP messages of that type
	int totalMessageHeaderSize;		 // total size (in bytes) of data in headers
	double averageMessageHeaderSize; // average header size

	virtual ~HttpMessageStats() {}

	virtual void clear()
	{
		numOfMessages = 0;
		messageRate.clear();
		totalMessageHeaderSize = 0;
		averageMessageHeaderSize = 0;
	}
};

struct HttpRequestStats : HttpMessageStats
{
	std::map<pcpp::HttpRequestLayer::HttpMethod, int> methodCount; // a map for counting the different HTTP methods seen in traffic
	std::map<std::string, int> hostnameCount;					   // a map for counting the hostnames seen in traffic

	void clear()
	{
		HttpMessageStats::clear();
		methodCount.clear();
		hostnameCount.clear();
	}
};

struct HttpResponseStats : HttpMessageStats
{
	std::map<std::string, int> statusCodeCount;	 // a map for counting the different status codes seen in traffic
	std::map<std::string, int> contentTypeCount; // a map for counting the content-types seen in traffic
	int numOfMessagesWithContentLength;			 // total number of responses containing the "content-length" field
	int totalContentLengthSize;					 // total body size extracted by responses containing "content-length" field
	double averageContentLengthSize;			 // average body size

	void clear()
	{
		HttpMessageStats::clear();
		numOfMessagesWithContentLength = 0;
		totalContentLengthSize = 0;
		averageContentLengthSize = 0;
		statusCodeCount.clear();
		contentTypeCount.clear();
	}
};

static bool onPacketArrivesBlockingMode(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
{
	// extract the stats object form the cookie
	PacketStats *stats = (PacketStats *)cookie;

	// parsed the raw packet
	pcpp::Packet parsedPacket(packet);

	// collect stats from packet
	stats->consumePacket(parsedPacket);

	// return false means we don't want to stop capturing after this callback
	return false;
}

static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
{
	// extract the stats object form the cookie
	PacketStats *stats = (PacketStats *)cookie;

	// parsed the raw packet
	pcpp::Packet parsedPacket(packet);

	// collect stats from packet
	stats->consumePacket(parsedPacket);
}

static struct option HttpAnalyzerOptions[] = {
	{"interface", required_argument, 0, 'i'},
	{"rate-calc-period", required_argument, 0, 'r'},
	{"list-interfaces", no_argument, 0, 'l'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}};

int main(int argc, char **argv)
{
	std::string interfaceIPAddr = "192.168.0.3";
	int printRatePeriod = 5;

	int optionIndex = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "i:r:hl", HttpAnalyzerOptions, &optionIndex)) != -1)
	{
		switch (opt)
		{
		case 0:
			break;
		case 'i':
			interfaceIPAddr = optarg;
			break;
		case 'r':
			printRatePeriod = atoi(optarg);
			break;
		case 'h':
			AppHandler::printUsage();
			exit(0);
			break;
		case 'l':
			AppHandler::listInterfaces();
			exit(0);
			break;
		default:
			AppHandler::printUsage();
			exit(-1);
		}
	}
	std::cout << "print rate period: " << printRatePeriod << std::endl;

	pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
	if (dev == NULL)
	{
		std::cerr << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
		return 1;
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
		return 1;
	}

	// create a filter instance to capture only HTTP traffic
	pcpp::ProtoFilter protocolFilter(pcpp::HTTP);
	dev->setFilter(protocolFilter);

	PacketStats stats;

	std::cout << std::endl
			  << "Starting capture in block mode..." << std::endl;

	dev->startCaptureBlockingMode(onPacketArrivesBlockingMode, &stats, printRatePeriod);

	std::cout << "Results:" << std::endl;
	stats.printToConsole();

	dev->close();

	return 0;
}
