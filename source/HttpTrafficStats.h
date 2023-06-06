#pragma once
#include <iomanip>
#include <sstream>
#include <string>
#include <map>

#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

#include <PacketUtils.h>
#include <HttpLayer.h>
#include <SSLLayer.h>
#include <IPv4Layer.h>

#include <ITrafficStats.h>
#include <HostInfo.h>

/// \brief Класс, определяющий формат вывода статистики и обработку пакетов HTTP трафика
class HttpTrafficStats : public ITrafficStats
{
private:
	std::map<std::string, HostInfo> stat; ///< Словарь, где ключ это IP адрес хоста, значение объект HostInfo

public:
	HttpTrafficStats(const std::string &interfaceIpAddr) : ITrafficStats(interfaceIpAddr) {}

	/// \brief Возвращает статистику об обработанных пакетах в виде строки
	std::string toString() override
	{
		std::stringstream ss;

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

	/// \brief Возвращает статистику об обработанных пакетах в формате JSON
	std::string toJsonString() override
	{
		nlohmann::json jsonStat = nlohmann::json::object();
		jsonStat["hosts"] = nlohmann::json::array();

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

		std::stringstream ss;
		ss << jsonStat << std::endl;

		return ss.str();
	}

	/// @brief Метод обрабатывающий пакет
	/// \param[in] packet Пакет, из который нужно
	void addPacket(const pcpp::Packet &packet) override
	{
		auto *ipLayer = packet.getLayerOfType<pcpp::IPLayer>();
		if (!ipLayer)
		{
			BOOST_LOG_TRIVIAL(warning) << "IPLayer was nullptr";
			return;
		}

		auto srcIp = ipLayer->getSrcIPAddress().toString();
		auto dstIp = ipLayer->getDstIPAddress().toString();
		int size = packet.getRawPacket()->getRawDataLen();

		int srcPort, dstPort;
		std::string transportProtoName;

		BOOST_LOG_TRIVIAL(debug) << "Captured packet {"
								 << " srcIP: " << std::left << std::setw(15) << srcIp
								 << " dstIP: " << std::left << std::setw(15) << dstIp
								 << " size: " << std::left << std::setw(9) << size << " }";

		bool isInPacket = dstIp == interfaceIpAddr;
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

	/// \brief Очищает статистику
	void clear() override
	{
		stat.clear();
	}
};
