// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>

#include <array>
#include <string_view>

import simdsum;

namespace {

constexpr std::array all_isas{simdsum::Isa::Scalar, simdsum::Isa::SSE2, simdsum::Isa::AVX2, simdsum::Isa::AVX512};

simdsum::Isa best_supported()
{
    auto best = simdsum::Isa::Scalar;
    for (auto isa : all_isas) {
        if (simdsum::isa_supported(isa)) {
            best = isa;
        }
    }
    return best;
}

} // namespace

TEST(Dispatch, ScalarIsAlwaysSupported)
{
    EXPECT_TRUE(simdsum::isa_supported(simdsum::Isa::Scalar));
}

TEST(Dispatch, DefaultsToBestSupportedIsa)
{
    EXPECT_EQ(simdsum::active_isa(), best_supported());
}

TEST(Dispatch, ForceIsaSwitchesAndRestores)
{
    const auto best = simdsum::active_isa();
    ASSERT_TRUE(simdsum::force_isa(simdsum::Isa::Scalar));
    EXPECT_EQ(simdsum::active_isa(), simdsum::Isa::Scalar);
    ASSERT_TRUE(simdsum::force_isa(best));
    EXPECT_EQ(simdsum::active_isa(), best);
}

TEST(Dispatch, ForcingUnsupportedIsaFailsAndKeepsState)
{
    const auto before = simdsum::active_isa();
    for (auto isa : all_isas) {
        if (!simdsum::isa_supported(isa)) {
            EXPECT_FALSE(simdsum::force_isa(isa));
            EXPECT_EQ(simdsum::active_isa(), before);
        }
    }
}

TEST(Dispatch, IsaNames)
{
    EXPECT_EQ(simdsum::to_string(simdsum::Isa::Scalar), std::string_view{"Scalar"});
    EXPECT_EQ(simdsum::to_string(simdsum::Isa::SSE2), std::string_view{"SSE2"});
    EXPECT_EQ(simdsum::to_string(simdsum::Isa::AVX2), std::string_view{"AVX2"});
    EXPECT_EQ(simdsum::to_string(simdsum::Isa::AVX512), std::string_view{"AVX512"});
}
