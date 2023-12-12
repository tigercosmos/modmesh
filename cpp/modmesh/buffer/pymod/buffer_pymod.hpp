#pragma once

/*
 * Copyright (c) 2022, Yung-Yu Chen <yyc@solvcon.net>
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

#include <pybind11/pybind11.h> // Must be the first include.
#include <pybind11/stl.h>

#include <modmesh/python/common.hpp>

namespace modmesh
{

namespace python
{

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapSimpleArray
    : public WrapBase<WrapSimpleArray<T>, SimpleArray<T>>
{

    using root_base_type = WrapBase<WrapSimpleArray<T>, SimpleArray<T>>;
    using wrapped_type = typename root_base_type::wrapped_type;
    using wrapper_type = typename root_base_type::wrapper_type;
    using value_type = typename wrapped_type::value_type;
    using shape_type = typename wrapped_type::shape_type;
    using slice_type = small_vector<int>;

    friend root_base_type;

    WrapSimpleArray(pybind11::module & mod, char const * pyname, char const * pydoc);

    wrapper_type & wrap_modifiers();

    wrapper_type & wrap_calculators();

    static void setitem_parser(wrapped_type & arr_out, pybind11::args const & args);

    static void copy_slice(slice_type & slice_out, pybind11::slice const & slice_in);
    static std::vector<slice_type> make_default_slices(wrapped_type const & arr);

    static void process_slices(pybind11::tuple const & tuple,
                               std::vector<slice_type> & slices,
                               size_t ndim);

    static void slice_syntax_check(pybind11::tuple const & tuple, size_t ndim);
    static void broadcast_array_using_slice(wrapped_type & arr_out,
                                            std::vector<slice_type> const & slices,
                                            pybind11::array const & arr_in);

    static void broadcast_array_using_ellipsis(wrapped_type & arr_out, pybind11::array const & arr_in);

    static shape_type make_shape(pybind11::object const & shape_in);
}; /* end class WrapSimpleArray */

using WrapSimpleArrayBool = WrapSimpleArray<bool>;
using WrapSimpleArrayInt8 = WrapSimpleArray<int8_t>;
using WrapSimpleArrayInt16 = WrapSimpleArray<int16_t>;
using WrapSimpleArrayInt32 = WrapSimpleArray<int32_t>;
using WrapSimpleArrayInt64 = WrapSimpleArray<int64_t>;
using WrapSimpleArrayUint8 = WrapSimpleArray<uint8_t>;
using WrapSimpleArrayUint16 = WrapSimpleArray<uint16_t>;
using WrapSimpleArrayUint32 = WrapSimpleArray<uint32_t>;
using WrapSimpleArrayUint64 = WrapSimpleArray<uint64_t>;
using WrapSimpleArrayFloat32 = WrapSimpleArray<float>;
using WrapSimpleArrayFloat64 = WrapSimpleArray<double>;

class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapSimpleArrayPlex : public WrapBase<WrapSimpleArrayPlex, SimpleArrayPlex>
{
    using root_base_type = WrapBase<WrapSimpleArrayPlex, SimpleArrayPlex>;
    using wrapped_type = typename root_base_type::wrapped_type;
    using wrapper_type = typename root_base_type::wrapper_type;
    using shape_type = modmesh::detail::shape_type;

    friend root_base_type;

    WrapSimpleArrayPlex(pybind11::module & mod, char const * pyname, char const * pydoc);

    static shape_type make_shape(pybind11::object const & shape_in);
}; /* end of class WrapSimpleArrayPlex*/

void initialize_buffer(pybind11::module & mod);
void wrap_ConcreteBuffer(pybind11::module & mod);
void wrap_SimpleArray(pybind11::module & mod);
void wrap_SimpleArrayPlex(pybind11::module & mod);

} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
