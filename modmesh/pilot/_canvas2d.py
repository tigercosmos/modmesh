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
High-level 2D canvas API with shape CRUD.

All shape and canvas classes are implemented in C++ and exposed via pybind11.
This module re-exports them for convenience from the ``modmesh.pilot``
namespace.

Architecture::

    Canvas2DFp64     shape registry + add/remove
        ^  world.canvas()
        |
    WorldFp64        owns Canvas2D, holds low-level geometry
        |  widget.updateWorld(world)
        v
    R3DWidget        Qt3D rendering
"""

from .. import core

__all__ = [
    # Shape2D hierarchy (Fp64)
    'Shape2DFp64',
    'Polygon2DFp64',
    'Rectangle2DFp64',
    'Triangle2DFp64',
    'Circle2DFp64',
    'Line2DFp64',
    'Polyline2DFp64',
    'Ellipse2DFp64',
    # canvas (accessed via world.canvas(), not constructed directly)
    'Canvas2DFp64',
]

# Guard: C++ bindings may not yet be compiled into _modmesh.
if hasattr(core, 'Shape2DFp64'):
    Shape2DFp64 = core.Shape2DFp64
    Polygon2DFp64 = core.Polygon2DFp64
    Rectangle2DFp64 = core.Rectangle2DFp64
    Triangle2DFp64 = core.Triangle2DFp64
    Circle2DFp64 = core.Circle2DFp64
    Line2DFp64 = core.Line2DFp64
    Polyline2DFp64 = core.Polyline2DFp64
    Ellipse2DFp64 = core.Ellipse2DFp64
    Canvas2DFp64 = core.Canvas2DFp64


# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4 tw=79:
