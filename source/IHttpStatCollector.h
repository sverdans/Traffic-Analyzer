#pragma once
#include <string>
#include <iomanip>
#include <sstream>
#include <map>

#include <nlohmann/json.hpp>

#include "Packet.h"
#include "PacketUtils.h"
#include "HttpLayer.h"
#include "SSLLayer.h"
#include "TcpLayer.h"
#include "UdpLayer.h"
#include "IPv4Layer.h"

#include "HostInfo.h"

class ITrafficStats
{
public:
	virtual ~ITrafficStats() {}

	virtual std::string toString() = 0;
	virtual std::string toJsonString() = 0;

	virtual void addPacket(const pcpp::Packet &packet) = 0;
};

class HttpTrafficStat : public ITrafficStats
{
private:
	std::map<std::string, HostInfo> stat;

public:
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

	std::string toJsonString() override
	{
		std::stringstream ss;
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

		ss << jsonStat << std::endl;
		return ss.str();
	}

	void addPacket(const pcpp::Packet &packet) override
	{
	}
};