@echo off
pushd bin\amd64
signtool sign /a /d "KProcessHacker" kprocesshacker.sys
popd