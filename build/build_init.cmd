@echo off
@setlocal enableextensions

call %~dp0\build_thirdparty.cmd INIT
call %~dp0\build_tools.cmd INIT
