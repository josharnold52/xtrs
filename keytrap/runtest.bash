#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR

if [[ ! -e ./target ]]; then
  echo "Must build first! " 2>&1
fi

dosbox -c "mount c /Users/arnold/devtools/dos16turboc2/prog" -c "mount d $SCRIPT_DIR/target" -c "d:"


 
