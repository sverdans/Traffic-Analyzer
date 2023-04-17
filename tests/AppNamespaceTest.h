#pragma once
#include <gtest/gtest.h>

#include "../source/App.h"

TEST(ComandLineParsingTest, TestNegativeExecutionTime)
{
	char *options[] = {"./path", "-t", "-100"};
	EXPECT_ANY_THROW(app::parseComandLine(3, options));
}

TEST(ComandLineParsingTest, TestNegativeUpdateTime)
{
	char *options[] = {"./path", "-u", "-100"};
	EXPECT_ANY_THROW(app::parseComandLine(3, options));
}

TEST(ComandLineParsingTest, TestNoParamsComandLine)
{
	char *options[] = {"./path"};
	EXPECT_NO_THROW(app::parseComandLine(1, options));
}

TEST(ComandLineParsingTest, TestComandLineWithRightParams)
{
	app::ProgramOptions expectation{false, 10, 200, "127.0.0.1"};

	char *options[] = {"./path", "-u", "10", "-t", "200", "-i", "127.0.0.1"};
	app::ProgramOptions result = app::parseComandLine(7, options);

	EXPECT_EQ(expectation.shouldClose, result.shouldClose);
	EXPECT_EQ(expectation.updatePeriod, result.updatePeriod);
	EXPECT_EQ(expectation.executionTime, result.executionTime);
	EXPECT_EQ(expectation.interfaceIpAddr, result.interfaceIpAddr);
}
