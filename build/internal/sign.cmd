@echo off

if "%1" == "" goto :notset

set additional=
if "%2" == "kmcs" set additional=/ac "%PHBASE%\build\internal\DigiCert High Assurance EV Root CA.crt"

signtool sign /t http://timestamp.digicert.com /i "DigiCert High Assurance Code Signing CA-1" %additional% %1

goto :end

:notset
echo Parameters not set.
pause

:end
