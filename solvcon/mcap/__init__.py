# Copyright (c) 2026, solvcon team <contact@solvcon.net>
# BSD 3-Clause License, see COPYING

"""
mcap: read MCAP recordings into solvcon.

The extension carries the subsystem only when it is configured with
``BUILD_MCAP=ON``, because decompressing MCAP chunks needs lz4 and zstd.
``HAS_MCAP`` is true only in such a build, and ``Reader`` exists only then.
"""

from .. import core
from . import _decode_plan

HAS_MCAP = core.HAS_MCAP
DecodePlan = _decode_plan.DecodePlan
DecodePlanError = _decode_plan.DecodePlanError

__all__ = [
    "ColumnSet",
    "DecodePlan",
    "DecodePlanError",
    "HAS_MCAP",
]


class ColumnSet:
    """Columns extracted from the messages of one topic.

    ``time`` is the log time of every message in nanoseconds.  One column
    per requested field follows it.  Every column holds one element per
    message, so a row index names the same message in all of them.  A
    column is the ``SimpleArray`` of the type the plan states for its
    field.  Index a column by the field path or by the position of the
    field in the request.
    """

    __slots__ = ("_time", "_fields", "_columns")

    def __init__(self, time, fields, columns):
        self._time = time
        self._fields = tuple(fields)
        self._columns = tuple(columns)

    @property
    def time(self):
        return self._time

    @property
    def fields(self):
        return self._fields

    def __len__(self):
        return len(self._columns)

    def __contains__(self, field):
        return field in self._fields

    def __getitem__(self, key):
        if isinstance(key, str):
            try:
                key = self._fields.index(key)
            except ValueError:
                raise KeyError(key) from None
        return self._columns[key]


if HAS_MCAP:
    class Reader(core.McapReader):
        """Reader of one MCAP recording, with column extraction."""

        def extract(self, topic, plan):
            """Run a decode plan over every message of a topic.

            Return a ``ColumnSet``.  Every channel of the topic must encode
            CDR messages of one schema.
            """
            core_plan = core.McapDecodePlan(plan.instructions,
                                            len(plan.fields))
            time, columns = super().extract(topic, core_plan)
            return ColumnSet(time, plan.fields, columns)

    __all__.append("Reader")

# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
