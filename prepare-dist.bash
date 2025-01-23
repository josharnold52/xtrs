#!/bin/bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";

cd "$SCRIPT_DIR" || exit 1

mkdir -p "$SCRIPT_DIR"/dist
rm -f "$SCRIPT_DIR"/dist/dosxtrs.zip


rm -rf _dist_gather
mkdir _dist_gather || exit 1

cp target/dos/dosxtrs.exe _dist_gather/ || exit 1

cp target/dos/jahdatst.exe _dist_gather/ || exit 1
cp target/dos/dpmcli.exe _dist_gather/ || exit 1
cp target/dos/videxp.exe _dist_gather/ || exit 1
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

rm -rf "$SCRIPT_DIR"/dist/dosxtrs.zip
zip -r "$SCRIPT_DIR"/dist/dosxtrs.zip *

rm -rf "$SCRIPT_DIR"/dist/dxexe.zip
zip -r "$SCRIPT_DIR"/dist/dxexe.zip dosxtrs.exe


rm -rf "$SCRIPT_DIR"/dist/emus.zip
zip -r "$SCRIPT_DIR"/dist/emus.zip EMUS

rm -rf "$SCRIPT_DIR"/dist/jahdatst.zip
zip -r "$SCRIPT_DIR"/dist/jahdatst.zip jahdatst.exe

rm -rf "$SCRIPT_DIR"/dist/dpmcli.zip
zip -r "$SCRIPT_DIR"/dist/dpmcli.zip dpmcli.exe


mkdir DEVLOCAL
rsync -av "$SCRIPT_DIR"/dboxrun/EMUS/LOCAL/ DEVLOCAL/
find DEVLOCAL -iname '*.LOG' -print -exec rm {} \;
rm -rf "$SCRIPT_DIR"/dist/devlocal.zip
zip -r "$SCRIPT_DIR"/dist/devlocal.zip DEVLOCAL


cd "$SCRIPT_DIR"

rm -rf _dist_gather

if [[ ! -f dist/dosxtrs-flp.img ]]; then
  head -c 2949120 /dev/zero > dist/dosxtrs-flp.img
  mkfs.vfat -D 0 -F 12 -g 2/36 -M 0xF0 -n DOSXTRS -S 512 dist/dosxtrs-flp.img
  echo "Made floppy image" 1>&2
fi
if [[ ! -f dist/dosxtrs-flp-b.img ]]; then
  head -c 2949120 /dev/zero > dist/dosxtrs-flp-b.img
  mkfs.vfat -D 0 -F 12 -g 2/36 -M 0xF0 -n DOSXTRS -S 512 dist/dosxtrs-flp-b.img
  echo "Made floppy image" 1>&2
fi

ar="$(mktemp)"

echo '
D:
CD \
MD NDX
CD NDX
UNZIP -o A:\dosxtrs.zip
CD EMUS
REM UNZIP -o A:\devlocal.zip
CD ..
REM LAUNCHER
d:\htst.bat
' | perl -pe 's/\n/\r\n/g' > "$ar"


mdel -i dist/dosxtrs-flp.img ::/dosxtrs.zip 2>/dev/null || true
mdel -i dist/dosxtrs-flp-b.img ::/devlocal.zip 2>/dev/null || true

mcopy -i dist/dosxtrs-flp.img dist/dosxtrs.zip ::/DOSXTRS.ZIP && \
  mcopy -i dist/dosxtrs-flp-b.img dist/devlocal.zip ::/DEVLOCAL.ZIP && \
  mcopy -o -i dist/dosxtrs-flp.img "$ar" ::/AUTORUN.BAT && \
  echo "Updated floppy image"




