#pragma once
#include <iostream>
#include <type_traits>
#include <stdexcept>
#include <string>
#include <mutex>

#include <boost/log/trivial.hpp>

#include "PcapLiveDeviceList.h"
#include "PacketUtils.h"
#include "Packet.h"

#include "ITrafficStats.h"

/**
 *	\brief Класс, реализующий захват пакетов из живого траффика
 */
template <class T, std::enable_if_t<std::is_base_of<ITrafficStats, T>::value, int> = 0>
class TrafficAnalyzer
{
private:
	std::mutex collectorMutex;
	std::string interfaceIPAddr; ///< Ip адрес интерфейса, для которого собирается статистика

	pcpp::OrFilter filter;
	pcpp::PcapLiveDevice *dev;

	T trafficStats;

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		TrafficAnalyzer *analyzer = static_cast<TrafficAnalyzer *>(cookie);
		std::lock_guard<std::mutex> guard(analyzer->collectorMutex);
		analyzer->trafficStats.addPacket(pcpp::Packet(packet));
	}

public:
	TrafficAnalyzer(const std::string interfaceIPAddr)
		: interfaceIPAddr(interfaceIPAddr), trafficStats(T(interfaceIPAddr)) {}

	bool initialize(std::vector<pcpp::GeneralFilter *> &portFilterVec, std::string &errorInfo)
	{
		this->filter = pcpp::OrFilter(portFilterVec);

		dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);

		if (!dev)
		{
			errorInfo = "TrafficAnalyzer: cannot find interface with IPv4 address of '" + interfaceIPAddr + "'";
			return false;
		}

		if (!dev->open())
		{
			errorInfo = "TrafficAnalyzer: cannot open device";
			return false;
		}

		std::string filterAsString;
		filter.parseToString(filterAsString);

		if (!dev->setFilter(filter))
		{
			errorInfo = "TrafficAnalyzer: cannot set filter '" + filterAsString + "'";
			dev->close();
			return false;
		}

		BOOST_LOG_TRIVIAL(info) << "TrafficAnalyzer filter: '" << filterAsString << "'";
		return true;
	}

	~TrafficAnalyzer() { dev->close(); }

	TrafficAnalyzer(const TrafficAnalyzer &) = delete;
	TrafficAnalyzer &operator=(const TrafficAnalyzer &) = delete;

	TrafficAnalyzer(TrafficAnalyzer &&other)
		: dev(other.dev),
		  interfaceIPAddr(std::move(other.interfaceIPAddr)),
		  collectorMutex(std::move(collectorMutex)),
		  filter(std::move(other.filter)),
		  trafficStats(std::move(other.trafficStats))
	{
		other.dev = nullptr;
	}

	TrafficAnalyzer &operator=(TrafficAnalyzer &&other)
	{
		dev = other.dev;
		filter = std::move(other.filter);
		trafficStats = std::move(other.trafficStats);
		collectorMutex = std::move(collectorMutex);
		interfaceIPAddr = std::move(other.interfaceIPAddr);

		other.dev = nullptr;
	}

	void startCapture() { dev->startCapture(onPacketArrives, this); }

	void stopCapture() { dev->stopCapture(); }

	std::string getPlaneTextStat()
	{
		std::lock_guard<std::mutex> guard(collectorMutex);
		return trafficStats.toString();
	}

	std::string getJsonStat()
	{
		std::lock_guard<std::mutex> guard(collectorMutex);
		return trafficStats.toJsonString();
	}

	void clearStats()
	{
		std::lock_guard<std::mutex> guard(collectorMutex);
		trafficStats.clear();
	}
};
