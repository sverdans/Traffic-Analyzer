#include <gtest/gtest.h>
#include "AppNamespaceTest.h"
#include "HttpTrafficStatsTests.h"
#include "TrafficAnalyzerTests.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}