#include "HttpStatsCollector.h"

#include <iomanip>
#include <boost/log/trivial.hpp>

#include "Packet.h"
#include "HttpLayer.h"
#include "SSLLayer.h"
#include "TcpLayer.h"
#include "UdpLayer.h"
#include "IPv4Layer.h"
#include "DnsResourceData.h"

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
	for (const auto &[ip, hostInfo] : totalStat)
	{
		printf("%-37s %6u packets (OUT %-6u | %6u IN) traffic: %8u [bytes] (OUT %-6u | %6u IN)\n",
			   (hostInfo.name.empty() ? ip.c_str() : hostInfo.name.c_str()),
			   hostInfo.inPackets + hostInfo.outPackets,
			   hostInfo.outPackets,
			   hostInfo.inPackets,
			   hostInfo.inTraffic + hostInfo.outTraffic,
			   hostInfo.outTraffic,
			   hostInfo.inTraffic);
	}
	printf("--------------------------------------------------------------------------------------------------------------------------------\n");
}

void HttpStatsCollector::printWithStat()
{
	printf("-------------------------------------------------------------RESULTS------------------------------------------------------------\n");
	print();
	unsigned int totalPacketsInCount{0}, totalPacketsOutCount{0};
	unsigned int totalTrafficIn{0}, totalTrafficOut{0};

	for (const auto &[ip, hostInfo] : totalStat)
	{
		totalPacketsInCount += hostInfo.inPackets;
		totalPacketsOutCount += hostInfo.outPackets;

		totalTrafficIn += hostInfo.inTraffic;
		totalTrafficOut += hostInfo.outTraffic;
	}
	printf("Total count of captured packets: %u\n", totalPacketsInCount + totalPacketsOutCount);
	printf("Total count of captured incoming packets: %u\n", totalPacketsInCount);
	printf("Total count of captured outgoing packets: %u\n", totalPacketsOutCount);

	printf("Total traffic of captured packets: %u\n", totalTrafficIn + totalTrafficOut);
	printf("Total traffic of captured incoming packets: %u\n", totalTrafficIn);
	printf("Total traffic of captured outgoing packets: %u\n", totalTrafficOut);
}

void HttpStatsCollector::addPacket(const pcpp::Packet &packet)
{
	auto *ipLayer = packet.getLayerOfType<pcpp::IPv4Layer>();

	if (!ipLayer)
	{
		BOOST_LOG_TRIVIAL(warning) << "IPLayer was nullptr";
		return;
	}

	int size = packet.getRawPacket()->getRawDataLen();

	auto srcIp = ipLayer->getSrcIPAddress().toString();
	auto dstIp = ipLayer->getDstIPAddress().toString();

	BOOST_LOG_TRIVIAL(debug) << "Captured packet {"
							 << " srcIP: " << std::left << std::setw(15) << srcIp
							 << " dstIP: " << std::left << std::setw(15) << dstIp
							 << " size: " << std::left << std::setw(9) << size << " }";

	bool isInPacket = dstIp == userIp;
	auto &hostInfo = isInPacket ? totalStat[srcIp] : totalStat[dstIp];

	hostInfo.addPacket(size, isInPacket);

	if (hostInfo.name.empty())
	{
		if (auto *httpRequestLayer = packet.getLayerOfType<pcpp::HttpRequestLayer>())
		{
			if (auto *hostField = httpRequestLayer->getFieldByName(PCPP_HTTP_HOST_FIELD))
			{
				hostInfo.name = hostField->getFieldValue();
				BOOST_LOG_TRIVIAL(info) << "HTTP host name detected: " << hostInfo.name;
			}
		}
		else if (auto *sslHadshakeLayer = packet.getLayerOfType<pcpp::SSLHandshakeLayer>())
		{
			if (auto *clientHelloMessage = sslHadshakeLayer->getHandshakeMessageOfType<pcpp::SSLClientHelloMessage>())
			{
				if (auto *sniExt = clientHelloMessage->getExtensionOfType<pcpp::SSLServerNameIndicationExtension>())
				{
					hostInfo.name = sniExt->getHostName();
					BOOST_LOG_TRIVIAL(info) << "HTTPS host name detected: " << hostInfo.name;
				}
			}
		}
	}
}