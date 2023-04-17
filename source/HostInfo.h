#pragma once
#include <string>

/// \brief Структура, хранящая статистику отдельного хоста
struct HostInfo
{
	unsigned int inPackets{0};	///< Количество входящих пакетов
	unsigned int outPackets{0}; ///< Количество исходящих пакетов
	unsigned int inTraffic{0};	///< Входящий трафик
	unsigned int outTraffic{0}; ///< Исходяший трафик
	std::string name;			///< Имя хоста

	/**
	 * \param[in] size Размер пакета
	 * \param[in] isInPacket Является ли пакет входящим
	 */
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