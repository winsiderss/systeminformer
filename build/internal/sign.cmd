@echo off

if "%1" == "" goto :notset

set additional=
if "%2" == "kmcs" set additional=/ac "%PHBASE%\build\internal\DigiCert High Assurance EV Root CA.crt"

set timestamp=
if "%SIGN_TIMESTAMP%" == "1" set timestamp=/t http://timestamp.digicert.com
set timestamp_rfc=
if "%SIGN_TIMESTAMP%" == "1" set timestamp_rfc=/tr http://timestamp.digicert.com /td sha256

if not "%2" == "sha2only" signtool sign %timestamp% /i "DigiCert High Assurance Code Signing CA-1" %additional% %1
signtool sign /as /fd sha256 %timestamp_rfc% /i "DigiCert SHA2 High Assurance Code Signing CA" %additional% %1

goto :end

:notset
echo Parameters not set.
pause

:end
