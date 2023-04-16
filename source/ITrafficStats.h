#pragma once
#include <string>

#include "Packet.h"

class ITrafficStats
{
protected:
	std::string interfaceIpAddr;

public:
	ITrafficStats(const std::string &interfaceIpAddr) : interfaceIpAddr(interfaceIpAddr) {}

	virtual ~ITrafficStats() {}

	virtual std::string toString() = 0;
	virtual std::string toJsonString() = 0;

	virtual void addPacket(const pcpp::Packet &packet) = 0;
	virtual void clear() = 0;
};
