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

/**
 * \brief Класс реализующий перехват пакетов из живого трафика и их анализ
 *
 * Производит захват и обработку пакетов в отдельном потоке
 */
class TrafficAnalyzer
{
private:
	std::string interfaceIpAddr; ///< IP-адрес интерфейса, для которого собирается статистика

	pcpp::OrFilter filter;
	pcpp::PcapLiveDevice *dev;

	std::unique_ptr<std::mutex> collectorMutex;	 ///< Объект синхронизирущий работу со статистикой из рахных потоков
	std::unique_ptr<ITrafficStats> trafficStats; ///< Объект отвечающий за обработку траффика и вывод статистики в формате строки

	static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie)
	{
		TrafficAnalyzer *analyzer = static_cast<TrafficAnalyzer *>(cookie);
		std::lock_guard<std::mutex> guard(*analyzer->collectorMutex);
		analyzer->trafficStats->addPacket(pcpp::Packet(packet));
	}

public:
	TrafficAnalyzer() : dev(nullptr), collectorMutex(std::make_unique<std::mutex>()) {}
	~TrafficAnalyzer() { finalize(); }

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

	/// \brief Инициализирующий метод
	/// \tparam T Тип который будет иметь trafficStats
	/// \param[in] interfaceIpAddr IP-адрес устройства, для которого будет собираться статистика
	/// \param[in] portFilterVec Вектор портов, по которым будет происходить анализ пакетов
	/// \param[out] errorInfo В случае ошибки инициализации, сюда будет записана причина
	/// \return True - если инициализация прошла усешно, иначе False
	template <class T>
	bool initializeAs(const std::string &interfaceIpAddr,
					  std::vector<pcpp::GeneralFilter *> &portFilterVec,
					  std::string &errorInfo)
	{
		this->interfaceIpAddr = interfaceIpAddr;

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

		filter = pcpp::OrFilter(portFilterVec);
		std::string filterAsString;
		filter.parseToString(filterAsString);

		if (!dev->setFilter(filter))
		{
			errorInfo = "TrafficAnalyzer: cannot set filter '" + filterAsString + "'";
			dev->close();
			return false;
		}

		BOOST_LOG_TRIVIAL(info) << "TrafficAnalyzer filter: '" << filterAsString << "'";

		trafficStats = std::make_unique<T>(interfaceIpAddr);

		return true;
	}

	/// \brief Освобождения ресурсы, занимаемымы объектом
	void finalize()
	{
		if (dev)
		{
			if (dev->captureActive())
				dev->stopCapture();

			if (dev->isOpened())
				dev->close();
		}

		if (trafficStats.get())
			trafficStats->clear();
	}

	/// \brief Начинает захват пакетов из живого трафика
	void startCapture()
	{
		if (dev && dev->isOpened())
			dev->startCapture(onPacketArrives, this);
		else
			BOOST_LOG_TRIVIAL(warning) << "TrafficAnalyzer trying to startCapture, but device was not opened or nullptr";
	}

	/// \brief Останавливает захват пакетов
	void stopCapture() { dev->stopCapture(); }

	/// \brief Возвращает собранную статистику в виде строки
	std::string getPlaneTextStat()
	{
		std::lock_guard<std::mutex> guard(*collectorMutex);
		return trafficStats->toString();
	}

	/// \brief Возвращает собранную статистику в формате JSON строки
	std::string getJsonStat()
	{
		std::lock_guard<std::mutex> guard(*collectorMutex);
		return trafficStats->toJsonString();
	}

	/// \brief Очищает собранную статистику
	void clearStats()
	{
		std::lock_guard<std::mutex> guard(*collectorMutex);
		trafficStats->clear();
	}
};
