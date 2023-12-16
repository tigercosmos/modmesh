#pragma once
/*
 * Copyright (c) 2019, Yung-Yu Chen <yyc@solvcon.net>
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

#include <modmesh/buffer/buffer.hpp>
#include <pybind11/pybind11.h>

// Define the Pybind11 caster for SimpleArray<T>
namespace pybind11
{
namespace detail
{

#define ARRAYPLEX_TYPE_CASTER(DATATYPE)                                                                                                                       \
    template <> /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                                                                              \
    struct type_caster<modmesh::SimpleArray##DATATYPE>                                                                                                        \
    {                                                                                                                                                         \
                                                                                                                                                              \
    public:                                                                                                                                                   \
        PYBIND11_TYPE_CASTER(modmesh::SimpleArray##DATATYPE, const_name("SimpleArray" #DATATYPE));                                                            \
                                                                                                                                                              \
        /* Conversion from Python object to C++ */                                                                                                            \
        bool load(pybind11::handle src, bool convert)                                                                                                         \
        {                                                                                                                                                     \
            /* Check if the source object is a valid SimpleArrayPlex  */                                                                                      \
            if (!pybind11::isinstance<modmesh::SimpleArrayPlex>(src))                                                                                         \
            {                                                                                                                                                 \
                return false;                                                                                                                                 \
            }                                                                                                                                                 \
                                                                                                                                                              \
            /* Get the SimpleArrayPlex object from the source handle */                                                                                       \
            modmesh::SimpleArrayPlex arrayplex = src.cast<modmesh::SimpleArrayPlex>();                                                                        \
                                                                                                                                                              \
            /* Check if the data type is matched */                                                                                                           \
            if (arrayplex.data_type() != modmesh::DataType::DATATYPE)                                                                                         \
            {                                                                                                                                                 \
                return false;                                                                                                                                 \
            }                                                                                                                                                 \
                                                                                                                                                              \
            /* construct the new array from the arrayplex */                                                                                                  \
            const modmesh::SimpleArray##DATATYPE * array_from_arrayplex = reinterpret_cast<const modmesh::SimpleArray##DATATYPE *>(arrayplex.instance_ptr()); \
            value = std::move(modmesh::SimpleArray##DATATYPE(*array_from_arrayplex));                                                                         \
            return true;                                                                                                                                      \
        }                                                                                                                                                     \
                                                                                                                                                              \
        /* Conversion from C++ to Python object */                                                                                                            \
        static pybind11::handle cast(const modmesh::SimpleArray##DATATYPE & src, pybind11::return_value_policy, pybind11::handle)                             \
        {                                                                                                                                                     \
            /* create an arrayplex from the array */                                                                                                          \
            modmesh::SimpleArrayPlex arrayplex(src);                                                                                                          \
                                                                                                                                                              \
            /* Return the Python object representing the converted SimpleArrayPlex */                                                                         \
            return pybind11::cast(arrayplex, pybind11::return_value_policy::move);                                                                            \
        }                                                                                                                                                     \
    }

ARRAYPLEX_TYPE_CASTER(Bool);
ARRAYPLEX_TYPE_CASTER(Int8);
ARRAYPLEX_TYPE_CASTER(Int16);
ARRAYPLEX_TYPE_CASTER(Int32);
ARRAYPLEX_TYPE_CASTER(Int64);
ARRAYPLEX_TYPE_CASTER(Uint8);
ARRAYPLEX_TYPE_CASTER(Uint16);
ARRAYPLEX_TYPE_CASTER(Uint32);
ARRAYPLEX_TYPE_CASTER(Uint64);
ARRAYPLEX_TYPE_CASTER(Float32);
ARRAYPLEX_TYPE_CASTER(Float64);

} // namespace detail
} // namespace pybind11

// vim: set ff=unix fenc=utf8 nobomb et sw=4 ts=4 sts=4:
