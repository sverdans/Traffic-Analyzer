#pragma once
#include <gtest/gtest.h>

#include "PacketUtils.h"
#include "EthLayer.h"
#include "HttpLayer.h"
#include "SSLLayer.h"
#include "TcpLayer.h"
#include "UdpLayer.h"
#include "IPv4Layer.h"

#include "../source/TrafficAnalyzer.h"
#include "../source/HttpTrafficStats.h"

struct HttpTrafficStatsClassTest : public testing::Test
{
	HttpTrafficStats *trafficStats;

	void SetUp() { trafficStats = new HttpTrafficStats("127.0.0.1"); }
	void TearDown() { delete trafficStats; }
};

TEST_F(HttpTrafficStatsClassTest, AddPacketTest)
{
	pcpp::EthLayer newEthernetLayer(pcpp::MacAddress("00:50:43:11:22:33"), pcpp::MacAddress("aa:bb:cc:dd:ee"));
	pcpp::IPv4Layer newIPLayer(pcpp::IPv4Address("192.192.1.1"), pcpp::IPv4Address("127.0.0.1"));
	pcpp::TcpLayer newTcpLayer(80, 80);

	pcpp::Packet newPacket;
	newPacket.addLayer(&newEthernetLayer);
	newPacket.addLayer(&newIPLayer);
	newPacket.addLayer(&newTcpLayer);

	newPacket.computeCalculateFields();

	trafficStats->addPacket(newPacket);

	std::stringstream expectation;

	expectation << std::left << std::setw(37) << newIPLayer.getSrcIPAddress().toString() << " "
				<< std::right << std::setw(6) << 1 << " packets (OUT "
				<< std::left << std::setw(6) << 0 << " | "
				<< std::right << std::setw(6) << 1 << " IN) traffic: "
				<< std::right << std::setw(8) << newPacket.getRawPacket()->getRawDataLen() << " [bytes] (OUT "
				<< std::left << std::setw(8) << 0 << " | "
				<< std::right << std::setw(6) << newPacket.getRawPacket()->getRawDataLen() << " IN)" << std::endl;

	EXPECT_EQ(expectation.str(), trafficStats->toString());
}

TEST_F(HttpTrafficStatsClassTest, ClearStatsTest)
{
	trafficStats->clear();
	EXPECT_EQ("", trafficStats->toString());
}
