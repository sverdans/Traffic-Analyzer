#include <iostream>
#include <vector>
#include <string>
#include <map>

#include "Packet.h"
#include "HttpLayer.h"
#include "TcpLayer.h"
#include "IPv4Layer.h"
#include "DnsResourceData.h"

class HostInfo
{
private:
	int inPackets;
	int outPackets;
	int inTraffic;
	int outTraffic;

public:
};

class HttpStatsCollector
{
private:
	std::map<std::string, HostInfo> totalStat;
	std::map<std::string, HostInfo> currentStat;

public:
	void clear() { currentStat.clear(); }

	void print()
	{
	}

	void addPacket(const pcpp::Packet &packet)
	{
		auto *tcpLayer = packet.getLayerOfType<pcpp::TcpLayer>();
		auto *ipLayer = packet.getLayerOfType<pcpp::IPv4Layer>();

		std::cout << ipLayer->getSrcIPAddress() << "\t" << ipLayer->getDstIPAddress() << std::endl;

		int size = tcpLayer->getLayerPayloadSize();

		std::cout << size << " [ bytes ]" << std::endl;

		if (auto *httpRequestLayer = packet.getLayerOfType<pcpp::HttpRequestLayer>())
		{
			pcpp::HeaderField *hostField = httpRequestLayer->getFieldByName(PCPP_HTTP_HOST_FIELD);
			if (hostField != NULL)
				std::cout << hostField->getFieldValue() << std::endl;
		}
		else if (auto *httpResponseLayer = packet.getLayerOfType<pcpp::HttpResponseLayer>())
		{
			pcpp::HeaderField *refererField = httpResponseLayer->getFieldByName(PCPP_HTTP_SERVER_FIELD);
			if (refererField != NULL)
				std::cout << refererField->getFieldValue() << std::endl;
		}
		std::cout << "_______________________" << std::endl;
	}
};