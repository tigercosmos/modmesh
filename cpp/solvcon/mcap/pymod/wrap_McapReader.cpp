/*
 * Copyright (c) 2026, solvcon team <contact@solvcon.net>
 * BSD 3-Clause License, see COPYING
 */

#include <solvcon/mcap/pymod/mcap_pymod.hpp>
#include <solvcon/solvcon.hpp>

namespace solvcon
{

namespace python
{

class SOLVCON_PYTHON_WRAPPER_VISIBILITY WrapMcapReader
    : public WrapBase<WrapMcapReader, mcap::Reader, std::shared_ptr<mcap::Reader>>
{

public:

    using base_type = WrapBase<WrapMcapReader, mcap::Reader, std::shared_ptr<mcap::Reader>>;
    using wrapped_type = typename base_type::wrapped_type;

    friend root_base_type;

protected:

    WrapMcapReader(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11; // NOLINT(misc-unused-alias-decls)

        (*this)
            .def(
                py::init(
                    [](std::string const & path)
                    { return std::make_shared<mcap::Reader>(path); }),
                py::arg("path"))
            //
            ;

        (*this)
            .def_property_readonly("path", &wrapped_type::path)
            .def("topics", &wrapped_type::topics)
            .def("chunk_count", &wrapped_type::chunk_count)
            .def("time_range", &wrapped_type::time_range)
            .def("has_time_range", &wrapped_type::has_time_range)
            //
            ;
    }

}; /* end class WrapMcapReader */

void wrap_McapReader(pybind11::module & mod)
{
    WrapMcapReader::commit(mod, "McapReader", "McapReader");
}

} /* end namespace python */

} /* end namespace solvcon */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
