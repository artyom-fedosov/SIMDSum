// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <string_view>

import simdsum;

TEST(Library, ReportsVersion)
{
    EXPECT_EQ(simdsum::version(), std::string_view{"1.0.0"});
}
