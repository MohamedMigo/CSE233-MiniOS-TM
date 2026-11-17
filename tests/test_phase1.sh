#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

assert_has() {
  if ! grep -Fq -- "$2" <<< "$1"; then
    printf 'FAIL: expected text: %s\n' "$2" >&2
    exit 1
  fi
}

printf '\n[TEST 1] Foreground fork, execvp, getpid/getppid, waitpid\n'
out=$(printf '1\necho FG_TEST_OK\n1\n64\n3\n5\n' | ./minios_tm 2>&1)
assert_has "$out" 'FG_TEST_OK'
assert_has "$out" '[CHILD] PID='
assert_has "$out" '[FG] Master Controller is waiting: waitpid'
assert_has "$out" 'COMPLETED'
assert_has "$out" 'exit code 0'

printf '[TEST 2] Quoted execvp argument\n'
out=$(printf '1\necho "TWO WORDS"\n1\n64\n5\n' | ./minios_tm 2>&1)
assert_has "$out" 'TWO WORDS'

printf '[TEST 3] Missing executable -> exit 127\n'
out=$(printf '1\nthis_command_does_not_exist_77314\n0\n64\n3\n5\n' | ./minios_tm 2>&1)
assert_has "$out" 'execvp failed'
assert_has "$out" 'exit code 127'
assert_has "$out" 'TERMINATED'

printf '[TEST 4] Background tasks, WNOHANG, kill and reaping\n'
out=$(
  { printf '2\nsleep 1\n1\n64\n2\nsleep 20\n20\n64\n3\n';
    sleep 1.4;
    printf '3\n4\n2\n3\n5\n';
  } | ./minios_tm 2>&1
)
assert_has "$out" 'waitpid(WNOHANG)'
assert_has "$out" '[REAP] Task #1'
assert_has "$out" 'COMPLETED'
assert_has "$out" 'kill(PID='
assert_has "$out" '[REAP] waitpid PID='
assert_has "$out" 'SIG15'

printf '[TEST 5] Clean exit terminates and reaps active children\n'
out=$(printf '2\nsleep 20\n20\n64\n5\n' | ./minios_tm 2>&1)
assert_has "$out" '[CLEANUP] SIGTERM to task #1'
assert_has "$out" 'MiniOS-TM exited cleanly'

printf '[OK] Original five system-call tests passed.\n'

printf '[TEST 6] Modern CLI overview, system-call inspector and version\n'
out=$(printf '7\n5\n' | ./minios_tm 2>&1)
assert_has "$out" 'LIVE OVERVIEW'
assert_has "$out" 'SYSTEM CALL INSPECTOR'
assert_has "$out" 'fork()'
assert_has "$(./minios_tm --version)" 'v2.0'

printf '[TEST 7] Timed process monitor displays updated task state\n'
out=$(printf '2\nsleep 1\n1\n64\n6\n3\n3\n5\n' | ./minios_tm 2>&1)
assert_has "$out" 'LIVE PROCESS MONITOR'
assert_has "$out" '[MONITOR] Snapshot 3/3'
assert_has "$out" 'COMPLETED'

printf '[PASS] All seven Phase 1 + Modern CLI Linux tests passed.\n'
