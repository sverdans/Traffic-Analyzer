#pragma once
#include <gtest/gtest.h>

#include "PcapFilter.h"

#include "../source/TrafficAnalyzer.h"
#include "../source/HttpTrafficStats.h"

struct TrafficAnalyzerClassTest : public testing::Test
{
	TrafficAnalyzer analyzer;
	std::vector<pcpp::GeneralFilter *> vec = {};
	std::string realIp;

	void SetUp()
	{
		realIp = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList()[0]->getIPv4Address().toString();
	}

	void TearDown()
	{
		analyzer.finalize();
	}
};

TEST_F(TrafficAnalyzerClassTest, TestCreateWithNonexistentIp)
{
	std::string errorInfo;
	EXPECT_EQ(false, analyzer.initializeAs<HttpTrafficStats>("255.255.255.255", vec, errorInfo));
	analyzer.finalize();
}

TEST_F(TrafficAnalyzerClassTest, TestCreateWithExistentIp)
{
	std::string errorInfo;
	EXPECT_EQ(true, analyzer.initializeAs<HttpTrafficStats>(realIp, vec, errorInfo));
	analyzer.finalize();
}
