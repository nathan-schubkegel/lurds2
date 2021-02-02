param (
  [switch]$run = $false
)

$ErrorActionPreference = "Stop"

Set-Location -Path $PSScriptRoot

if (-not (Test-Path -Path "tcc")) {
  $tccUrl = "http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win32-bin.zip"
  Write-Host "Downloading TCC (Tiny C Compiler) from $tccUrl"
  $tccZip = "tcc.zip"
  (New-Object System.Net.WebClient).DownloadFile($tccUrl, $tccZip)
  Expand-Archive -LiteralPath $tccZip -DestinationPath "."
  if (-not (Test-Path -Path "tcc")) {
    throw "um... I downloaded tcc and unzipped it and expected it to contain a 'tcc' folder, but no such folder was produced!"
  }
  Remove-Item -Path $tccZip
}

$tcc = "tcc\tcc.exe"
& $tcc -o "lurds2.exe" "src\main.c"
if (-not $?) { exit 1 }

if ($run) {
  & ".\lurds2.exe"
}
