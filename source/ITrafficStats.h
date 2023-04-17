#pragma once
#include <string>

#include "Packet.h"

/** \brief Интерфейс, определяющий методы обработки полученных пакетов и вывода статистики
 *  Обязывает наследников переопределить абстактные методы
 **/
class ITrafficStats
{
protected:
	std::string interfaceIpAddr; ///< IP-адрес интерфейса, для которого собирается статистика

public:
	ITrafficStats(const std::string &interfaceIpAddr) : interfaceIpAddr(interfaceIpAddr) {}

	virtual ~ITrafficStats() {}

	/// \brief Возвращает статистику об обработанных пакетах в виде строки
	virtual std::string toString() = 0;

	/// \brief Возвращает статистику об обработанных пакетах в формате JSON
	virtual std::string toJsonString() = 0;

	/// \brief Обрабатывает полученный пакет, записывает данные о нём в статистку
	virtual void addPacket(const pcpp::Packet &packet) = 0;

	/// \brief Очищает собранную статистку
	virtual void clear() = 0;
};
