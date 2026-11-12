#!/usr/bin/env bash
# Regression suite for obfuscator_cpp.
#
# Every case is obfuscated in token, line, no-alias, two-pass and translate
# mode, then compiled and executed; a case passes only when the restored
# program exits with status 0.
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
CXX="${CXX:-g++}"
STD="${STD:--std=c++20}"

PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); }
fail() {
  FAIL=$((FAIL + 1))
  echo "FAIL: $*"
}

if ! "$CXX" -O2 "$ROOT/main.cpp" -o "$TMP/obf"; then
  echo "FATAL: cannot build obfuscator"
  exit 1
fi

# run_one <label> <source> <output> [obfuscator options...]
run_one() {
  local label="$1" src="$2" out="$3"
  shift 3
  timeout 60 "$TMP/obf" "$src" "$out" "$@" >"$TMP/obf.log" 2>&1
  local rc=$?
  if [ "$rc" -ne 0 ]; then
    fail "$label: obfuscator exited $rc"
    return
  fi
  if ! "$CXX" $STD "$out" -o "$TMP/bin" 2>"$TMP/cc.log"; then
    fail "$label: compile failed: $(grep -m1 -E 'error|fatal' "$TMP/cc.log")"
    return
  fi
  "$TMP/bin"
  rc=$?
  if [ "$rc" -ne 0 ]; then
    fail "$label: program exited $rc"
    return
  fi
  if [ -e "$out.tmp" ] || [ -e "$out.exp" ] || [ -e "$out.raw" ]; then
    fail "$label: sidecar files left behind"
    return
  fi
  pass
}

check_case() {
  local name="$1" src="$2"
  local tok="$TMP/$name.token.cpp" line="$TMP/$name.line.cpp"
  local noa="$TMP/$name.noalias.cpp"
  run_one "$name/token" "$src" "$tok"
  run_one "$name/line" "$src" "$line" -l
  run_one "$name/noalias" "$src" "$noa" -n
  run_one "$name/twopass" "$src" "$TMP/$name.twopass.cpp" \
    -x "OOO OO1 O1O O11" "OOO OO1 O1O O11"
  run_one "$name/translate" "$src" "$TMP/$name.translate.cpp" -t
  # Make sure each mode actually did what its name says.
  if [ -e "$tok" ] && ! grep -q '^#define ' "$tok"; then
    fail "$name/token: no #define lines emitted"
  fi
  if [ -e "$line" ] && ! grep -q '^#define ' "$line"; then
    fail "$name/line: no #define lines emitted"
  fi
  if [ -e "$noa" ] && grep -q '^#define ' "$noa"; then
    fail "$name/noalias: aliases emitted in -n mode"
  fi
}

# expect_rc <label> <expected-exit-code> [obfuscator arguments...]
expect_rc() {
  local label="$1" want="$2"
  shift 2
  timeout 10 "$TMP/obf" "$@" >"$TMP/arg.log" 2>&1
  local rc=$?
  if [ "$rc" -eq "$want" ]; then
    pass
  else
    fail "$label: expected exit $want, got $rc"
  fi
}

for src in "$ROOT"/tests/cases/*.cpp; do
  name="$(basename "$src" .cpp)"
  check_case "$name" "$src"
done

# -l must really alias whole lines: far fewer aliases than per-token mode.
if [ -e "$TMP/raw_same_line.token.cpp" ] && [ -e "$TMP/raw_same_line.line.cpp" ]; then
  tdefs=$(grep -c '^#define ' "$TMP/raw_same_line.token.cpp")
  ldefs=$(grep -c '^#define ' "$TMP/raw_same_line.line.cpp")
  if [ "$tdefs" -le "$ldefs" ]; then
    fail "raw_same_line: token mode should emit more aliases than line mode"
  fi
fi

# Documented workflow: translating an already-obfuscated file restores code.
run_one "numbers/chained-translate" "$TMP/numbers.token.cpp" \
  "$TMP/numbers.restored.cpp" -t

# Short symbol set: generated aliases must never collide with input names.
run_one "alias_collision/custom-set" "$ROOT/tests/cases/alias_collision.cpp" \
  "$TMP/collision1.cpp" -s "A B"
run_one "alias_collision/twopass-custom" "$ROOT/tests/cases/alias_collision.cpp" \
  "$TMP/collision2.cpp" -x "A B" "A B"

# Output shaping and head file.
printf '// obfuscator test signature\n' >"$TMP/head.h"
run_one "options/head-wrap" "$ROOT/tests/cases/header_macros.cpp" \
  "$TMP/headwrap.cpp" -h "$TMP/head.h" -m 3 -M 60
if [ "$(head -n 1 "$TMP/headwrap.cpp" 2>/dev/null)" != "// obfuscator test signature" ]; then
  fail "options/head-wrap: head file content missing from output"
fi

# UTF-8 BOM before the first directive.
printf '\xEF\xBB\xBF#include <cstddef>\nint main() { return sizeof(std::size_t) >= 4 ? 0 : 1; }\n' \
  >"$TMP/bom.cpp"
run_one "options/utf8-bom" "$TMP/bom.cpp" "$TMP/bom.out.cpp"

# Argument and I/O validation.
SRC="$ROOT/tests/cases/numbers.cpp"
expect_rc "args/empty-symbol-set" 12 "$SRC" "$TMP/arg.cpp" -s ""
expect_rc "args/one-symbol" 12 "$SRC" "$TMP/arg.cpp" -s "A"
expect_rc "args/duplicate-symbol" 12 "$SRC" "$TMP/arg.cpp" -s "A A"
expect_rc "args/mixed-symbol-lengths" 12 "$SRC" "$TMP/arg.cpp" -s "A AA"
expect_rc "args/mixed-symbol-lengths-x" 16 "$SRC" "$TMP/arg.cpp" \
  -x "A AA" "B BB"
expect_rc "args/zero-per-line" 15 "$SRC" "$TMP/arg.cpp" -m 0
expect_rc "args/bad-per-line" 15 "$SRC" "$TMP/arg.cpp" -m abc
expect_rc "args/zero-max-chars" 17 "$SRC" "$TMP/arg.cpp" -M 0
expect_rc "args/twopass-missing-set" 16 "$SRC" "$TMP/arg.cpp" -x "A B"
expect_rc "args/missing-output" 11 "$SRC"
expect_rc "args/no-paths" 11
mkdir -p "$TMP/input-dir" "$TMP/output-dir"
expect_rc "args/directory-input" 21 "$TMP/input-dir" "$TMP/dir.out.cpp"
expect_rc "args/output-is-directory" 25 "$SRC" "$TMP/output-dir"
printf '#error boom\nint main() {}\n' >"$TMP/bad.cpp"
expect_rc "args/preprocessing-fails" 24 "$TMP/bad.cpp" "$TMP/bad.out.cpp"
if [ -e "$TMP/bad.out.cpp.exp" ]; then
  fail "args/preprocessing-fails: .exp sidecar left behind"
fi

# Output paths must reach g++/rm as data, never as shell syntax. A relative
# name keeps the injected command free of slashes; a broken quoting layer would
# run "touch pwned" in $TMP.
OUT_REL='weird$(touch pwned).cpp'
rm -f "$TMP/pwned"
( cd "$TMP" && timeout 60 "$TMP/obf" "$SRC" "$OUT_REL" ) >"$TMP/obf.log" 2>&1
rc=$?
if [ "$rc" -ne 0 ]; then
  fail "options/shell-safe-path: obfuscator exited $rc"
elif ! "$CXX" $STD "$TMP/$OUT_REL" -o "$TMP/bin" 2>"$TMP/cc.log"; then
  fail "options/shell-safe-path: compile failed"
elif ! "$TMP/bin"; then
  fail "options/shell-safe-path: program failed"
elif [ -e "$TMP/pwned" ]; then
  fail "options/shell-safe-path: output path was interpreted by the shell"
else
  pass
fi

echo "----------------------------------------"
echo "PASS: $PASS  FAIL: $FAIL"
[ "$FAIL" -eq 0 ]
