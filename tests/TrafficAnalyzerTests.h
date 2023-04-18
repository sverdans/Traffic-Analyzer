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

TEST_F(TrafficAnalyzerClassTest, TestStartCaptureBeforeInit)
{
	analyzer.finalize();
	EXPECT_NO_THROW(analyzer.startCapture());
}

TEST_F(TrafficAnalyzerClassTest, TestGetStatBeforeInit)
{
	EXPECT_EQ("", analyzer.getPlaneTextStat());
}
