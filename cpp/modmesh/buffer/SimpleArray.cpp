/*
 * Copyright (c) 2023, Yung-Yu Chen <yyc@solvcon.net>
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

#include <modmesh/buffer/SimpleArray.hpp>

namespace modmesh
{

DataType get_data_type_from_string(const std::string & data_type)
{
    if (data_type == "bool")
    {
        return DataType::Bool;
    }
    else if (data_type == "int8")
    {
        return DataType::Int8;
    }
    else if (data_type == "int16")
    {
        return DataType::Int16;
    }
    else if (data_type == "int32")
    {
        return DataType::Int32;
    }
    else if (data_type == "int64")
    {
        return DataType::Uint64;
    }
    else if (data_type == "uint8")
    {
        return DataType::Uint8;
    }
    else if (data_type == "uint8")
    {
        return DataType::Uint8;
    }
    else if (data_type == "uint16")
    {
        return DataType::Uint16;
    }
    else if (data_type == "uint32")
    {
        return DataType::Uint32;
    }
    else if (data_type == "uint64")
    {
        return DataType::Uint64;
    }
    else if (data_type == "float32")
    {
        return DataType::Float32;
    }
    else if (data_type == "float64")
    {
        return DataType::Float64;
    }
    throw std::runtime_error("Unsupported datatype");
}

SimpleArrayPlex::SimpleArrayPlex(const shape_type & shape, DataType data_type)
    : m_data_type(data_type)
{
    switch (data_type)
    {
    case DataType::Bool:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayBool(shape));
        break;
    }
    case DataType::Int8:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayInt8(shape));
        break;
    }
    case DataType::Int16:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayInt16(shape));
        break;
    }
    case DataType::Int32:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayInt32(shape));
        break;
    }
    case DataType::Int64:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayInt64(shape));
        break;
    }
    case DataType::Uint8:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayUint8(shape));
        break;
    }
    case DataType::Uint16:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayUint16(shape));
        break;
    }
    case DataType::Uint32:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayUint32(shape));
        break;
    }
    case DataType::Uint64:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayUint64(shape));
        break;
    }
    case DataType::Float32:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayFloat32(shape));
        break;
    }
    case DataType::Float64:
    {
        m_instance_ptr = reinterpret_cast<void *>(new SimpleArrayFloat64(shape));
        break;
    }
    default:
    {
        throw std::runtime_error("Unsupported datatype");
    }
    }
}

SimpleArrayPlex::~SimpleArrayPlex()
{
    if (m_instance_ptr == nullptr)
    {
        return;
    }

    switch (m_data_type)
    {
    case DataType::Bool:
    {
        delete reinterpret_cast<SimpleArrayBool *>(m_instance_ptr);
        break;
    }
    case DataType::Int8:
    {
        delete reinterpret_cast<SimpleArrayInt8 *>(m_instance_ptr);
        break;
    }
    case DataType::Int16:
    {
        delete reinterpret_cast<SimpleArrayInt16 *>(m_instance_ptr);
        break;
    }
    case DataType::Int32:
    {
        delete reinterpret_cast<SimpleArrayInt32 *>(m_instance_ptr);
        break;
    }
    case DataType::Int64:
    {
        delete reinterpret_cast<SimpleArrayInt64 *>(m_instance_ptr);
        break;
    }
    case DataType::Uint8:
    {
        delete reinterpret_cast<SimpleArrayUint8 *>(m_instance_ptr);
        break;
    }
    case DataType::Uint16:
    {
        delete reinterpret_cast<SimpleArrayUint16 *>(m_instance_ptr);
        break;
    }
    case DataType::Uint32:
    {
        delete reinterpret_cast<SimpleArrayUint32 *>(m_instance_ptr);
        break;
    }
    case DataType::Uint64:
    {
        delete reinterpret_cast<SimpleArrayUint64 *>(m_instance_ptr);
        break;
    }
    case DataType::Float32:
    {
        delete reinterpret_cast<SimpleArrayFloat32 *>(m_instance_ptr);
        break;
    }
    case DataType::Float64:
    {
        delete reinterpret_cast<SimpleArrayFloat64 *>(m_instance_ptr);
        break;
    }
    default:
        break;
    }

    m_instance_ptr = nullptr;
}

} // namespace modmesh
