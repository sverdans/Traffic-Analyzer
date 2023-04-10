#include "HttpStatsCollector.h"

#include <iomanip>
#include <boost/log/trivial.hpp>

#include "pcapplusplus/Packet.h"
#include "pcapplusplus/HttpLayer.h"
#include "pcapplusplus/SSLLayer.h"
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
	// system("clear");

	for (const auto &[ip, hostInfo] : totalStat)
	{
		printf("%-37s %6d packets (OUT %-6d | %6d IN) traffic: %8d [bytes] (OUT %-6d | %6d IN)\n",
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

void HttpStatsCollector::addPacket(const pcpp::Packet &packet)
{
	auto *tcpLayer = packet.getLayerOfType<pcpp::TcpLayer>();
	auto *ipLayer = packet.getLayerOfType<pcpp::IPv4Layer>();

	if (!tcpLayer || !ipLayer)
	{
		BOOST_LOG_TRIVIAL(warning) << "IPv4Layer or TcpLayer was nullptr";
		return;
	}

	int size = tcpLayer->getLayerPayloadSize();
	auto srcIp = ipLayer->getSrcIPv4Address().toString();
	auto dstIp = ipLayer->getDstIPv4Address().toString();

	BOOST_LOG_TRIVIAL(debug) << "Captured packet {"
							 << " srcIP: " << std::left << std::setw(15) << srcIp
							 << " dstIP: " << std::left << std::setw(15) << dstIp
							 << " srcPort: " << std::left << std::setw(5) << tcpLayer->getSrcPort()
							 << " dstPort: " << std::left << std::setw(5) << tcpLayer->getDstPort() << " }";

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
			while (sslHadshakeLayer)
			{
				if (auto *clientHelloMessage = sslHadshakeLayer->getHandshakeMessageOfType<pcpp::SSLClientHelloMessage>())
				{
					if (auto *sniExt = clientHelloMessage->getExtensionOfType<pcpp::SSLServerNameIndicationExtension>())
					{
						hostInfo.name = sniExt->getHostName();
						BOOST_LOG_TRIVIAL(info) << "HTTPS host name detected: " << hostInfo.name;
						break;
					}
				}
				sslHadshakeLayer = packet.getNextLayerOfType<pcpp::SSLHandshakeLayer>(sslHadshakeLayer);
			}
		}
	}
}