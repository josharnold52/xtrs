#!/bin/bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";

cd "$SCRIPT_DIR" || exit 1

mkdir -p "$SCRIPT_DIR"/../dist
rm -f "$SCRIPT_DIR"/../dist/dosxtrs.zip

mkdir -p "$SCRIPT_DIR"/../dist-extras



rm -rf _dist_gather
mkdir _dist_gather || exit 1

cp dosxtrs.exe _dist_gather/ || exit 1
cp cwsdpmi/BIN/CWSDPMI.EXE _dist_gather/ || exit 1
cp M1L1.ROM _dist_gather/ || exit 1
cp M1L2.ROM _dist_gather/ || exit 1
cp ./keytrap/target/KEYTRAP.COM _dist_gather || exit 1
rsync -a "$SCRIPT_DIR"/../dist-extras/ _dist_gather/ || exit 1


echo "
keytrap.com dosxtrs -model 1 -romfile M1L1.ROM
" | perl -pe 's/\n/\r\n/g' > _dist_gather/RUN_M1L1.BAT

echo "
keytrap.com dosxtrs -model 1 -romfile M1L2.ROM
" | perl -pe 's/\n/\r\n/g' > _dist_gather/RUN_M1L2.BAT

cd _dist_gather || exit 1

zip "$SCRIPT_DIR"/../dist/dosxtrs.zip *

cd "$SCRIPT_DIR"

rm -rf _dist_gather


