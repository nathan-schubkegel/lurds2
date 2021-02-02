$ErrorActionPreference = "Stop"

if (-not (Test-Path -Path "$PSScriptRoot\tcc")) {
  $tccUrl = "http://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win32-bin.zip"
  Write-Host "Downloading TCC (Tiny C Compiler) from $tccUrl"
  $tccZip = "$PSScriptRoot\tcc.zip"
  (New-Object System.Net.WebClient).DownloadFile($tccUrl, $tccZip)
  Expand-Archive -LiteralPath $tccZip -DestinationPath "$PSScriptRoot"
  if (-not (Test-Path -Path "$PSScriptRoot\tcc")) {
    throw "um... I downloaded tcc and unzipped it and expected it to contain a 'tcc' folder, but no such folder was produced!"
  }
  Remove-Item -Path $tccZip
}

