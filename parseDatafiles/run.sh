#!/bin/bash
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

CURR_PWD=$(pwd)

cd "${SCRIPT_DIR}"
python3 ./main*.py
cd "${CURR_PWD}"
