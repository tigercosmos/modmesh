# Copyright (c) 2021, Yung-Yu Chen <yyc@solvcon.net>
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
Show sample mesh
"""

from .. import core
from .. import view
from .. import apputil


def help_tri(set_command=False):
    cmd = """
# Open a sub window for a triangle:
w_tri = add3DWidget()
mh_tri = make_triangle()
w_tri.updateMesh(mh_tri)
w_tri.showMark()
print("tri nedge:", mh_tri.nedge)
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def help_tet(set_command=False):
    cmd = """
# Open a sub window for a tetrahedron:
w_tet = add3DWidget()
mh_tet = make_tetrahedron()
w_tet.updateMesh(mh_tet)
w_tet.showMark()
print("tet nedge:", mh_tet.nedge)
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def help_2dmix(set_command=False):
    cmd = """
# Open a sub window for triangles and quadrilaterals:
w_2dmix = add3DWidget()
mh_2dmix = make_2dmix(do_small=False)
w_2dmix.updateMesh(mh_2dmix)
w_2dmix.showMark()
print("2dmix nedge:", mh_2dmix.nedge)
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def help_3dmix(set_command=False):
    cmd = """
# Open a sub window for triangles and quadrilaterals:
w_3dmix = add3DWidget()
mh_3dmix = make_3dmix()
w_3dmix.updateMesh(mh_3dmix)
w_3dmix.showMark()
print("3dmix nedge:", mh_3dmix.nedge)
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def help_other(set_command=False):
    cmd = """
# Show triangle information:
print("position:", w_tri.position)
print("up_vector:", w_tri.up_vector)
print("view_center:", w_tri.view_center)

# Set view:
w_tri.up_vector = (0, 1, 0)
w_tri.position = (-10, -10, -20)
w_tri.view_center = (0, 0, 0)

# Outdated and may not work:
# line = mm.view.RLine(-1, -1, -1, -2, -2, -2, 0, 128, 128)
# print(line)
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def help_mesh_viewer(path, set_command=False):
    cmd = f"""
# Open a sub window for the mesh viewer:
w_mh_viewer = add3DWidget()
mh_viewer = make_mesh_viewer("{path}")
w_mh_viewer.updateMesh(mh_viewer)
w_mh_viewer.showMark()
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def help_bezier(set_command=False):
    cmd = """
# Open a sub window for some bezier curves:
w = make_bezier()
"""
    view.mgr.pycon.writeToHistory(cmd)
    if set_command:
        view.mgr.pycon.command = cmd.strip()


def make_mesh_viewer(path):
    data = open(path, 'rb').read()
    gm = core.Gmsh(data)
    mh = gm.to_block()
    return mh


def make_triangle():
    mh = core.StaticMesh(ndim=2, nnode=4, nface=0, ncell=3)
    mh.ndcrd.ndarray[:, :] = (0, 0), (-1, -1), (1, -1), (0, 1)
    mh.cltpn.ndarray[:] = core.StaticMesh.TRIANGLE
    mh.clnds.ndarray[:, :4] = (3, 0, 1, 2), (3, 0, 2, 3), (3, 0, 3, 1)
    mh.build_interior()
    mh.build_boundary()
    mh.build_ghost()
    return mh


def make_2dmix(do_small=False):
    T = core.StaticMesh.TRIANGLE
    Q = core.StaticMesh.QUADRILATERAL

    if do_small:
        mh = core.StaticMesh(ndim=2, nnode=6, nface=0, ncell=3)
        mh.ndcrd.ndarray[:, :] = [
            (0, 0), (1, 0), (0, 1), (1, 1), (2, 0), (2, 1)
        ]
        mh.cltpn.ndarray[:] = [
            T, T, Q,
        ]
        mh.clnds.ndarray[:, :5] = [
            (3, 0, 3, 2, -1), (3, 0, 1, 3, -1), (4, 1, 4, 5, 3),
        ]
    else:
        mh = core.StaticMesh(ndim=2, nnode=16, nface=0, ncell=14)
        mh.ndcrd.ndarray[:, :] = [
            (0, 0), (1, 0), (2, 0), (3, 0),
            (0, 1), (1, 1), (2, 1), (3, 1),
            (0, 2), (1, 2), (2, 2), (3, 2),
            (0, 3), (1, 3), (2, 3), (3, 3),
        ]
        mh.cltpn.ndarray[:] = [
            T, T, T, T, T, T,  # 0-5,
            Q, Q,  # 6-7
            T, T, T, T,  # 8-11
            Q, Q,  # 12-13
        ]
        mh.clnds.ndarray[:, :5] = [
            (3, 0, 5, 4, -1), (3, 0, 1, 5, -1),  # 0-1 triangles
            (3, 1, 2, 5, -1), (3, 2, 6, 5, -1),  # 2-3 triangles
            (3, 2, 7, 6, -1), (3, 2, 3, 7, -1),  # 4-5 triangles
            (4, 4, 5, 9, 8), (4, 5, 6, 10, 9),  # 6-7 quadrilaterals
            (3, 6, 7, 10, -1), (3, 7, 11, 10, -1),  # 8-9 triangles
            (3, 8, 9, 12, -1), (3, 9, 13, 12, -1),  # 10-11 triangles
            (4, 9, 10, 14, 13), (4, 10, 11, 15, 14),  # 12-13 quadrilaterals
        ]
    mh.build_interior()
    mh.build_boundary()
    mh.build_ghost()
    return mh


def make_3dmix():
    HEX = core.StaticMesh.HEXAHEDRON
    TET = core.StaticMesh.TETRAHEDRON
    PSM = core.StaticMesh.PRISM
    PYR = core.StaticMesh.PYRAMID

    mh = core.StaticMesh(ndim=3, nnode=11, nface=0, ncell=4)
    mh.ndcrd.ndarray[:, :] = [
        (0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0),
        (0, 0, 1), (1, 0, 1), (1, 1, 1), (0, 1, 1),
        (0.5, 1.5, 0.5),
        (1.5, 1, 0.5), (1.5, 0, 0.5),
    ]
    mh.cltpn.ndarray[:] = [
        HEX, PYR, TET, PSM,
    ]
    mh.clnds.ndarray[:, :9] = [
        (8, 0, 1, 2, 3, 4, 5, 6, 7), (5, 2, 3, 7, 6, 8, -1, -1, -1),
        (4, 2, 6, 9, 8, -1, -1, -1, -1), (6, 2, 6, 9, 1, 5, 10, -1, -1),
    ]
    mh.build_interior()
    mh.build_boundary()
    mh.build_ghost()
    return mh


def make_tetrahedron():
    mh = core.StaticMesh(ndim=3, nnode=4, nface=4, ncell=1)
    mh.ndcrd.ndarray[:, :] = (0, 0, 0), (0, 1, 0), (-1, 1, 0), (0, 1, 1)
    mh.cltpn.ndarray[:] = core.StaticMesh.TETRAHEDRON
    mh.clnds.ndarray[:, :5] = [(4, 0, 1, 2, 3)]
    mh.build_interior()
    mh.build_boundary()
    mh.build_ghost()
    return mh


def make_bezier():
    """
    A simple example for drawing a couple of cubic Bezier curves.
    """
    Vector = core.Vector3dFp64
    World = core.WorldFp64
    w = World()
    b1 = w.add_bezier(
        [Vector(0, 0, 0), Vector(1, 1, 0), Vector(3, 1, 0),
         Vector(4, 0, 0)])
    b1.sample(7)
    b2 = w.add_bezier(
        [Vector(4, 0, 0), Vector(3, -1, 0), Vector(1, -1, 0),
         Vector(0, 0, 0)])
    b2.sample(25)
    b3 = w.add_bezier(
        [Vector(0, 2, 0), Vector(1, 3, 0), Vector(3, 3, 0),
         Vector(4, 2, 0)])
    b3.sample(9)
    wid = view.mgr.add3DWidget()
    wid.updateWorld(w)
    wid.showMark()
    return wid


def load_app():
    aenv = apputil.get_current_appenv()
    symbols = (
        'help_tri',
        'help_tet',
        'help_2dmix',
        'help_3dmix',
        'help_mesh_viewer',
        'help_other',
        'help_bezier',
        'make_triangle',
        'make_tetrahedron',
        'make_2dmix',
        'make_3dmix',
        'make_mesh_viewer',
        'make_bezier',
        ('add3DWidget', view.mgr.add3DWidget),
    )
    for k in symbols:
        if isinstance(k, tuple):
            k, o = k
        else:
            o = globals().get(k, None)
            if o is None:
                o = locals().get(k, None)
        view.mgr.pycon.writeToHistory(f"Adding symbol {k}\n")
        aenv.globals[k] = o
    view.mgr.pycon.writeToHistory("""
# Use the functions for more examples:
help_tri(set_command=False)  # or True
help_tet(set_command=False)  # or True
help_2dmix(set_command=False)  # or True
help_3dmix(set_command=False)  # or True
help_other(set_command=False)  # or True
help_mesh_viewer(path, set_command=False)  # or True
help_bezier(set_command=False)  # or True
""")

# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
