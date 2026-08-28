#!/bin/bash
#
# Copyright (c) 2026, solvcon team <contact@solvcon.net>
# BSD 3-Clause License, see COPYING
#
# Carry the "skip-ci" label to the workflow runs a pull request already has:
# cancel what is still running when the label arrives, and re-run what the
# label stopped when it goes away. Driven by
# .github/workflows/skip_ci_label.yml, and runnable by hand against any
# repository the local `gh` is authenticated for:
#
#   GITHUB_REPOSITORY=solvcon/solvcon LABEL_NAME=skip-ci LABEL_ACTION=labeled \
#     HEAD_SHA=<sha> contrib/ci/apply-skip-ci-label.sh
#
# LABEL_NAME is the label that changed, LABEL_ACTION is "labeled" or
# "unlabeled", and HEAD_SHA names the commit whose runs to act on.

set -euo pipefail

: "${GITHUB_REPOSITORY:?GITHUB_REPOSITORY is required}"
: "${LABEL_NAME:?LABEL_NAME is required}"
: "${LABEL_ACTION:?LABEL_ACTION is required}"
: "${HEAD_SHA:?HEAD_SHA is required}"

# The workflows whose heavy jobs check_skip_ci gates.
WORKFLOWS="devbuild.yml devbuild_windows.yml lint.yml"

if [ "$LABEL_NAME" != "skip-ci" ]; then
  echo "::notice::The \"$LABEL_NAME\" label does not gate CI."
  exit 0
fi

case "$LABEL_ACTION" in
  labeled | unlabeled) ;;
  *)
    echo "::error::LABEL_ACTION must be labeled or unlabeled, got" \
         "'$LABEL_ACTION'"
    exit 1
    ;;
esac

# Cancelling a run that finished between the listing and the request answers
# HTTP 409, which is the label arriving a moment too late rather than a
# failure of this script.
cancel_run() {
  if gh api -X POST "repos/$GITHUB_REPOSITORY/actions/runs/$1/cancel" \
      >/dev/null 2>&1; then
    echo "::notice::Cancelled $2 run $1."
  else
    echo "::notice::Could not cancel $2 run $1; it is no longer running."
  fi
}

# Re-run only what the label stopped. A run that built the commit for real
# keeps the result it reported, and re-running it would spend the matrix
# again for nothing.
rerun_run() {
  local stopped
  stopped="$(gh api --paginate \
    "repos/$GITHUB_REPOSITORY/actions/runs/$1/jobs?per_page=100" \
    --jq '[.jobs[] | select(.conclusion == "skipped" or
                            .conclusion == "cancelled")] | length')"
  if [ "$stopped" -eq 0 ]; then
    return
  fi
  gh api -X POST "repos/$GITHUB_REPOSITORY/actions/runs/$1/rerun" >/dev/null
  echo "::notice::Re-ran $2 run $1."
}

for workflow in $WORKFLOWS; do
  gh api --paginate \
    "repos/$GITHUB_REPOSITORY/actions/workflows/$workflow/runs?head_sha=$HEAD_SHA&per_page=100" \
    --jq '.workflow_runs[] | [(.id | tostring), .status] | @tsv' \
  | while IFS="$(printf '\t')" read -r run_id status; do
      if [ "$LABEL_ACTION" = "labeled" ]; then
        if [ "$status" != "completed" ]; then
          cancel_run "$run_id" "$workflow"
        fi
      elif [ "$status" = "completed" ]; then
        rerun_run "$run_id" "$workflow"
      fi
    done
done

# vim: set ff=unix fenc=utf8 et sw=2 ts=2 sts=2:
