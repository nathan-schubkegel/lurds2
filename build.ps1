param (
  [switch]$run = $false
)

$ErrorActionPreference = "Stop"

Set-Location -Path $PSScriptRoot

if (-not (Test-Path -Path "tcc\tcc.exe")) {
  Expand-Archive -LiteralPath "deps\tcc-0.9.27-win32-bin.zip" -DestinationPath "." -Force
}

if (-not (Test-Path -Path "tcc\include\winapi\mmsystem.h")) {
  Expand-Archive -LiteralPath "deps\tcc-winapi-full-for-0.9.27.zip" -DestinationPath "tcc" -Force
  Copy-Item "tcc\winapi-full-for-0.9.27\include" "tcc" -Force -Recurse
  if (-not (Test-Path -Path "tcc\include\winapi\mmsystem.h")) {
    throw "tried to expand winAPI files, but they didn't end up in the right spot"
  }
}

& tcc\tcc.exe -g -lwinmm -lOle32 -lMfplat -o lurds2.exe src\main.c
if (-not $?) { exit 1 }

if ($run) {
  & .\lurds2.exe
}
