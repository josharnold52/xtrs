#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR

if [[ ! -e ./target || -e ./target/LAUNCHER.COM ]]; then
  echo "Must build first! " 2>&1
fi

cp -f target/LAUNCHER.COM testdata/

dosbox -c "mount c /Users/arnold/devtools/dos16turboc2/prog" -c "mount e $SCRIPT_DIR/testdata" -c "e:" -c "LAUNCHER" 


 
