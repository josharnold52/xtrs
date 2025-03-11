#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR

if [[ -e ./target ]]; then
  rm -rf ./target
fi

mkdir target

cp keytrap.c target/
cp scanbuf.h target/

echo "
md misc
tcc -mt -lt keytrap.c > BUILD.OUT
tcc -nmisc -mt -S keytrap.c > ASM.OUT
cd misc
tasm -l keytrap.asm > LIST.OUT
cd ..
" > target/compile.bat


# TODO - Can switch to using --fixname argument of my turboc-20 script

perl -i -pe 's/\n/\r\n/g' -- target/*

#mkdir target/misc

#dosbox -c "mount c /Users/arnold/devtools/dos16turboc2/prog" -c "mount d $SCRIPT_DIR/target" -c 'call d:\compile.bat' -c exit > target/dosbox.log 2>&1

turboc-20 --projdir target --headless 'call d:\compile.bat' >target/dosbox.log 2>&1


perl -i -pe 's/\r//g' -- $(find target -type f ! \( -iname '*.com' -o -iname '*.obj' -o -iname '*.exe' \) )
echo

cat target/BUILD.OUT
 
