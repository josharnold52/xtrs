#!/bin/bash
cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" || exit 1

# This patches CWSDPMI.EXE to only request 16M when running in XMS mode (using HIMEM.SYS/HIMEMX.SYS, but
# no memory manager such as EMM386 or JEMM/JEMM386).   Normally it would allocate all of memory - this patch
# makes it allocate the hardcoded amount of 16M (it would fail if 16M is not available).  This leaves XMS
# space available for allocation within dosxtrs.
#
# See the README in this directory for patch details

infile=../cwsdpmi/BIN/CWSDPMI.EXE
outfile=./C16MDPMI.EXE
outfile2=./CM1MDPMI.EXE

rm -f "$outfile"
rm -f "$outfile2"

xxd -p "$infile" | tr -d '\n' |  perl -pe 's/b488ff1e943d0a/ba0000b80040c3/' | xxd -r -p > "$outfile"
diff -U3 <(hexdump -C "$infile")  <(hexdump -C "$outfile") | head -20 > c16mdpmi.diff


xxd -p "$infile" | tr -d '\n' | \
  perl -pe 's/0adb7508668bd066c1ea10c3b408ff1e943d33d2c3/6d2d000400000adb74036631c0668bd066c1ea10c3/' | \
  xxd -r -p > "$outfile2"
diff -U3 <(hexdump -C "$infile")  <(hexdump -C "$outfile2") | head -20 > cm1mdpmi.diff

[[ "$(cat "$infile" | wc -c)" = "$(cat "$outfile" | wc -c)" ]] \
  && [[ "$(cat "$infile" | wc -c)" = "$(cat "$outfile2" | wc -c)" ]]

rc="$?"
echo "patch_cwsdpmi result = $rc"
exit "$rc"

