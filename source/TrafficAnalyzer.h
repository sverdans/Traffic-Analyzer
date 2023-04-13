#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <map>

#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

#include "PcapLiveDeviceList.h"
#include "PacketUtils.h"
#include "HttpLayer.h"
#include "SSLLayer.h"
#include "TcpLayer.h"
#include "UdpLayer.h"
#include "IPv4Layer.h"
#include "Packet.h"

#include "HostInfo.h"

class TrafficAnalyzer
{
private:
	std::mutex collectorMutex;
	std::string interfaceIPAddr;
	std::map<std::string, HostInfo> stat;

	pcpp::PcapLiveDevice *dev;
	pcpp::ProtoFilter filter;

	void addPacket(const pcpp::Packet &packet)
	{
		if (!packet.isPacketOfType(pcpp::TCP))
			return;

		auto *ipLayer = packet.getLayerOfType<pcpp::IPLayer>();

		if (!ipLayer)
		{
			BOOST_LOG_TRIVIAL(warning) << "IPLayer was nullptr";
			return;
		}

		auto srcIp = ipLayer->getSrcIPAddress().toString();
		auto dstIp = ipLayer->getDstIPAddress().toString();
		int size = packet.getRawPacket()->getRawDataLen();

		auto *tcpLayer = packet.getLayerOfType<pcpp::TcpLayer>();
		int srcPort = tcpLayer->getSrcPort();
		int dstPort = tcpLayer->getDstPort();

		BOOST_LOG_TRIVIAL(debug) << "Captured packet {"
								 << " srcIP: " << std::left << std::setw(15) << srcIp
								 << " dstIP: " << std::left << std::setw(15) << dstIp
								 << " srcPort: " << std::left << std::setw(6) << srcPort
								 << " dstPort: " << std::left << std::setw(6) << dstPort
								 << " }";

		/*
		BOOST_LOG_TRIVIAL(debug) << "Captured packet {"
								 << " srcIP: " << std::left << std::setw(15) << srcIp
								 << " dstIP: " << std::left << std::setw(15) << dstIp
								 << " size: " << std::left << std::setw(9) << size << " }";
		*/

		bool isInPacket = dstIp == interfaceIPAddr;
		auto &hostInfo = isInPacket ? stat[srcIp] : stat[dstIp];

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

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		TrafficAnalyzer *analyzer = static_cast<TrafficAnalyzer *>(cookie);

		std::lock_guard<std::mutex> guard(analyzer->collectorMutex);

		analyzer->addPacket(pcpp::Packet(packet));
	}

public:
	TrafficAnalyzer(std::string interfaceIPAddr, pcpp::ProtocolType protocol)
		: interfaceIPAddr(interfaceIPAddr), filter(pcpp::ProtoFilter(protocol))
	{
		dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);

		if (!dev)
			throw std::runtime_error("TrafficAnalyzer: cannot find interface with IPv4 address of '" + interfaceIPAddr + "'");

		if (!dev->open())
			throw std::runtime_error("TrafficAnalyzer: cannot open device");

		if (!dev->setFilter(filter))
		{
			dev->close();
			throw std::runtime_error("TrafficAnalyzer: cannot set filter");
		}
	}

	~TrafficAnalyzer() { dev->close(); }

	TrafficAnalyzer(const TrafficAnalyzer &) = delete;
	TrafficAnalyzer &operator=(const TrafficAnalyzer &) = delete;

	std::string getPlaneTextStat()
	{
		std::stringstream ss;
		std::lock_guard<std::mutex> guard(collectorMutex);

		for (const auto &[ip, hostInfo] : stat)
		{
			ss << std::left << std::setw(37) << (hostInfo.name.empty() ? ip.c_str() : hostInfo.name.c_str()) << " "
			   << std::right << std::setw(6) << (hostInfo.inPackets + hostInfo.outPackets) << " packets (OUT "
			   << std::left << std::setw(6) << hostInfo.outPackets << " | "
			   << std::right << std::setw(6) << hostInfo.inPackets << " IN) traffic: "
			   << std::right << std::setw(8) << (hostInfo.inTraffic + hostInfo.outTraffic) << " [bytes] (OUT "
			   << std::left << std::setw(8) << hostInfo.outTraffic << " | "
			   << std::right << std::setw(6) << hostInfo.inTraffic << " IN)" << std::endl;
		}

		return ss.str();
	}

	std::string getJsonStat()
	{
		std::stringstream ss;
		nlohmann::json jsonStat = nlohmann::json::object();
		jsonStat["hosts"] = nlohmann::json::array();

		std::lock_guard<std::mutex> guard(collectorMutex);

		for (const auto &[ip, hostInfo] : stat)
		{
			nlohmann::json hostJson;
			hostJson["ip"] = ip;
			hostJson["name"] = hostInfo.name;

			hostJson["traffic"]["in"] = hostInfo.inTraffic;
			hostJson["traffic"]["out"] = hostInfo.outTraffic;
			hostJson["traffic"]["total"] = hostInfo.outTraffic + hostInfo.inTraffic;

			hostJson["packets"]["in"] = hostInfo.inPackets;
			hostJson["packets"]["out"] = hostInfo.outPackets;
			hostJson["packets"]["total"] = hostInfo.outPackets + hostInfo.inPackets;

			jsonStat["hosts"].push_back(hostJson);
		}

		ss << jsonStat << std::endl;
		return ss.str();
	}

	void startCapture()
	{
		dev->startCapture(onPacketArrives, this);
	}

	void stopCapture()
	{
		dev->stopCapture();
	}
};
