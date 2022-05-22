
rm JOSH.LOG 

cp keytrap/target/KEYTRAP.COM ./ || exit 1

dosbox -c "mount c ." -c "c:" -c "cwsdpmi\\bin\\cwsdpmi.exe" \
   -c "keytrap.com dosxtrs -model 1 -romfile M1L1.ROM"
