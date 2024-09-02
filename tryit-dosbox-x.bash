#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd "$SCRIPT_DIR"

export PATH="/usr/gitarnold/gitarnold-dosbox-x/bin:$PATH"

function dosbox {
  dosbox-x -conf dbox-conf/dosbox-x.conf "$@"
}


mkdir -p dboxrun
cd dboxrun || exit 1
unzip -o ../dist/dosxtrs.zip
cd ..
cp dbox-extras/4DOS.COM ./dboxrun/
if [[ -n "$1" ]]; then
  if [[ "$1" == --custom ]]; then
    shift;
    dosbox -c "mount c ./dboxrun" -c "c:" -c "$*"
  else
    dosbox -c "mount c ./dboxrun" -c "c:" -c "4DOS /C LAUNCH.BAT $1"
  fi
else
    dosbox -c "mount c ./dboxrun" -c "c:" -c "4DOS /C LAUNCHER"
fi




exit "$?"

REM The rest of this is old and unused - delete once it looks like the above approach will work



# ROM_ARGS=( -model 1 -romfile M1L2.ROM )

# while [[ $# -gt 0 ]]; do
#   case $1 in
#     --rom)
#       shift
#       if [[ "$1" == "1" ]]; then ROM_ARGS=( -model 1 -romfile M1L1.ROM ) 
#       elif [[ "$1" == "1" ]]; then ROM_ARGS=( -model 1 -romfile M1L2.ROM ) 
#       else { echo "Unknown Rom $1" 1>&2 ; exit 1 ; } 
#       fi
#       shift
#       ;;
#     *)
#       echo "Unknown option $1" 1>&2
#       exit 1
#       ;;
#   esac
# done


# mkdir -p dboxrun

# rm -f dboxrun/JOSH.LOG

# rsync -av cwsdpmi dboxrun/ 
# rsync -av ../dist-extras/ dboxrun/

# rsync dosxtrs dboxrun/

# cp keytrap/target/KEYTRAP.COM ./dboxrun/ || exit 1

# echo "XW_32470.CAS 0 1" | perl -pe 's/\n/\r\n/g' > ./dboxrun/_CSTE.CTL


# #DISKMODE="-diskdir DSKSET/M1LDOS53"
# DISKMODE=""

# dosbox -c "mount c ./dboxrun" -c "c:" -c "cwsdpmi\\bin\\cwsdpmi.exe" \
#    -c "keytrap.com dosxtrs ${ROM_ARGS[*]} $DISKMODE"


