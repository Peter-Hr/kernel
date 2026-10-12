#!/bin/bash

#
# Run tests for given suites, report results for a dashboard
# This script is expected to be executed only by GitLab CI runners
# Local execution may fail in an upredictable way
#


set -ueo pipefail


upload_report() {
   ssh \
       -i tools/key_dashboard \
       -o UserKnownHostsFile=/dev/null \
       -o StrictHostKeyChecking=no \
       -o PasswordAuthentication=no \
       -o IdentitiesOnly=yes \
       -p 22200 \
       dashboard@lab.d3s.mff.cuni.cz \
       report "$@"
}

cd "$( dirname "$0" )/../"

if [ "$#" -eq 0 ]; then
   echo "Run with suite filename." >&2
   exit 1
fi

json_report="$( mktemp )"

exit_code=0
./tools/tester.py suite --verbose --json-report "$json_report" "$@" || exit_code="$?"

suite_checksum="$( echo -n "$@" | md5sum | cut '-d ' -f 1 )"

if [ -e "tools/key_dashboard" ]; then
    chmod 0600 "tools/key_dashboard"
    if [ -n "${CI_MERGE_REQUEST_IID:-}" ]; then
        branch="m/$CI_MERGE_REQUEST_IID"
    else
        branch="b/${CI_COMMIT_BRANCH:-unknown}"
    fi
    upload_report "$branch" "${CI_COMMIT_SHA:-unknown}" "$suite_checksum" <"$json_report" || true
fi

rm -f "$json_report"

exit "$exit_code"
