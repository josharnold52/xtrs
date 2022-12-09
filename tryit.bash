#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd "$SCRIPT_DIR"

mkdir -p dboxrun

rm -f dboxrun/JOSH.LOG

rsync -av cwsdpmi dboxrun/ 
rsync -av ../dist-extras/ dboxrun/

rsync dosxtrs dboxrun/

cp keytrap/target/KEYTRAP.COM ./dboxrun/ || exit 1

echo "MICAH2.CAS 0 1" | perl -pe 's/\n/\r\n/g' > ./dboxrun/_CSTE.CTL

dosbox -c "mount c ./dboxrun" -c "c:" -c "cwsdpmi\\bin\\cwsdpmi.exe" \
   -c "keytrap.com dosxtrs -model 1 -romfile M1L2.ROM -diskdir DSKSET/M1LDOS53"


