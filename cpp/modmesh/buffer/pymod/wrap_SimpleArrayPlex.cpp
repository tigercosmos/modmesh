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

// clang-format off
#include <modmesh/buffer/pymod/buffer_pymod.hpp> // Must be the first include.
// clang-format on

// Define the Pybind11 caster for SimpleArray<T>
namespace pybind11
{
namespace detail
{

#define ARRAYPLEX_TYPE_CASTER(DATATYPE)                                                                                                                           \
    template <> /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                                                                                  \
    struct type_caster<modmesh::python::WrapSimpleArray##DATATYPE>                                                                                                \
    {                                                                                                                                                             \
                                                                                                                                                                  \
    public:                                                                                                                                                       \
        PYBIND11_TYPE_CASTER(modmesh::python::WrapSimpleArray##DATATYPE, _("WrapSimpleArray" #DATATYPE));                                                         \
                                                                                                                                                                  \
        /* Conversion from Python object to C++ */                                                                                                                \
        bool load(pybind11::handle src, bool convert)                                                                                                             \
        {                                                                                                                                                         \
            /* Check if the source object is a valid modmesh::python::SimpleArrayPlex  */                                                                         \
            if (!pybind11::isinstance<modmesh::python::WrapSimpleArrayPlex>(src))                                                                                 \
            {                                                                                                                                                     \
                return false;                                                                                                                                     \
            }                                                                                                                                                     \
                                                                                                                                                                  \
            /* Get the modmesh::python::SimpleArrayPlex object from the source handle */                                                                          \
            modmesh::python::WrapSimpleArrayPlex arrayplex = src.cast<modmesh::python::WrapSimpleArrayPlex>();                                                            \
                                                                                                                                                                  \
            /* Check if the data type is matched */                                                                                                               \
            if (arrayplex.data_type() != modmesh::DataType::DATATYPE)                                                                                             \
            {                                                                                                                                                     \
                return false;                                                                                                                                     \
            }                                                                                                                                                     \
                                                                                                                                                                  \
            /* construct the new array from the arrayplex */                                                                                                      \
            modmesh::python::SimpleArray##DATATYPE * array_from_arrayplex = reinterpret_cast<modmesh::python::SimpleArray##DATATYPE *>(arrayplex.instance_ptr()); \
            value = modmesh::python::SimpleArray##DATATYPE(*array_from_arrayplex);                                                                                \
        }                                                                                                                                                         \
                                                                                                                                                                  \
        /* Conversion from C++ to Python object */                                                                                                                \
        static pybind11::handle cast(const modmesh::python::WrapSimpleArray##DATATYPE & src, pybind11::return_value_policy, pybind11::handle)                     \
        {                                                                                                                                                         \
            /* create an arrayplex from the array */                                                                                                              \
            modmesh::python::SimpleArrayPlex arrayplex(src, modmesh::DataType::DATATYPE);                                                                         \
                                                                                                                                                                  \
            /* Return the Python object representing the converted modmesh::python::SimpleArrayPlex */                                                            \
            return pybind11::cast(arrayplex, pybind11::return_value_policy::move);                                                                                \
        }                                                                                                                                                         \
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

namespace modmesh
{

namespace python
{

WrapSimpleArrayPlex::WrapSimpleArrayPlex(pybind11::module & mod, char const * pyname, char const * pydoc)
    : root_base_type(mod, pyname, pydoc, pybind11::buffer_protocol())
{
    (*this)
        .def_timed(
            pybind11::init(
                [](pybind11::object const & shape, std::string const & datatype)
                { return wrapped_type(make_shape(shape), datatype); }),
            pybind11::arg("shape"),
            pybind11::arg("dtype"))
        /// TODO: should have the same interface as WrapSimpleArray
        ;
}

WrapSimpleArrayPlex::shape_type WrapSimpleArrayPlex::make_shape(pybind11::object const & shape_in)
{
    shape_type shape;
    try
    {
        shape.push_back(shape_in.cast<size_t>());
    }
    catch (const pybind11::cast_error &)
    {
        shape = shape_in.cast<std::vector<size_t>>();
    }
    return shape;
}

void wrap_SimpleArrayPlex(pybind11::module & mod)
{
    namespace py = pybind11; // NOLINT(misc-unused-alias-decls)
    WrapSimpleArrayPlex::commit(mod, "SimpleArray", "SimpleArray");

    // Register the type caster
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayBool>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt8>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt16>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt32>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt64>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint8>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint16>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint32>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint64>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayFloat32>();
    pybind11::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayFloat64>();
}

} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
