/*
 * Copyright (c) 2026, An-Chi Liu <phy.tiger@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the copyright holder nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <vector>

#include <modmesh/simd/simd.hpp>
#include <modmesh/simd/simd_support.hpp>

// If detect_simd() does not report NEON on aarch64 the SIMD tests below
// silently fall through to the scalar path and stay green without ever
// exercising the NEON refactor.
TEST(SimdDispatch, neon_active_on_aarch64)
{
    const auto feature = modmesh::simd::detail::detect_simd();
#if defined(__aarch64__)
    EXPECT_EQ(feature, modmesh::simd::detail::SIMD_NEON);
#else
    GTEST_SKIP() << "detect_simd() = " << feature;
#endif
}

namespace
{

const std::int32_t * find_out_of_range(std::vector<std::int32_t> const & v, std::int32_t lo, std::int32_t hi)
{
    return modmesh::simd::check_between<std::int32_t>(v.data(), v.data() + v.size(), lo, hi);
}

// Regression guard for issue #635: inputs shorter than one SIMD lane
// used to recurse into the generic loop and corrupt the stack.
TEST(SimdTransformBinary, add_int32_covers_all_shapes)
{
    constexpr std::size_t sizes[] = {0, 1, 3, 4, 5, 8, 17};
    for (std::size_t n : sizes)
    {
        std::vector<std::int32_t> a(n), b(n), out(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            a[i] = static_cast<std::int32_t>(i);
            b[i] = static_cast<std::int32_t>(2 * i + 1);
        }
        modmesh::simd::add<std::int32_t>(out.data(), out.data() + n, a.data(), b.data());
        for (std::size_t i = 0; i < n; ++i)
        {
            EXPECT_EQ(out[i], a[i] + b[i]) << "n=" << n << " i=" << i;
        }
    }
}

TEST(SimdTransformBinary, sub_mul_div_float)
{
    constexpr std::size_t n = 7; // one NEON float lane + 3-element tail
    std::vector<float> a(n), b(n), out(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        a[i] = static_cast<float>(i + 10);
        b[i] = static_cast<float>(i + 1);
    }

    auto apply_and_check = [&](auto simd_op, auto scalar_op)
    {
        simd_op(out.data(), out.data() + n, a.data(), b.data());
        for (std::size_t i = 0; i < n; ++i)
        {
            EXPECT_FLOAT_EQ(out[i], scalar_op(a[i], b[i]));
        }
    };

    apply_and_check(&modmesh::simd::sub<float>, std::minus<float>{});
    apply_and_check(&modmesh::simd::mul<float>, std::multiplies<float>{});
    apply_and_check(&modmesh::simd::div<float>, std::divides<float>{});
}

// vmulq has no int64 overload; the SFINAE guard must route int64 multiply
// to the generic scalar path instead of failing to compile.
TEST(SimdTransformBinary, int64_mul_falls_back_to_generic)
{
    std::vector<std::int64_t> a{1, 2, 3, 4, 5};
    std::vector<std::int64_t> b{10, 20, 30, 40, 50};
    std::vector<std::int64_t> out(a.size());
    modmesh::simd::mul<std::int64_t>(out.data(), out.data() + out.size(), a.data(), b.data());
    EXPECT_EQ(out, (std::vector<std::int64_t>{10, 40, 90, 160, 250}));
}

TEST(SimdCheckBetween, returns_null_when_all_in_range)
{
    std::vector<std::int32_t> data{0, 1, 2, 3, 4, 5, 6};
    EXPECT_EQ(find_out_of_range(data, 0, 6), nullptr);
}

TEST(SimdCheckBetween, hits_lane_in_vector_body)
{
    std::vector<std::int32_t> data{0, 1, -1, 3, 4, 5, 6};
    const auto * hit = find_out_of_range(data, 0, 6);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit - data.data(), 2);
}

TEST(SimdCheckBetween, hits_lane_in_scalar_tail)
{
    std::vector<std::int32_t> data{0, 1, 2, 3, 4, 99, 6};
    const auto * hit = find_out_of_range(data, 0, 6);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit - data.data(), 5);
}

// Regression guard for the earliest-lane bug: within one SIMD block an
// earlier lane is below min and a later lane is above max. Callers like
// take_along_axis_simd build a diagnostic from the returned pointer, so
// the earlier index is the correct answer.
TEST(SimdCheckBetween, earliest_lane_wins_within_same_block)
{
    std::vector<std::int32_t> data{2, -1, 3, 99};
    const auto * hit = find_out_of_range(data, 0, 10);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit - data.data(), 1);
}

// Inputs shorter than one SIMD lane used to form `end - N_lane`, a
// pointer before the buffer (UB). asan/ubsan catches a regression here.
TEST(SimdCheckBetween, no_underflow_on_short_inputs)
{
    std::vector<std::int32_t> empty;
    EXPECT_EQ(find_out_of_range(empty, 0, 10), nullptr);

    std::vector<std::int32_t> one{5};
    EXPECT_EQ(find_out_of_range(one, 0, 10), nullptr);

    std::vector<std::int32_t> three{5, 99, 6};
    const auto * hit = find_out_of_range(three, 0, 10);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit - three.data(), 1);
}

} // namespace

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
