#pragma once

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

/**
 * @file High-level 2D drawing API with CRUD and edit-history tracing.
 *
 * Architecture:
 *
 *   Canvas2D<T>         shape registry + add/remove + edit history
 *       ^  world->canvas()
 *       |
 *   World<T>            owns Canvas2D, holds low-level geometry
 *       |  widget.updateWorld(world)
 *       v
 *   R3DWidget           Qt3D rendering
 *
 * Usage:
 *   Canvas2D<T>& canvas = world->canvas();
 *   canvas.add_rectangle(0, 0, 10, 5);
 *   auto shape = canvas.get(0);
 *   shape->translate(5, 3);          // transforms live on the shape
 *   canvas.commit();                 // rebuilds world segments
 *
 * Every mutation is recorded as an EditRecord so the full edit history
 * can be inspected or replayed.
 */

#include <modmesh/universe/World.hpp>
#include <modmesh/universe/coord.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace modmesh
{

// ---------------------------------------------------------------------------
// Shape2D base class
// ---------------------------------------------------------------------------

/**
 * Abstract base for every 2D shape managed by Canvas2D.
 *
 * All concrete shapes must implement the pure-virtual methods below.  Shapes
 * live on the z = 0 plane; Point3d is used for compatibility with the
 * existing rendering pipeline.
 */
template <typename T>
class Shape2D
    : public std::enable_shared_from_this<Shape2D<T>>
{

public:

    using value_type = T;
    using point_type = Point3d<T>;

    virtual ~Shape2D() = default;

    int32_t id() const { return m_id; }
    void set_id(int32_t v) { m_id = v; }
    bool closed() const { return m_closed; }

    /// Number of vertices that define the shape boundary.
    virtual size_t nvertex() const = 0;

    /// Return the *i*-th vertex (bounds-checked).
    virtual point_type vertex(size_t i) const = 0;

    // -- transforms ---------------------------------------------------------

    virtual void translate(T dx, T dy) = 0;
    virtual void scale(T factor, T cx = 0, T cy = 0) = 0;
    virtual void rotate(T angle_deg, T cx = 0, T cy = 0) = 0;

    // -- rendering ----------------------------------------------------------

    /// Write the shape as segments into *world*.
    virtual void draw(World<T> & world) const = 0;

    // -- utilities ----------------------------------------------------------

    virtual std::shared_ptr<Shape2D<T>> clone() const = 0;
    virtual std::string type_name() const = 0;

protected:

    explicit Shape2D(bool closed_flag)
        : m_closed(closed_flag)
    {
    }

    int32_t m_id = -1;
    bool m_closed;
};

// ---------------------------------------------------------------------------
// Concrete shape classes
// ---------------------------------------------------------------------------

/**
 * Closed polygon defined by an arbitrary vertex list (>= 3 vertices).
 */
template <typename T>
class Polygon2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    explicit Polygon2D(std::vector<point_type> vertices)
        : Shape2D<T>(/* closed */ true)
        , m_vertices(std::move(vertices))
    {
        if (m_vertices.size() < 3)
        {
            throw std::invalid_argument(
                "Polygon2D requires at least 3 vertices");
        }
    }

    void set_vertices(std::vector<point_type> const & vertices)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::set_vertices: not yet implemented");
    }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement -- emit closed polygon as segments
        throw std::runtime_error("Polygon2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polygon2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Polygon2D"; }

private:

    std::vector<point_type> m_vertices;

}; /* end class Polygon2D */

/**
 * Axis-aligned rectangle (before transforms) defined by lower-left corner
 * and dimensions.
 */
template <typename T>
class Rectangle2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    Rectangle2D(T x, T y, T width, T height)
        : Shape2D<T>(/* closed */ true)
        , m_x(x)
        , m_y(y)
        , m_width(width)
        , m_height(height)
    {
    }

    T x() const { return m_x; }
    T y() const { return m_y; }
    T width() const { return m_width; }
    T height() const { return m_height; }

    void set_width(T v) { m_width = v; }
    void set_height(T v) { m_height = v; }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement -- always 4
        throw std::runtime_error("Rectangle2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Rectangle2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Rectangle2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Rectangle2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Rectangle2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement -- emit 4 segments
        throw std::runtime_error("Rectangle2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Rectangle2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Rectangle2D"; }

private:

    T m_x;
    T m_y;
    T m_width;
    T m_height;

}; /* end class Rectangle2D */

/**
 * Triangle defined by three vertices.
 */
template <typename T>
class Triangle2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    Triangle2D(point_type p0, point_type p1, point_type p2)
        : Shape2D<T>(/* closed */ true)
        , m_p0(p0)
        , m_p1(p1)
        , m_p2(p2)
    {
    }

    Triangle2D(T x0, T y0, T x1, T y1, T x2, T y2)
        : Triangle2D(
              point_type(x0, y0, 0),
              point_type(x1, y1, 0),
              point_type(x2, y2, 0))
    {
    }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement -- always 3
        throw std::runtime_error("Triangle2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Triangle2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Triangle2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Triangle2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Triangle2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement -- emit 3 segments
        throw std::runtime_error("Triangle2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Triangle2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Triangle2D"; }

private:

    point_type m_p0;
    point_type m_p1;
    point_type m_p2;

}; /* end class Triangle2D */

/**
 * Circle approximated as a regular *nseg*-gon.
 */
template <typename T>
class Circle2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    Circle2D(T cx, T cy, T radius, size_t nseg = 64)
        : Shape2D<T>(/* closed */ true)
        , m_cx(cx)
        , m_cy(cy)
        , m_radius(radius)
        , m_nseg(nseg)
    {
        if (radius <= 0)
        {
            throw std::invalid_argument("Circle2D: radius must be positive");
        }
        if (nseg < 3)
        {
            throw std::invalid_argument("Circle2D: nseg must be >= 3");
        }
    }

    T cx() const { return m_cx; }
    T cy() const { return m_cy; }
    T radius() const { return m_radius; }
    size_t nseg() const { return m_nseg; }

    void set_radius(T v) { m_radius = v; }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement -- returns m_nseg
        throw std::runtime_error("Circle2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement -- compute point on circle
        throw std::runtime_error("Circle2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Circle2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Circle2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Circle2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement -- emit nseg segments forming a circle
        throw std::runtime_error("Circle2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Circle2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Circle2D"; }

private:

    T m_cx;
    T m_cy;
    T m_radius;
    size_t m_nseg;

}; /* end class Circle2D */

/**
 * Single line segment between two points (open, not closed).
 */
template <typename T>
class Line2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    Line2D(T x0, T y0, T x1, T y1)
        : Shape2D<T>(/* closed */ false)
        , m_p0(point_type(x0, y0, 0))
        , m_p1(point_type(x1, y1, 0))
    {
    }

    Line2D(point_type p0, point_type p1)
        : Shape2D<T>(/* closed */ false)
        , m_p0(p0)
        , m_p1(p1)
    {
    }

    T x0() const { return m_p0.x(); }
    T y0() const { return m_p0.y(); }
    T x1() const { return m_p1.x(); }
    T y1() const { return m_p1.y(); }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement -- always 2
        throw std::runtime_error("Line2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Line2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Line2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Line2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Line2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Line2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Line2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Line2D"; }

private:

    point_type m_p0;
    point_type m_p1;

}; /* end class Line2D */

/**
 * Open polyline (not closed) defined by an ordered vertex list (>= 2).
 */
template <typename T>
class Polyline2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    explicit Polyline2D(std::vector<point_type> vertices)
        : Shape2D<T>(/* closed */ false)
        , m_vertices(std::move(vertices))
    {
        if (m_vertices.size() < 2)
        {
            throw std::invalid_argument(
                "Polyline2D requires at least 2 vertices");
        }
    }

    void set_vertices(std::vector<point_type> const & vertices)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::set_vertices: not yet implemented");
    }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement -- emit (n-1) open segments
        throw std::runtime_error("Polyline2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Polyline2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Polyline2D"; }

private:

    std::vector<point_type> m_vertices;

}; /* end class Polyline2D */

/**
 * Ellipse approximated as a closed *nseg*-gon.
 */
template <typename T>
class Ellipse2D
    : public Shape2D<T>
{

public:

    using typename Shape2D<T>::value_type;
    using typename Shape2D<T>::point_type;

    Ellipse2D(T cx, T cy, T a, T b, size_t nseg = 64)
        : Shape2D<T>(/* closed */ true)
        , m_cx(cx)
        , m_cy(cy)
        , m_a(a)
        , m_b(b)
        , m_nseg(nseg)
    {
        if (a <= 0 || b <= 0)
        {
            throw std::invalid_argument(
                "Ellipse2D: semi-axes a and b must be positive");
        }
        if (nseg < 3)
        {
            throw std::invalid_argument("Ellipse2D: nseg must be >= 3");
        }
    }

    T cx() const { return m_cx; }
    T cy() const { return m_cy; }
    T a() const { return m_a; }
    T b() const { return m_b; }
    size_t nseg() const { return m_nseg; }

    void set_a(T v) { m_a = v; }
    void set_b(T v) { m_b = v; }

    size_t nvertex() const override
    {
        // TODO(2D drawing API): implement -- returns m_nseg
        throw std::runtime_error("Ellipse2D::nvertex: not yet implemented");
    }

    point_type vertex(size_t i) const override
    {
        // TODO(2D drawing API): implement -- compute point on ellipse
        throw std::runtime_error("Ellipse2D::vertex: not yet implemented");
    }

    void translate(T dx, T dy) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Ellipse2D::translate: not yet implemented");
    }

    void scale(T factor, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Ellipse2D::scale: not yet implemented");
    }

    void rotate(T angle_deg, T cx = 0, T cy = 0) override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Ellipse2D::rotate: not yet implemented");
    }

    void draw(World<T> & world) const override
    {
        // TODO(2D drawing API): implement -- emit nseg segments forming an ellipse
        throw std::runtime_error("Ellipse2D::draw: not yet implemented");
    }

    std::shared_ptr<Shape2D<T>> clone() const override
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Ellipse2D::clone: not yet implemented");
    }

    std::string type_name() const override { return "Ellipse2D"; }

private:

    T m_cx;
    T m_cy;
    T m_a;
    T m_b;
    size_t m_nseg;

}; /* end class Ellipse2D */

// Type aliases
template <typename T>
using Shape2DPtr = std::shared_ptr<Shape2D<T>>;

using Shape2DFp32 = Shape2D<float>;
using Shape2DFp64 = Shape2D<double>;
using Polygon2DFp32 = Polygon2D<float>;
using Polygon2DFp64 = Polygon2D<double>;
using Rectangle2DFp32 = Rectangle2D<float>;
using Rectangle2DFp64 = Rectangle2D<double>;
using Triangle2DFp32 = Triangle2D<float>;
using Triangle2DFp64 = Triangle2D<double>;
using Circle2DFp32 = Circle2D<float>;
using Circle2DFp64 = Circle2D<double>;
using Line2DFp32 = Line2D<float>;
using Line2DFp64 = Line2D<double>;
using Polyline2DFp32 = Polyline2D<float>;
using Polyline2DFp64 = Polyline2D<double>;
using Ellipse2DFp32 = Ellipse2D<float>;
using Ellipse2DFp64 = Ellipse2D<double>;

// ---------------------------------------------------------------------------
// Canvas2D -- shape manager owned by World, with add/remove and edit-history
// ---------------------------------------------------------------------------

/**
 * Canvas layer that lives on a World.
 *
 * Accessed via ``world->canvas()``.  Holds Shape2D objects by integer ID.
 * Transforms (translate / scale / rotate) are called directly on the shape;
 * the canvas only tracks add, remove, and commit events.
 *
 * On ``commit()``, clears the owning World's segments and re-emits every
 * managed shape so the rendering is up-to-date.
 */
template <typename T>
class Canvas2D
{

public:

    using value_type = T;
    using shape_type = Shape2D<T>;
    using shape_ptr = std::shared_ptr<shape_type>;
    using point_type = Point3d<T>;

    explicit Canvas2D(World<T> & world)
        : m_world(world)
    {
    }

    Canvas2D() = delete;
    Canvas2D(Canvas2D const &) = delete;
    Canvas2D(Canvas2D &&) = delete;
    Canvas2D & operator=(Canvas2D const &) = delete;
    Canvas2D & operator=(Canvas2D &&) = delete;
    ~Canvas2D() = default;

    // -- Create -------------------------------------------------------------

    /// Add a pre-built shape. Returns the assigned ID.
    int32_t add(shape_ptr shape)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add: not yet implemented");
    }

    int32_t add_polygon(std::vector<point_type> const & vertices)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_polygon: not yet implemented");
    }

    int32_t add_rectangle(T x, T y, T width, T height)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_rectangle: not yet implemented");
    }

    int32_t add_triangle(T x0, T y0, T x1, T y1, T x2, T y2)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_triangle: not yet implemented");
    }

    int32_t add_circle(T cx, T cy, T radius, size_t nseg = 64)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_circle: not yet implemented");
    }

    int32_t add_line(T x0, T y0, T x1, T y1)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_line: not yet implemented");
    }

    int32_t add_polyline(std::vector<point_type> const & vertices)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_polyline: not yet implemented");
    }

    int32_t add_ellipse(T cx, T cy, T a, T b, size_t nseg = 64)
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::add_ellipse: not yet implemented");
    }

    // -- Read ---------------------------------------------------------------

    /// Get shape by ID.  Throws std::out_of_range if not found.
    shape_ptr get(int32_t shape_id) const
    {
        // TODO(2D drawing API): implement
        throw std::runtime_error("Canvas2D::get: not yet implemented");
    }

    size_t nshape() const { return m_shapes.size(); }

    // -- Delete -------------------------------------------------------------

    void remove(int32_t shape_id)
    {
        // TODO(2D drawing API): implement -- erase shape + record history
        throw std::runtime_error("Canvas2D::remove: not yet implemented");
    }

    void clear()
    {
        // TODO(2D drawing API): implement -- remove all shapes + record history
        throw std::runtime_error("Canvas2D::clear: not yet implemented");
    }

    // -- Render -------------------------------------------------------------

    /**
     * Clear the owning World and re-emit every managed shape into it.
     *
     * After commit() the World's segments reflect the current state of all
     * shapes on this canvas.
     */
    void commit()
    {
        // TODO(2D drawing API): implement
        //   m_world.clear();
        //   for (auto const & [id, shape] : m_shapes)
        //       shape->draw(m_world);
        throw std::runtime_error("Canvas2D::commit: not yet implemented");
    }

    // TODO: edit history can be added here to track add/remove/clear actions.

private:

    World<T> & m_world;
    std::map<int32_t, shape_ptr> m_shapes;
    int32_t m_next_id = 0;

}; /* end class Canvas2D */

using Canvas2DFp32 = Canvas2D<float>;
using Canvas2DFp64 = Canvas2D<double>;

// ---------------------------------------------------------------------------
// World::canvas() implementation (needs complete Canvas2D type)
// ---------------------------------------------------------------------------

template <typename T>
Canvas2D<T> & World<T>::canvas()
{
    if (!m_canvas)
    {
        m_canvas = std::make_shared<Canvas2D<T>>(*this);
    }
    return *m_canvas;
}

} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
