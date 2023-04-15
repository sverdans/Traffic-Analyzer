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

template <class T, std::enable_if_t<std::is_base_of<ITrafficStats, T>::value, int> = 0>
class TrafficAnalyzer
{
private:
	std::mutex collectorMutex;
	std::string interfaceIPAddr;

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
	TrafficAnalyzer(const std::string interfaceIPAddr, std::vector<pcpp::GeneralFilter *> &portFilterVec)
		: interfaceIPAddr(interfaceIPAddr), filter(pcpp::OrFilter(portFilterVec)), trafficStats(T(interfaceIPAddr))
	{
		dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);

		if (!dev)
			throw std::runtime_error("TrafficAnalyzer: cannot find interface with IPv4 address of '" + interfaceIPAddr + "'");

		if (!dev->open())
			throw std::runtime_error("TrafficAnalyzer: cannot open device");

		std::string filterAsString;
		filter.parseToString(filterAsString);

		if (!dev->setFilter(filter))
		{
			dev->close();
			throw std::runtime_error("TrafficAnalyzer: cannot set filter '" + filterAsString + "'");
		}

		BOOST_LOG_TRIVIAL(info) << "TrafficAnalyzer filter: '" << filterAsString << "'";
	}

	~TrafficAnalyzer()
	{
		dev->close();
	}

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

	void startCapture()
	{
		dev->startCapture(onPacketArrives, this);
	}

	void stopCapture()
	{
		dev->stopCapture();
	}
};
