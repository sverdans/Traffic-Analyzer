#pragma once
#include <iostream>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <string>
#include <mutex>

#include <boost/log/trivial.hpp>

#include "PcapLiveDeviceList.h"
#include "PacketUtils.h"
#include "Packet.h"

#include "ITrafficStats.h"

class TrafficAnalyzer
{
private:
	std::string interfaceIpAddr; ///< Ip адрес интерфейса, для которого собирается статистика

	pcpp::OrFilter filter;
	pcpp::PcapLiveDevice *dev{nullptr};

	std::unique_ptr<std::mutex> collectorMutex;
	std::unique_ptr<ITrafficStats> trafficStats;

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		TrafficAnalyzer *analyzer = static_cast<TrafficAnalyzer *>(cookie);
		std::lock_guard<std::mutex> guard(*analyzer->collectorMutex);
		analyzer->trafficStats->addPacket(pcpp::Packet(packet));
	}

public:
	TrafficAnalyzer() : collectorMutex(std::make_unique<std::mutex>()) {}
	~TrafficAnalyzer()
	{
		if (dev)
			dev->close();
	}

	TrafficAnalyzer(const TrafficAnalyzer &) = delete;
	TrafficAnalyzer &operator=(const TrafficAnalyzer &) = delete;

	TrafficAnalyzer(TrafficAnalyzer &&other)
		: dev(other.dev),
		  interfaceIpAddr(std::move(other.interfaceIpAddr)),
		  collectorMutex(std::move(other.collectorMutex)),
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
		collectorMutex = std::move(other.collectorMutex);
		interfaceIpAddr = std::move(other.interfaceIpAddr);

		other.dev = nullptr;

		return *this;
	}

	template <class T>
	bool initializeAs(const std::string &interfaceIpAddr,
					  std::vector<pcpp::GeneralFilter *> &portFilterVec,
					  std::string &errorInfo)
	{
		this->interfaceIpAddr = interfaceIpAddr;
		trafficStats = std::make_unique<T>(interfaceIpAddr);

		filter = pcpp::OrFilter(portFilterVec);
		dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIpAddr);

		if (!dev)
		{
			errorInfo = "TrafficAnalyzer: cannot find interface with IPv4 address of '" + interfaceIpAddr + "'";
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

	void finalize() { dev->close(); }

	void startCapture() { dev->startCapture(onPacketArrives, this); }

	void stopCapture() { dev->stopCapture(); }

	std::string getPlaneTextStat()
	{
		std::lock_guard<std::mutex> guard(*collectorMutex);
		return trafficStats->toString();
	}

	std::string getJsonStat()
	{
		std::lock_guard<std::mutex> guard(*collectorMutex);
		return trafficStats->toJsonString();
	}

	void clearStats()
	{
		std::lock_guard<std::mutex> guard(*collectorMutex);
		trafficStats->clear();
	}
};
