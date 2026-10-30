#!/usr/bin/env bash
# Reactivate the stock OPPO K9 / m7332 factory root channel and run one command.
#
# Host requirements:
#   - Git Bash on Windows
#   - a wslc container named "adb-tv" with adb + a JDK (see tools/README.md)
#   - TV reachable at SERIAL with network ADB enabled and authorized
#
# Usage:
#   ./activate.sh 'id'
#   ./activate.sh 'ls -l /vendor/tvcertificate'
#   OCFREE_WAIT=5 ./activate.sh 'cat /proc/partitions'
#
# The command runs as uid 0 in u:r:factory_service_fusion:s0.
# It is NOT unconfined root: /system, /data, boot/vbmeta are still blocked.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTAINER="${OCFREE_CONTAINER:-adb-tv}"
SERIAL="${OCFREE_SERIAL:-<TV_IP>:5555}"
JDWP_PORT="${OCFREE_JDWP_PORT:-8700}"
WAIT="${OCFREE_WAIT:-2}"
WSL_C="${WSL_C:-C:/Program Files/WSL/wslc.exe}"
OUT="/vendor/tvcertificate/.ocfree_cmd.out"

wslc() {
  if [[ -n "${WSL_C}" && -f "${WSL_C}" ]]; then
    MSYS_NO_PATHCONV=1 "${WSL_C}" "$@"
  else
    MSYS_NO_PATHCONV=1 wslc "$@"
  fi
}

adb_raw() { wslc exec "${CONTAINER}" adb -s "${SERIAL}" "$@"; }
adb_str() { adb_raw "$@" | tr -d '\r'; }

if [[ $# -eq 0 ]]; then
  CMD="id"
else
  CMD="$*"
fi

PAYLOAD_FILE=""
cleanup() {
  adb_raw forward --remove "tcp:${JDWP_PORT}" >/dev/null 2>&1 || true
  [[ -n "${PAYLOAD_FILE}" ]] && rm -f "${PAYLOAD_FILE}" || true
}
trap cleanup EXIT

echo "[1/5] connect ${SERIAL}"
wslc exec "${CONTAINER}" adb connect "${SERIAL}" >/dev/null 2>&1 || true
for _ in $(seq 1 20); do
  state="$(adb_str get-state 2>/dev/null || true)"
  [[ "${state}" == "device" ]] && break
  sleep 1
done
if [[ "${state}" != "device" ]]; then
  echo "adb state=${state:-unknown}; authorize the TV debugging dialog or check connectivity" >&2
  exit 1
fi

echo "[2/5] ensure com.cvte.factory.service is running"
adb_raw shell am startservice -n com.cvte.factory.service/.MainService \
  -a cvte.factory.intent.action.APIInit >/dev/null 2>&1 || true
PID="$(adb_str shell pidof com.cvte.factory.service | awk '{print $1}')"
if [[ -z "${PID}" ]]; then
  echo "com.cvte.factory.service pid not found" >&2
  exit 1
fi
echo "      pid=${PID}"

echo "[3/5] build JDI client if needed"
if ! wslc exec "${CONTAINER}" test -f /tmp/JdiCall.class >/dev/null 2>&1; then
  wslc exec -i "${CONTAINER}" tee /tmp/JdiCall.java < "${SCRIPT_DIR}/jdi/JdiCall.java" >/dev/null
  wslc exec "${CONTAINER}" javac --add-modules jdk.jdi -cp /tmp -d /tmp /tmp/JdiCall.java
fi
adb_raw forward --remove "tcp:${JDWP_PORT}" >/dev/null 2>&1 || true
adb_raw forward "tcp:${JDWP_PORT}" "jdwp:${PID}" >/dev/null

# The factory API takes a single string; observed truncation/limit is 255 bytes.
PAYLOAD="( ${CMD} ) > ${OUT} 2>&1; /vendor/bin/chmod 644 ${OUT}"
if (( ${#PAYLOAD} > 255 )); then
  echo "payload is ${#PAYLOAD} bytes; factory API limit is about 255 (command too long)" >&2
  exit 2
fi

echo "[4/5] invoke sendCmdToFactorySvc(1, ...)"
PAYLOAD_FILE="$(mktemp "${TMPDIR:-/tmp}/ocfree-payload.XXXXXX")"
printf '%s' "${PAYLOAD}" > "${PAYLOAD_FILE}"
wslc exec -i "${CONTAINER}" tee /tmp/jdi_payload.txt < "${PAYLOAD_FILE}" >/dev/null
wslc exec "${CONTAINER}" java --add-modules jdk.jdi -cp /tmp JdiCall 1
sleep "${WAIT}"

echo "[5/5] output"
if adb_raw shell test -f "${OUT}" >/dev/null 2>&1; then
  adb_str shell cat "${OUT}"
else
  echo "(no output file; command may have failed before redirect)" >&2
fi

# Best-effort cleanup through a second attach.
printf '%s' "/vendor/bin/rm -f ${OUT}" > "${PAYLOAD_FILE}"
wslc exec -i "${CONTAINER}" tee /tmp/jdi_payload.txt < "${PAYLOAD_FILE}" >/dev/null
wslc exec "${CONTAINER}" java --add-modules jdk.jdi -cp /tmp JdiCall 1 >/dev/null 2>&1 || true
