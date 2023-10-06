#!/bin/bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";

cd "$SCRIPT_DIR" || exit 1

mkdir -p "$SCRIPT_DIR"/dist
rm -f "$SCRIPT_DIR"/dist/dosxtrs.zip


rm -rf _dist_gather
mkdir _dist_gather || exit 1

cp dosxtrs.exe _dist_gather/ || exit 1

cp jahdatst.exe _dist_gather/ || exit 1
cp cwsdpmi/BIN/CWSDPMI.EXE _dist_gather/ || exit 1
cp ./keytrap/target/KEYTRAP.COM _dist_gather || exit 1
cp ./launcher/target/LAUNCHER.COM _dist_gather || exit 1

if [[ -d "../dostrs-build-extras/runtime-trs" ]]; then
  rsync -a "../dostrs-build-extras/runtime-trs/" _dist_gather/ || exit 1
else
  echo "Cannot find runtime-trs" 1>&2
  exit 1
fi

echo "
keytrap.com dosxtrs -model 1 -romfile M1L1.ROM
" | perl -pe 's/\n/\r\n/g' > _dist_gather/RUN_M1L1.BAT

echo "
keytrap.com dosxtrs -model 1 -romfile M1L2.ROM
" | perl -pe 's/\n/\r\n/g' > _dist_gather/RUN_M1L2.BAT

cat dosbat/LAUNCH.BAT | perl -pe 's/\n/\r\n/g' > _dist_gather/LAUNCH.BAT
cat dosbat/GETUPD.BAT | perl -pe 's/\n/\r\n/g' > _dist_gather/GETUPD.BAT


cd _dist_gather || exit 1

zip -r "$SCRIPT_DIR"/dist/dosxtrs.zip *

zip -r "$SCRIPT_DIR"/dist/dxexe.zip dosxtrs.exe
zip -r "$SCRIPT_DIR"/dist/emus.zip EMUS
zip -r "$SCRIPT_DIR"/dist/jahdatst.zip jahdatst.exe


cd "$SCRIPT_DIR"

rm -rf _dist_gather


