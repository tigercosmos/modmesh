#!/bin/bash
#
# Throwaway diagnostic helper for the segfault_probe workflow. Run the pilot
# test harness in a loop until a deadline and report how many iterations
# segfaulted, so a crash that fires once in many runs is measured rather than
# guessed at. The pilot binary is driven directly because the make target
# depends on the build, which would spend the budget re-checking it.
# Delete this along with the probe branch.
#
# Usage: loop_pytest.sh <label> <seconds> <pytest-options>

set -u

label="${1:?label}"
budget="${2:?seconds}"
opts="${3:?pytest options}"

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
pilot="$(find "${root}/build" -type f -name pilot -perm -u+x 2>/dev/null | head -n 1)"
if [ -z "${pilot}" ] ; then
    echo "no pilot binary under ${root}/build; build it first" >&2
    exit 1
fi

logdir="${root}/probe-logs"
mkdir -p "${logdir}"

export PYTHONPATH="${root}"
export LD_LIBRARY_PATH="$(python3 -c "import os, shiboken6; print(os.path.dirname(shiboken6.__file__))")"
# The runner has no desktop to take over, so let the tests map real windows,
# exactly as the lint workflow does.
export SOLVCON_TEST_SHOW_WINDOWS="${SOLVCON_TEST_SHOW_WINDOWS:-ON}"

deadline=$(( $(date +%s) + budget ))
iterations=0
crashes=0
others=0

while [ "$(date +%s)" -lt "${deadline}" ]; do
    iterations=$(( iterations + 1 ))
    log="${logdir}/${label}-${iterations}.log"
    if env PYTEST_OPTS="${opts}" "${pilot}" --mode=pytest > "${log}" 2>&1 ; then
        rm -f "${log}"
        continue
    fi
    if grep -q "Segmentation fault" "${log}" ; then
        crashes=$(( crashes + 1 ))
        echo "iteration ${iterations}: SEGFAULT"
        grep -m 1 -A 8 "Fatal Python error" "${log}" || true
    else
        others=$(( others + 1 ))
        echo "iteration ${iterations}: failed without a segfault"
        tail -n 20 "${log}"
    fi
done

echo "RESULT ${label}: iterations=${iterations} segfaults=${crashes} other_failures=${others}"
{
    echo "| ${ARM:-?} | ${label} | ${iterations} | ${crashes} | ${others} |"
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"

# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
