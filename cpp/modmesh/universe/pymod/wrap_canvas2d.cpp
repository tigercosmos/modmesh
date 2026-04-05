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

#include <modmesh/universe/pymod/universe_pymod.hpp>

#include <modmesh/universe/canvas2d.hpp>

#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace modmesh
{

namespace python
{

// TODO: EditAction enum and EditRecord struct bindings can be added here
//       when edit history is implemented.

// ---------------------------------------------------------------------------
// Shape2D base + concrete shapes
// ---------------------------------------------------------------------------

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapShape2D
    : public WrapBase<WrapShape2D<T>, Shape2D<T>, std::shared_ptr<Shape2D<T>>>
{

public:

    using base_type = WrapBase<WrapShape2D<T>, Shape2D<T>, std::shared_ptr<Shape2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;
    using point_type = typename wrapped_type::point_type;

    friend typename base_type::root_base_type;

protected:

    WrapShape2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def_property_readonly("id", &wrapped_type::id)
            .def_property_readonly("closed", &wrapped_type::closed)
            .def_property_readonly("nvertex", &wrapped_type::nvertex)
            .def("vertex", &wrapped_type::vertex, py::arg("i"))
            .def("translate", &wrapped_type::translate, py::arg("dx"), py::arg("dy"))
            .def("scale", &wrapped_type::scale, py::arg("factor"), py::arg("cx") = 0, py::arg("cy") = 0)
            .def("rotate", &wrapped_type::rotate, py::arg("angle_deg"), py::arg("cx") = 0, py::arg("cy") = 0)
            .def("type_name", &wrapped_type::type_name)
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapPolygon2D
    : public WrapBase<WrapPolygon2D<T>, Polygon2D<T>, std::shared_ptr<Polygon2D<T>>>
{

public:

    using base_type = WrapBase<WrapPolygon2D<T>, Polygon2D<T>, std::shared_ptr<Polygon2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;
    using point_type = typename wrapped_type::point_type;

    friend typename base_type::root_base_type;

protected:

    WrapPolygon2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<std::vector<point_type>>(), py::arg("vertices"))
            .def("set_vertices", &wrapped_type::set_vertices, py::arg("vertices"))
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapRectangle2D
    : public WrapBase<WrapRectangle2D<T>, Rectangle2D<T>, std::shared_ptr<Rectangle2D<T>>>
{

public:

    using base_type = WrapBase<WrapRectangle2D<T>, Rectangle2D<T>, std::shared_ptr<Rectangle2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;

    friend typename base_type::root_base_type;

protected:

    WrapRectangle2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<T, T, T, T>(), py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"))
            .def_property_readonly("x", &wrapped_type::x)
            .def_property_readonly("y", &wrapped_type::y)
            .def_property("width", &wrapped_type::width, &wrapped_type::set_width)
            .def_property("height", &wrapped_type::height, &wrapped_type::set_height)
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapTriangle2D
    : public WrapBase<WrapTriangle2D<T>, Triangle2D<T>, std::shared_ptr<Triangle2D<T>>>
{

public:

    using base_type = WrapBase<WrapTriangle2D<T>, Triangle2D<T>, std::shared_ptr<Triangle2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;

    friend typename base_type::root_base_type;

protected:

    WrapTriangle2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<T, T, T, T, T, T>(),
                 py::arg("x0"),
                 py::arg("y0"),
                 py::arg("x1"),
                 py::arg("y1"),
                 py::arg("x2"),
                 py::arg("y2"))
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapCircle2D
    : public WrapBase<WrapCircle2D<T>, Circle2D<T>, std::shared_ptr<Circle2D<T>>>
{

public:

    using base_type = WrapBase<WrapCircle2D<T>, Circle2D<T>, std::shared_ptr<Circle2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;

    friend typename base_type::root_base_type;

protected:

    WrapCircle2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<T, T, T, size_t>(),
                 py::arg("cx"),
                 py::arg("cy"),
                 py::arg("radius"),
                 py::arg("nseg") = 64)
            .def_property_readonly("cx", &wrapped_type::cx)
            .def_property_readonly("cy", &wrapped_type::cy)
            .def_property("radius", &wrapped_type::radius, &wrapped_type::set_radius)
            .def_property_readonly("nseg", &wrapped_type::nseg)
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapLine2D
    : public WrapBase<WrapLine2D<T>, Line2D<T>, std::shared_ptr<Line2D<T>>>
{

public:

    using base_type = WrapBase<WrapLine2D<T>, Line2D<T>, std::shared_ptr<Line2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;

    friend typename base_type::root_base_type;

protected:

    WrapLine2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<T, T, T, T>(),
                 py::arg("x0"),
                 py::arg("y0"),
                 py::arg("x1"),
                 py::arg("y1"))
            .def_property_readonly("x0", &wrapped_type::x0)
            .def_property_readonly("y0", &wrapped_type::y0)
            .def_property_readonly("x1", &wrapped_type::x1)
            .def_property_readonly("y1", &wrapped_type::y1)
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapPolyline2D
    : public WrapBase<WrapPolyline2D<T>, Polyline2D<T>, std::shared_ptr<Polyline2D<T>>>
{

public:

    using base_type = WrapBase<WrapPolyline2D<T>, Polyline2D<T>, std::shared_ptr<Polyline2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;
    using point_type = typename wrapped_type::point_type;

    friend typename base_type::root_base_type;

protected:

    WrapPolyline2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<std::vector<point_type>>(), py::arg("vertices"))
            .def("set_vertices", &wrapped_type::set_vertices, py::arg("vertices"))
            //
            ;
    }
};

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapEllipse2D
    : public WrapBase<WrapEllipse2D<T>, Ellipse2D<T>, std::shared_ptr<Ellipse2D<T>>>
{

public:

    using base_type = WrapBase<WrapEllipse2D<T>, Ellipse2D<T>, std::shared_ptr<Ellipse2D<T>>>;
    using wrapped_type = typename base_type::wrapped_type;

    friend typename base_type::root_base_type;

protected:

    WrapEllipse2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            .def(py::init<T, T, T, T, size_t>(),
                 py::arg("cx"),
                 py::arg("cy"),
                 py::arg("a"),
                 py::arg("b"),
                 py::arg("nseg") = 64)
            .def_property_readonly("cx", &wrapped_type::cx)
            .def_property_readonly("cy", &wrapped_type::cy)
            .def_property("a", &wrapped_type::a, &wrapped_type::set_a)
            .def_property("b", &wrapped_type::b, &wrapped_type::set_b)
            .def_property_readonly("nseg", &wrapped_type::nseg)
            //
            ;
    }
};

// ---------------------------------------------------------------------------
// Canvas2D (accessed via world.canvas())
// ---------------------------------------------------------------------------

template <typename T>
class MODMESH_PYTHON_WRAPPER_VISIBILITY WrapCanvas2D
    : public WrapBase<WrapCanvas2D<T>, Canvas2D<T>>
{

public:

    using base_type = WrapBase<WrapCanvas2D<T>, Canvas2D<T>>;
    using wrapped_type = typename base_type::wrapped_type;
    using point_type = typename wrapped_type::point_type;
    using shape_ptr = typename wrapped_type::shape_ptr;

    friend typename base_type::root_base_type;

protected:

    WrapCanvas2D(pybind11::module & mod, char const * pyname, char const * pydoc)
        : base_type(mod, pyname, pydoc)
    {
        namespace py = pybind11;

        (*this)
            // Create
            .def("add", &wrapped_type::add, py::arg("shape"))
            .def("add_polygon", &wrapped_type::add_polygon, py::arg("vertices"))
            .def("add_rectangle", &wrapped_type::add_rectangle, py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"))
            .def("add_triangle", &wrapped_type::add_triangle, py::arg("x0"), py::arg("y0"), py::arg("x1"), py::arg("y1"), py::arg("x2"), py::arg("y2"))
            .def("add_circle", &wrapped_type::add_circle, py::arg("cx"), py::arg("cy"), py::arg("radius"), py::arg("nseg") = 64)
            .def("add_line", &wrapped_type::add_line, py::arg("x0"), py::arg("y0"), py::arg("x1"), py::arg("y1"))
            .def("add_polyline", &wrapped_type::add_polyline, py::arg("vertices"))
            .def("add_ellipse", &wrapped_type::add_ellipse, py::arg("cx"), py::arg("cy"), py::arg("a"), py::arg("b"), py::arg("nseg") = 64)
            // Read
            .def("get", &wrapped_type::get, py::arg("shape_id"))
            .def_property_readonly("nshape", &wrapped_type::nshape)
            // Delete
            .def("remove", &wrapped_type::remove, py::arg("shape_id"))
            .def("clear", &wrapped_type::clear)
            // Render
            .def("commit", &wrapped_type::commit)
            // TODO: edit history bindings can be added here.
            //
            ;
    }
};

// ---------------------------------------------------------------------------
// Registration entry point
// ---------------------------------------------------------------------------

void wrap_canvas2d(pybind11::module & mod)
{
    // Shape2D base (abstract -- needed for polymorphic return types)
    WrapShape2D<float>::commit(mod, "Shape2DFp32", "Shape2DFp32");
    WrapShape2D<double>::commit(mod, "Shape2DFp64", "Shape2DFp64");

    // Concrete shapes
    WrapPolygon2D<float>::commit(mod, "Polygon2DFp32", "Polygon2DFp32");
    WrapPolygon2D<double>::commit(mod, "Polygon2DFp64", "Polygon2DFp64");

    WrapRectangle2D<float>::commit(mod, "Rectangle2DFp32", "Rectangle2DFp32");
    WrapRectangle2D<double>::commit(mod, "Rectangle2DFp64", "Rectangle2DFp64");

    WrapTriangle2D<float>::commit(mod, "Triangle2DFp32", "Triangle2DFp32");
    WrapTriangle2D<double>::commit(mod, "Triangle2DFp64", "Triangle2DFp64");

    WrapCircle2D<float>::commit(mod, "Circle2DFp32", "Circle2DFp32");
    WrapCircle2D<double>::commit(mod, "Circle2DFp64", "Circle2DFp64");

    WrapLine2D<float>::commit(mod, "Line2DFp32", "Line2DFp32");
    WrapLine2D<double>::commit(mod, "Line2DFp64", "Line2DFp64");

    WrapPolyline2D<float>::commit(mod, "Polyline2DFp32", "Polyline2DFp32");
    WrapPolyline2D<double>::commit(mod, "Polyline2DFp64", "Polyline2DFp64");

    WrapEllipse2D<float>::commit(mod, "Ellipse2DFp32", "Ellipse2DFp32");
    WrapEllipse2D<double>::commit(mod, "Ellipse2DFp64", "Ellipse2DFp64");

    // Canvas2D is not directly constructed -- accessed via world.canvas().
    // Register it so pybind11 knows the type for return values.
    WrapCanvas2D<float>::commit(mod, "Canvas2DFp32", "Canvas2DFp32");
    WrapCanvas2D<double>::commit(mod, "Canvas2DFp64", "Canvas2DFp64");
}

} /* end namespace python */

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
