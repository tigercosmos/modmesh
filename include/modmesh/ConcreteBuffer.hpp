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

#include "modmesh/small_vector.hpp"

#include <stdexcept>
#include <memory>

namespace modmesh
{

/**
 * Untyped and unresizeable memory buffer for contiguous data storage.
 */
class ConcreteBuffer
  : public std::enable_shared_from_this<ConcreteBuffer>
{

private:

    struct ctor_passkey {};

public:

    static std::shared_ptr<ConcreteBuffer> construct(size_t nbytes)
    {
        return std::make_shared<ConcreteBuffer>(nbytes, ctor_passkey());
    }

    static std::shared_ptr<ConcreteBuffer> construct() { return construct(0); }

    std::shared_ptr<ConcreteBuffer> clone() const
    {
        std::shared_ptr<ConcreteBuffer> ret = construct(nbytes());
        std::copy_n(data(), size(), (*ret).data());
        return ret;
    }

    /**
     * \param[in] length Memory buffer length.
     */
    ConcreteBuffer(size_t nbytes, const ctor_passkey &)
      : m_nbytes(nbytes)
      , m_data(allocate(nbytes))
    {}

    ~ConcreteBuffer() = default;

    ConcreteBuffer() = delete;
    ConcreteBuffer(ConcreteBuffer &&) = delete;
    ConcreteBuffer & operator=(ConcreteBuffer &&) = delete;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
    // Avoid enabled_shared_from_this copy constructor
    // NOLINTNEXTLINE(bugprone-copy-constructor-init)
    ConcreteBuffer(ConcreteBuffer const & other)
      : m_nbytes(other.m_nbytes)
      , m_data(allocate(other.m_nbytes))
    {
        if (size() != other.size())
        {
            throw std::out_of_range("Buffer size mismatch");
        }
        std::copy_n(other.data(), size(), data());
    }
#pragma GCC diagnostic pop

    ConcreteBuffer & operator=(ConcreteBuffer const & other)
    {
        if (this != &other)
        {
            if (size() != other.size())
            {
                throw std::out_of_range("Buffer size mismatch");
            }
            std::copy_n(other.data(), size(), data());
        }
        return *this;
    }

    explicit operator bool() const noexcept { return bool(m_data); }

    size_t nbytes() const noexcept { return m_nbytes; }
    size_t size() const noexcept { return nbytes(); }

    using iterator = int8_t *;
    using const_iterator = int8_t const *;

    iterator begin() noexcept { return data(); }
    iterator end() noexcept { return data() + size(); }
    const_iterator begin() const noexcept { return data(); }
    const_iterator end() const noexcept { return data() + size(); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }

    int8_t   operator[](size_t it) const { return data(it); }
    int8_t & operator[](size_t it)       { return data(it); }

    int8_t   at(size_t it) const { validate_range(it); return data(it); }
    int8_t & at(size_t it)       { validate_range(it); return data(it); }

    /* Backdoor */
    int8_t   data(size_t it) const { return data()[it]; }
    int8_t & data(size_t it)       { return data()[it]; }
    int8_t const * data() const noexcept { return data<int8_t>(); }
    int8_t       * data()       noexcept { return data<int8_t>(); }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    template<typename T> T const * data() const noexcept { return reinterpret_cast<T*>(m_data.get()); }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    template<typename T> T       * data()       noexcept { return reinterpret_cast<T*>(m_data.get()); }

private:

    // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
    using unique_ptr_type = std::unique_ptr<int8_t, std::default_delete<int8_t[]>>;
    static_assert(sizeof(size_t) == sizeof(unique_ptr_type), "sizeof(Buffer::m_data) must be a word");

    void validate_range(size_t it) const
    {
        if (it >= size())
        {
            std::ostringstream ms;
            ms << "ConcreteBuffer: index " << it << " is out of bounds with size " << size();
            throw std::out_of_range(ms.str());
        }
    }

    static unique_ptr_type allocate(size_t nbytes)
    {
        unique_ptr_type ret;
        if (0 != nbytes)
        {
            ret = unique_ptr_type(new int8_t[nbytes]);
        }
        else
        {
            ret = unique_ptr_type();
        }
        return ret;
    }

    size_t m_nbytes;
    unique_ptr_type m_data;

}; /* end class ConcreteBuffer */

} /* end namespace modmesh */

/* vim: set et ts=4 sw=4: */
