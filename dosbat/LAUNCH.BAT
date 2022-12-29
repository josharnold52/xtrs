@ECHO OFF

IF NOT EXIST .\EMUS\%1\CONFIG.XTR GOTO bad

SET /P CONFIG=<.\EMUS\%1\CONFIG.XTR

CD .\EMUS\%1


..\..\KEYTRAP.COM ..\..\DOSXTRS %CONFIG%

CD ..\..

GOTO :done

:bad
ECHO Error %1 not an EMUS

:done
