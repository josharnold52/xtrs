d:
md \ndx 
cd \ndx

if exist dosxtrs.zip del dosxtrs.zip

REM This address and port will probably change...

c:\net\mtcp\htget.exe http://192.168.21.72:7777/dosxtrs.zip >dosxtrs.zip
unzip -o dosxtrs.zip

 
