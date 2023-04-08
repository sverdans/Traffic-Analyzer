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
	return *this;
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
	return *this;
}

void HttpStatsCollector::print()
{

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
	//	printf("----------------------------------------------------------\n");
}

void HttpStatsCollector::addPacket_old(const pcpp::Packet &packet)
{
	if (!packet.isPacketOfType(pcpp::HTTP))
		return;

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
	}
}

void HttpStatsCollector::addPacket(const pcpp::Packet &packet)
{
}