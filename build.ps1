param (
  [switch]$run = $false
)

$ErrorActionPreference = "Stop"

Set-Location -Path $PSScriptRoot

if (-not (Test-Path -Path "tcc\tcc.exe")) {
  Write-Host "Extracting tcc"
  Expand-Archive -LiteralPath "deps\tcc-0.9.27-win32-bin.zip" -DestinationPath "." -Force
  if (-not (Test-Path -Path "tcc\tcc.exe")) {
    throw "tried to expand TCC files, but they didn't end up in the right spot"
  }
}

if (-not (Test-Path -Path "tcc\include\winapi\mmsystem.h")) {
  Write-Host "Extracting tcc additional windows api"
  Expand-Archive -LiteralPath "deps\tcc-winapi-full-for-0.9.27.zip" -DestinationPath "tcc" -Force
  Copy-Item "tcc\winapi-full-for-0.9.27\include" "tcc" -Force -Recurse
  if (-not (Test-Path -Path "tcc\include\winapi\mmsystem.h")) {
    throw "tried to expand TCC winAPI files, but they didn't end up in the right spot"
  }
}

Write-Host "Compiling lurds2.exe"
& tcc\tcc.exe -g -lwinmm -o lurds2.exe src\main.c
if (-not $?) { exit 1 }

if ($run) {
  Write-Host "Running lurds2.exe"
  & .\lurds2.exe
}
