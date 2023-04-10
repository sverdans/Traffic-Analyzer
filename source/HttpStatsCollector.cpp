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

	//	for (int i = 0; i < printedLinesCount; ++i)
	//	{
	//		printf("\033[1A");
	//		printf("\033[K");
	//	}

	system("clear");
	printedLinesCount = 0;

	for (const auto &[ip, hostInfo] : totalStat)
	{
		//	printf("%2d ", printedLinesCount);
		printf("%-35s %6d packets ( OUT %-6d / %6d IN ) traffic: %8d [bytes] ( OUT %-6d / %6d IN )\n",
			   (hostInfo.name.empty() ? ip.c_str() : hostInfo.name.c_str()),
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

void HttpStatsCollector::addPacket(const pcpp::Packet &packet)
{
	// verify packet is TCP
	if (!packet.isPacketOfType(pcpp::TCP))
		return;

	auto *tcpLayer = packet.getLayerOfType<pcpp::TcpLayer>();
	auto *ipLayer = packet.getLayerOfType<pcpp::IPv4Layer>();

	int size = tcpLayer->getLayerPayloadSize();
	auto srcIp = ipLayer->getSrcIPv4Address().toString();
	auto dstIp = ipLayer->getDstIPv4Address().toString();

	BOOST_LOG_TRIVIAL(debug) << "Captured packet { "
							 << " srcIP: " << std::left << std::setw(15) << srcIp
							 << " dstIP: " << std::left << std::setw(15) << dstIp
							 << " srcPort: " << std::left << std::setw(5) << tcpLayer->getSrcPort()
							 << " dstPort: " << std::left << std::setw(5) << tcpLayer->getDstPort() << " }";

	bool isInPacket;
	HostInfo *hostInfo;

	if (srcIp == userIp)
	{
		isInPacket = false;
		hostInfo = &(totalStat[dstIp]);
	}
	else
	{
		isInPacket = true;
		hostInfo = &(totalStat[srcIp]);
	}

	hostInfo->addPacket(size, isInPacket);

	if (hostInfo->name.empty())
	{
		if (auto *httpRequestLayer = packet.getLayerOfType<pcpp::HttpRequestLayer>())
		{
			pcpp::HeaderField *hostField = httpRequestLayer->getFieldByName(PCPP_HTTP_HOST_FIELD);
			if (hostField)
			{
				hostInfo->name = hostField->getFieldValue();
				BOOST_LOG_TRIVIAL(info) << "HTTP host name detected: " << hostInfo->name;
			}
		}
		else if (auto *sslLayer = packet.getLayerOfType<pcpp::SSLLayer>())
		{
			while (sslLayer)
			{
				pcpp::SSLRecordType recType = sslLayer->getRecordType();
				if (recType == pcpp::SSL_HANDSHAKE)
				{
					pcpp::SSLHandshakeLayer *handshakeLayer = dynamic_cast<pcpp::SSLHandshakeLayer *>(sslLayer);
					if (!handshakeLayer)
						continue;

					pcpp::SSLClientHelloMessage *clientHelloMessage = handshakeLayer->getHandshakeMessageOfType<pcpp::SSLClientHelloMessage>();
					if (!clientHelloMessage)
						continue;

					pcpp::SSLServerNameIndicationExtension *sniExt = clientHelloMessage->getExtensionOfType<pcpp::SSLServerNameIndicationExtension>();
					if (!sniExt)
						continue;

					hostInfo->name = sniExt->getHostName();
					BOOST_LOG_TRIVIAL(info) << "HTTPS host name detected: " << hostInfo->name;
					break;
				}

				sslLayer = packet.getNextLayerOfType<pcpp::SSLLayer>(sslLayer);
			}
		}
	}
}