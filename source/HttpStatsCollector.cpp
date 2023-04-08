#include "HttpStatsCollector.h"

#include "pcapplusplus/Packet.h"
#include "pcapplusplus/HttpLayer.h"
#include "pcapplusplus/TcpLayer.h"
#include "pcapplusplus/IPv4Layer.h"
#include "pcapplusplus/DnsResourceData.h"

HttpStatsCollector::HttpStatsCollector(const std::string &userIp) : userIp(userIp) {}
HttpStatsCollector::~HttpStatsCollector()
{
	totalStat.clear();
	userIp.clear();
}

HttpStatsCollector::HttpStatsCollector(const HttpStatsCollector &other) noexcept
	: userIp(other.userIp), totalStat(other.totalStat) {}

HttpStatsCollector &HttpStatsCollector::operator=(const HttpStatsCollector &other) noexcept
{
	userIp = other.userIp;
	totalStat = other.totalStat;
}

HttpStatsCollector::HttpStatsCollector(HttpStatsCollector &&other) noexcept
	: userIp(other.userIp), totalStat(other.totalStat)
{
	other.totalStat.clear();
	other.userIp.clear();
}

HttpStatsCollector &HttpStatsCollector::operator=(HttpStatsCollector &&other) noexcept
{
	userIp = other.userIp;
	totalStat = other.totalStat;

	other.totalStat.clear();
	other.userIp.clear();
}

void HttpStatsCollector::print()
{
	//	25 | 7 | 5 | 5 | 7 | 6 | 6 |
	//	printf(" host                      ║ packets │ OUT   │ IN    ║ traffic │ OUT     │ IN      \n");

	for (int i = 0; i < printedLinesCount; ++i)
	{
		printf("\033[1A");
		printf("\033[K");
	}

	printedLinesCount = 0;

	for (const auto &[ip, hostInfo] : totalStat)
	{
		printf("%2d ", printedLinesCount);
		printf("%-30s %6d packets ( OUT %6d / %-6d IN ) traffic: %8d [bytes] ( OUT %6d / %-6d IN )\n",
			   hostInfo.name.c_str(),
			   hostInfo.inPackets + hostInfo.outPackets,
			   hostInfo.outPackets,
			   hostInfo.inPackets,
			   hostInfo.inTraffic + hostInfo.outTraffic,
			   hostInfo.outTraffic,
			   hostInfo.inTraffic);
		printedLinesCount++;
	}
}

void HttpStatsCollector::addPacket(const pcpp::Packet &packet)
{
	auto *tcpLayer = packet.getLayerOfType<pcpp::TcpLayer>();
	auto *ipLayer = packet.getLayerOfType<pcpp::IPv4Layer>();
	int size = tcpLayer->getLayerPayloadSize();

	if (ipLayer->getSrcIPAddress() == userIp)
	{
		auto hostIp = ipLayer->getDstIPAddress().toString();
		auto &hostInfo = totalStat[hostIp];
		hostInfo.addOutPacket(size);
		if (hostInfo.name.empty())
			if (auto *httpRequestLayer = packet.getLayerOfType<pcpp::HttpRequestLayer>())
			{
				pcpp::HeaderField *hostField = httpRequestLayer->getFieldByName(PCPP_HTTP_HOST_FIELD);
				if (hostField != NULL)
					hostInfo.name = hostField->getFieldValue();
			}
	}
	else
	{
		auto hostIp = ipLayer->getSrcIPAddress().toString();
		totalStat[hostIp].addInPacket(size);
		if (totalStat[hostIp].name.empty())
			std::cout << "HOST IP EMPTY!!!" << std::endl;
	}

	/*
		std::cout << ipLayer->getSrcIPAddress() << "\t" << ipLayer->getDstIPAddress() << std::endl;

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
	*/
}