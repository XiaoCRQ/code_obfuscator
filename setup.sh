#!/usr/bin/env bash

sudo rm /usr/local/bin/obfuscator_cpp

SRC="main.cpp"
OUT="/usr/local/bin/obfuscator_cpp"

start_time=$(date +%s)

spinner() {
  local pid=$1
  local delay=0.1
  local spinstr='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'

  while ps -p $pid >/dev/null 2>&1; do
    local temp=${spinstr#?}
    printf "\r[Info] Building %s " "$spinstr"
    spinstr=$temp${spinstr%"$temp"}
    sleep $delay
  done
}

CXXFLAGS="-O2 -static "

echo "[Info] Building ..."

sudo g++ $CXXFLAGS "$SRC" -o "$OUT" &
PID=$!
spinner $PID
wait $PID

printf "\r[Info] ✔ Build finished → %s\n" "$OUT"
