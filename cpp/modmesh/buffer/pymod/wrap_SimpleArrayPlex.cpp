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

template <>
struct type_caster<modmesh::SimpleArrayInt32>
{
public:
    PYBIND11_TYPE_CASTER(modmesh::SimpleArrayInt32, _("SimpleArrayInt32"));

    // Conversion from Python object to C++
    bool load(py::handle src, bool convert)
    {
        // Check if the source object is a valid modmesh::SimpleArrayPlex
        if (!py::isinstance<modmesh::SimpleArrayPlex>(src))
        {
            return false;
        }

        // Get the modmesh::SimpleArrayPlex object from the source handle
        modmesh::SimpleArrayPlex arrayplex = src.cast<modmesh::SimpleArrayPlex>();

        // Check if the data type is "int32"
        if (arrayplex.data_type() != modmesh::SimpleArrayPlex::DataType::Int32)
        {
            return false;
        }

        // construct the new array from the arrayplex
        modmesh::SimpleArrayInt32 * array_from_arrayplex = reinterpret_cast<modmesh::SimpleArrayInt32 *>(arrayplex.instance_ptr());
        value = modmesh::SimpleArrayInt32(*array_from_arrayplex);
    }

    // Conversion from C++ to Python object
    static py::handle cast(const ArrayInt & src, py::return_value_policy, py::handle)
    {
        // create an arrayplex from the array
        modmesh::SimpleArrayPlex arrayplex(src, modmesh::SimpleArrayPlex::DataType::Int32);

        // Return the Python object representing the converted modmesh::SimpleArrayPlex
        return py::cast(arrayplex, py::return_value_policy::move);
    }
};

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
}
} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
