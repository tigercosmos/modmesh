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

#include <modmesh/buffer/buffer.hpp>
#include <modmesh/buffer/pymod/buffer_pymod.hpp> // Must be the first include.

// Define the Pybind11 caster for SimpleArray<T>
namespace pybind11
{
namespace detail
{

#define ARRAYPLEX_TYPE_CASTER(DATATYPE)                                                                                                           \
    template <>                                                                                                                                   \
    struct type_caster<modmesh::SimpleArray##DATATYPE>                                                                                            \
    {                                                                                                                                             \
    public:                                                                                                                                       \
        PYBIND11_TYPE_CASTER(modmesh::SimpleArray##DATATYPE, _("SimpleArray" #DATATYPE));                                                         \
                                                                                                                                                  \
        /* Conversion from Python object to C++ */                                                                                                \
        bool load(py::handle src, bool convert)                                                                                                   \
        {                                                                                                                                         \
            /* Check if the source object is a valid modmesh::SimpleArrayPlex  */                                                                 \
            if (!py::isinstance<modmesh::SimpleArrayPlex>(src))                                                                                   \
            {                                                                                                                                     \
                return false;                                                                                                                     \
            }                                                                                                                                     \
                                                                                                                                                  \
            /* Get the modmesh::SimpleArrayPlex object from the source handle */                                                                  \
            modmesh::SimpleArrayPlex arrayplex = src.cast<modmesh::SimpleArrayPlex>();                                                            \
                                                                                                                                                  \
            /* Check if the data type is matched */                                                                                               \
            if (arrayplex.data_type() != modmesh::DataType::DATATYPE)                                                                             \
            {                                                                                                                                     \
                return false;                                                                                                                     \
            }                                                                                                                                     \
                                                                                                                                                  \
            /* construct the new array from the arrayplex */                                                                                      \
            modmesh::SimpleArray##DATATYPE * array_from_arrayplex = reinterpret_cast<modmesh::SimpleArray##DATATYPE *>(arrayplex.instance_ptr()); \
            value = modmesh::SimpleArray##DATATYPE(*array_from_arrayplex);                                                                        \
        }                                                                                                                                         \
                                                                                                                                                  \
        /* Conversion from C++ to Python object */                                                                                                \
        static py::handle cast(const ArrayInt & src, py::return_value_policy, py::handle)                                                         \
        {                                                                                                                                         \
            /* create an arrayplex from the array */                                                                                              \
            modmesh::SimpleArrayPlex arrayplex(src, modmesh::DataType::DATATYPE);                                                                 \
                                                                                                                                                  \
            /* Return the Python object representing the converted modmesh::SimpleArrayPlex */                                                    \
            return py::cast(arrayplex, py::return_value_policy::move);                                                                            \
        }                                                                                                                                         \
    };

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

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapSimpleArrayPlex
    : public WrapBase<WrapSimpleArrayPlex, SimpleArrayPlex<T>>
{

    using root_base_type = WrapBase<WrapSimpleArrayPlex, SimpleArrayPlex>;
    using wrapped_type = typename root_base_type::wrapped_type;
    using wrapper_type = typename root_base_type::wrapper_type;
    using value_type = typename wrapped_type::value_type;
    using shape_type = typename wrapped_type::shape_type;
    using slice_type = small_vector<int>;

    friend root_base_type;

    WrapSimpleArrayPlex(pybind11::module & mod, char const * pyname, char const * pydoc)
        : root_base_type(mod, pyname, pydoc, pybind11::buffer_protocol())
    {
        namespace py = pybind11;

        (*this)
            .def_timed(
                py::init(
                    [](py::object const & shape, std::string const & datatype)
                    { return wrapped_type(make_shape(shape), datatype); }),
                py::arg("shape"),
                py::arg("dtype"))
            /// TODO: should have the same interface as WrapSimpleArray
            ;
    }

    static shape_type make_shape(pybind11::object const & shape_in)
    {
        namespace py = pybind11; // NOLINT(misc-unused-alias-decls)
        shape_type shape;
        try
        {
            shape.push_back(shape_in.cast<size_t>());
        }
        catch (const py::cast_error &)
        {
            shape = shape_in.cast<std::vector<size_t>>();
        }
        return shape;
    }
}

void
wrap_SimpleArrayPlex(pybind11::module & mod)
{
    WrapSimpleArrayPlex::commit(mod, "SimpleArray", "SimpleArray");

    // Register the type caster
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayBool>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt8>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt16>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt32>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayInt64>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint8>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint16>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint32>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayUint64>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayFloat32>();
    py::implicitly_convertible<WrapArrayPlex, WrapSimpleArrayFloat64>();
}
}

} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
