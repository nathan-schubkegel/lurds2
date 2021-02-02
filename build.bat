@pushd %~dp0
@powershell -ExecutionPolicy Bypass .\build.ps1 %*
@popd