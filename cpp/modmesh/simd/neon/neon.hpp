#pragma once

/*
 * Copyright (c) 2025, Kuan-Hsien Lee <khlee870529@gmail.com>
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

#include <functional>
#include <type_traits>

#include <modmesh/simd/simd_generic.hpp>
#include <modmesh/simd/neon/neon_type.hpp>
#include <modmesh/simd/neon/neon_alias.hpp>

#if defined(__aarch64__)
#include <arm_neon.h>
#endif /* defined(__aarch64__) */

namespace modmesh
{

namespace simd
{

namespace neon
{

namespace detail
{

#ifndef NDEBUG
template <typename T>
bool is_aligned(T const * pointer, size_t alignment)
{
    return (reinterpret_cast<std::uintptr_t>(pointer) % alignment) == 0;
}

template <typename T>
void check_alignment(T const * pointer, size_t required_alignment, const char * name)
{
    if (!is_aligned(pointer, required_alignment))
    {
        std::fprintf(stderr,
                     "Warning: %s pointer %p is not aligned to %zu bytes. "
                     "SIMD performance may be degraded.\n",
                     name,
                     static_cast<const void *>(pointer),
                     required_alignment);
    }
}
#endif // NDEBUG

// Get the recommended memory alignment for SIMD operations based on the detected SIMD instruction set.
inline constexpr size_t get_recommended_alignment()
{
#if defined(__aarch64__) || defined(__arm__)
    return 16;
// TODO: The non-NEON conditional should be factored out elsewhere in the future.
#elif defined(__AVX512F__)
    return 64;
#elif defined(__AVX__) || defined(__AVX2__)
    return 32;
#elif defined(__SSE__) || defined(__SSE2__) || defined(__SSE3__) || defined(__SSSE3__) || defined(__SSE4_1__) || defined(__SSE4_2__)
    return 16;
#else
    return 0;
#endif
}

} /* end namespace detail */

#if defined(__aarch64__)

// Each vec_op's `operator()` is expression-SFINAE'd on `decltype(v*q(a, b))`
// so that `std::is_invocable_v<VecOp, V, V>` reports false for element types
// that lack a NEON overload (e.g. vmulq has no int64 form, vdivq is float
// only), routing those types to the scalar generic path at compile time.
struct vec_add
{
    template <typename V>
    auto operator()(V a, V b) const -> decltype(vaddq(a, b)) { return vaddq(a, b); }
};
struct vec_sub
{
    template <typename V>
    auto operator()(V a, V b) const -> decltype(vsubq(a, b)) { return vsubq(a, b); }
};
struct vec_mul
{
    template <typename V>
    auto operator()(V a, V b) const -> decltype(vmulq(a, b)) { return vmulq(a, b); }
};
struct vec_div
{
    template <typename V>
    auto operator()(V a, V b) const -> decltype(vdivq(a, b)) { return vdivq(a, b); }
};

// The scalar remainder is an inline loop rather than a recursive call back
// into `generic::transform_binary`: the old recursive form corrupted the
// stack for inputs shorter than one SIMD lane (issue #635 / PR #641).
template <typename T, typename ScalarOp, typename VecOp>
void transform_binary(T * dest, T const * dest_end, T const * src1, T const * src2, ScalarOp scalar_op, VecOp vec_op)
{
    // The two checks are nested rather than combined with `||` because
    // `if constexpr` does not suppress template substitution of its
    // subexpressions; naming `type::vector_t<T>` in a single combined
    // condition would instantiate it for element types without a NEON
    // vector binding (e.g. bool, Complex<T>), breaking compilation.
    if constexpr (!type::has_vectype<T>)
    {
        generic::transform_binary<T>(dest, dest_end, src1, src2, scalar_op);
    }
    else
    {
        using vec_t = type::vector_t<T>;
        if constexpr (!std::is_invocable_v<VecOp, vec_t, vec_t>)
        {
            generic::transform_binary<T>(dest, dest_end, src1, src2, scalar_op);
        }
        else
        {
            constexpr size_t N_lane = type::vector_lane<T>;

#ifndef NDEBUG
            constexpr size_t alignment = detail::get_recommended_alignment();
            detail::check_alignment(dest, alignment, "transform_binary dest");
            detail::check_alignment(src1, alignment, "transform_binary src1");
            detail::check_alignment(src2, alignment, "transform_binary src2");
#endif

            // Remaining-length compare avoids forming `dest_end - N_lane`,
            // which is UB (pointer before the buffer) when the input is
            // shorter than one SIMD lane. Both pointers lie inside the
            // buffer so `dest_end - ptr` is well-defined.
            T * ptr = dest;
            while (static_cast<size_t>(dest_end - ptr) >= N_lane)
            {
                vec_t v1 = vld1q(src1);
                vec_t v2 = vld1q(src2);
                vst1q(ptr, vec_op(v1, v2));
                ptr += N_lane;
                src1 += N_lane;
                src2 += N_lane;
            }
            while (ptr < dest_end)
            {
                *ptr = scalar_op(*src1, *src2);
                ++ptr;
                ++src1;
                ++src2;
            }
        }
    }
}

template <typename T>
inline void add(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    transform_binary<T>(dest, dest_end, src1, src2, std::plus<T>{}, vec_add{});
}

template <typename T>
inline void sub(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    transform_binary<T>(dest, dest_end, src1, src2, std::minus<T>{}, vec_sub{});
}

template <typename T>
inline void mul(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    transform_binary<T>(dest, dest_end, src1, src2, std::multiplies<T>{}, vec_mul{});
}

template <typename T>
inline void div(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    transform_binary<T>(dest, dest_end, src1, src2, std::divides<T>{}, vec_div{});
}

template <typename T, typename std::enable_if_t<!type::has_vectype<T>> * = nullptr>
const T * check_between(T const * start, T const * end, T const & min_val, T const & max_val)
{
    return generic::check_between<T>(start, end, min_val, max_val);
}

template <typename T, typename std::enable_if_t<type::has_vectype<T>> * = nullptr>
const T * check_between(T const * start, T const * end, T const & min_val, T const & max_val)
{
    using vec_t = type::vector_t<T>;
    using cmpvec_t = type::vector_t<uint64_t>;
    constexpr size_t N_lane = type::vector_lane<T>;

#ifndef NDEBUG
    constexpr size_t alignment = detail::get_recommended_alignment();
    detail::check_alignment(start, alignment, "check_between start");
#endif

    vec_t max_vec = vdupq(max_val);
    vec_t min_vec = vdupq(min_val);

    // Remaining-length compare avoids forming `end - N_lane` when the input
    // is shorter than one SIMD lane (pointer before the buffer is UB).
    T const * ptr = start;
    while (static_cast<size_t>(end - ptr) >= N_lane)
    {
        vec_t data_vec = vld1q(ptr);

        // Both bounds must be checked in the same block before picking a
        // winner: scanning `>= max` first and only falling back to `< min`
        // when no lane matched would report a later too-large lane ahead
        // of an earlier too-small lane in the same vector, giving the
        // wrong index back to callers like `take_along_axis_simd` that
        // build diagnostics from the returned pointer.
        cmpvec_t ge_vec = (cmpvec_t)vcgeq(data_vec, max_vec);
        cmpvec_t lt_vec = (cmpvec_t)vcltq(data_vec, min_vec);
        bool out_of_range = vgetq<0>(ge_vec) || vgetq<1>(ge_vec) || vgetq<0>(lt_vec) || vgetq<1>(lt_vec);

        if (out_of_range)
        {
            T ge_val[N_lane] = {};
            T lt_val[N_lane] = {};
            vst1q(ge_val, ge_vec);
            vst1q(lt_val, lt_vec);
            for (size_t i = 0; i < N_lane; ++i)
            {
                if (ge_val[i] || lt_val[i])
                {
                    return ptr + i;
                }
            }
            return ptr;
        }

        ptr += N_lane;
    }

    // Scalar tail: inline loop, no recursion into generic.
    for (; ptr < end; ++ptr)
    {
        if (*ptr < min_val || *ptr > max_val)
        {
            return ptr;
        }
    }
    return nullptr;
}

#else
template <typename T>
const T * check_between(T const * start, T const * end, T const & min_val, T const & max_val)
{
    return generic::check_between<T>(start, end, min_val, max_val);
}

template <typename T>
void add(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    generic::add<T>(dest, dest_end, src1, src2);
}

template <typename T>
void sub(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    generic::sub<T>(dest, dest_end, src1, src2);
}

template <typename T>
void mul(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    generic::mul<T>(dest, dest_end, src1, src2);
}

template <typename T>
void div(T * dest, T const * dest_end, T const * src1, T const * src2)
{
    generic::div<T>(dest, dest_end, src1, src2);
}

#endif /* defined(__aarch64__) */

} /* namespace neon */

} /* namespace simd */

} /* namespace modmesh */
