#pragma once
#include <gtest/gtest.h>

#include "PcapFilter.h"

#include "../source/TrafficAnalyzer.h"
#include "../source/HttpTrafficStats.h"

struct TrafficAnalyzerClassTest : public testing::Test
{
	TrafficAnalyzer<HttpTrafficStats> *ta{nullptr};
	std::vector<pcpp::GeneralFilter *> vec = {};
	std::string realIp;

	void SetUp()
	{
		realIp = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList()[0]->getIPv4Address().toString();
	}

	void TearDown()
	{
		if (ta)
			delete ta;
	}
};

TEST_F(TrafficAnalyzerClassTest, TestCreateWithNonexistentIp)
{
	EXPECT_ANY_THROW(ta = new TrafficAnalyzer<HttpTrafficStats>("999.999.999.999", vec));
	delete ta;
	ta = nullptr;
}

TEST_F(TrafficAnalyzerClassTest, TestCreateWithExistentIp)
{
	EXPECT_NO_THROW(ta = new TrafficAnalyzer<HttpTrafficStats>(realIp, vec));
	delete ta;
	ta = nullptr;
}
