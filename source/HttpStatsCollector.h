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
	int inPackets{0};
	int outPackets{0};
	int inTraffic{0};
	int outTraffic{0};
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
	int printedLinesCount{0};
	std::string userIp;
	std::map<std::string, HostInfo> totalStat;

public:
	HttpStatsCollector(const std::string &userIp);
	~HttpStatsCollector();

	HttpStatsCollector(const HttpStatsCollector &other) noexcept;

	HttpStatsCollector &operator=(const HttpStatsCollector &other) noexcept;

	HttpStatsCollector(HttpStatsCollector &&other) noexcept;

	HttpStatsCollector &operator=(HttpStatsCollector &&other) noexcept;

	void clear();
	void print();

	void addPacket_old(const pcpp::Packet &packet);
	void addPacket(const pcpp::Packet &packet);
};