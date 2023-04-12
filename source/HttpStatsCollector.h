#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>

namespace pcpp
{
	class Packet;
}

struct HostInfo
{
	unsigned int inPackets{0};
	unsigned int outPackets{0};
	unsigned int inTraffic{0};
	unsigned int outTraffic{0};
	std::string name;

	void addPacket(int size, bool isInPacket)
	{
		if (isInPacket)
		{
			inPackets++;
			inTraffic += size;
		}
		else
		{
			outPackets++;
			outTraffic += size;
		}
	}
};

class HttpStatsCollector
{
private:
	std::string userIp;
	std::map<std::string, HostInfo> totalStat;

public:
	HttpStatsCollector(const std::string &userIp);
	~HttpStatsCollector();

	HttpStatsCollector(const HttpStatsCollector &other) noexcept;

	HttpStatsCollector &operator=(const HttpStatsCollector &other) noexcept;

	HttpStatsCollector(HttpStatsCollector &&other) noexcept;

	HttpStatsCollector &operator=(HttpStatsCollector &&other) noexcept;

	void print();
	void printWithStat();

	void addPacket(const pcpp::Packet &packet);
};