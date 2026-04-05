# Copyright (c) 2026, An-Chi Liu <phy.tiger@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# - Neither the name of the copyright holder nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


"""
Demonstrate the 2D Canvas API.

Usage pattern::

    world = mm.WorldFp64()
    canvas = world.canvas()
    canvas.add_rectangle(0, 0, 10, 5)
    shape = canvas.get(0)
    shape.translate(1, 2)
    canvas.commit()          # world now contains the segments
    widget.updateWorld(world)
"""

import unittest

import modmesh as mm
import pytest

pt = mm.Point3dFp64


def _pt(x, y):
    return pt(x, y, 0.0)


@pytest.mark.xfail(reason="Canvas2D API is prototype-only (stubs throw)")
class Canvas2DBasicTC(unittest.TestCase):
    """Basic create / read / remove / commit workflow."""

    def test_add_and_get(self):
        w = mm.WorldFp64()
        canvas = w.canvas()
        sid = canvas.add_rectangle(x=0, y=0, width=4, height=3)
        self.assertEqual(canvas.nshape, 1)
        shape = canvas.get(sid)
        self.assertEqual(shape.type_name(), "Rectangle2D")

    def test_add_multiple_shapes(self):
        w = mm.WorldFp64()
        canvas = w.canvas()
        canvas.add_line(0, 0, 1, 1)
        canvas.add_circle(cx=5, cy=5, radius=2, nseg=16)
        canvas.add_polygon([_pt(0, 0), _pt(3, 0), _pt(3, 4)])
        self.assertEqual(canvas.nshape, 3)

    def test_remove(self):
        w = mm.WorldFp64()
        canvas = w.canvas()
        sid = canvas.add_line(0, 0, 1, 1)
        canvas.remove(sid)
        self.assertEqual(canvas.nshape, 0)

    def test_clear(self):
        w = mm.WorldFp64()
        canvas = w.canvas()
        canvas.add_line(0, 0, 1, 1)
        canvas.add_circle(cx=0, cy=0, radius=1)
        canvas.clear()
        self.assertEqual(canvas.nshape, 0)

    def test_commit_populates_world(self):
        """After commit the World should contain segments from all shapes."""
        w = mm.WorldFp64()
        canvas = w.canvas()
        canvas.add_line(0, 0, 1, 1)              # 1 segment
        canvas.add_rectangle(x=0, y=0, width=2, height=1)  # 4 segments
        canvas.commit()
        self.assertEqual(w.nsegment, 5)

    def test_commit_after_shape_transform(self):
        """Transforms on the shape are reflected after commit."""
        w = mm.WorldFp64()
        canvas = w.canvas()
        sid = canvas.add_line(0, 0, 1, 0)
        shape = canvas.get(sid)
        shape.translate(dx=10, dy=0)
        canvas.commit()
        seg = w.segment(0)
        self.assertAlmostEqual(seg.p0.x, 10.0)
        self.assertAlmostEqual(seg.p1.x, 11.0)


# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
