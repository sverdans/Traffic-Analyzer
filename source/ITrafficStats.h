#pragma once
#include <string>

#include "Packet.h"

/// \brief Интерфейс, определяющий методы обработки полученных пакетов и вывода статистики
class ITrafficStats
{
protected:
	std::string interfaceIpAddr; ///< Ip адрес интерфейса, для которого собирается статистика

public:
	ITrafficStats(const std::string &interfaceIpAddr) : interfaceIpAddr(interfaceIpAddr) {}

	virtual ~ITrafficStats() {}

	virtual std::string toString() = 0;
	virtual std::string toJsonString() = 0;

	virtual void addPacket(const pcpp::Packet &packet) = 0;
	virtual void clear() = 0;
};
