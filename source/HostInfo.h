#pragma once
#include <string>

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