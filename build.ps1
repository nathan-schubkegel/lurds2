param (
  [switch]$run = $false,
  [switch]$test = $false,
  [switch]$publish = $false,
  [switch]$clean = $false)

$ErrorActionPreference = "Stop"

Set-Location -Path $PSScriptRoot

if ($clean) {
  Write-Host "Removing build folders..."
  Remove-Item "lua-5.4.2" -Recurse -ErrorAction Ignore 
  Remove-Item "obj" -Recurse -ErrorAction Ignore 
  Remove-Item "publish" -Recurse -ErrorAction Ignore 
  Remove-Item "tcc" -Recurse -ErrorAction Ignore 
  exit 0
}

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

if (-not (Test-Path -Path "lua-5.4.2\src\lapi.h")) {
  Write-Host "Extracting lua"
  Expand-Archive -LiteralPath "deps\lua-5.4.2.zip" -DestinationPath "." -Force
  if (-not (Test-Path -Path "lua-5.4.2\src\lapi.h")) {
    throw "tried to expand lua files, but they didn't end up in the right spot"
  }
}

if (-not (Test-Path -Path "obj\lua.o")) {
  Write-Host "Compiling lua.o"
  $luaFiles = (
    "lapi.c", "lcode.c", "lctype.c", "ldebug.c", "ldo.c", "ldump.c", 
    "lfunc.c", "lgc.c", "llex.c", "lmem.c", "lobject.c", "lopcodes.c", "lparser.c", 
    "lstate.c", "lstring.c", "ltable.c", "ltm.c", "lundump.c", "lvm.c", "lzio.c", 
    "lauxlib.c", "lbaselib.c", "lcorolib.c", "ldblib.c", "liolib.c", "lmathlib.c", 
    "loadlib.c", "loslib.c", "lstrlib.c", "ltablib.c", "lutf8lib.c", "linit.c"
    ) | ForEach-Object { "lua-5.4.2\src\$($_)" }
  $unused = [System.IO.Directory]::CreateDirectory("obj");
  & tcc\tcc.exe -r -o obj\lua.o $luaFiles
  if (-not $?) { exit 1 }
}

if ($test) {
  Write-Host "Compiling lurds2_testApp.exe"
  & tcc\tcc.exe -g -lwinmm -lopengl32 -o lurds2_testApp.exe src\lurds2_testApp.c obj\lua.o "-Ilua-5.4.2\src"
  if (-not $?) { exit 1 }

  Write-Host "Running lurds2_testApp.exe"
  & .\lurds2_testApp.exe
}
else {
  # delete old publish directory first, so there's some time between deleting it and recreating it (because delete is async)
  if ($publish) {
    Write-Host "Removing old 'publish' directory"
    Remove-Item "publish" -Recurse -ErrorAction Ignore 
  }

  Write-Host "Compiling lurds2.exe"
  & tcc\tcc.exe -g -lwinmm -lopengl32 -o lurds2.exe src\lurds2_main.c obj\lua.o "-Ilua-5.4.2\src"
  if (-not $?) { exit 1 }

  if ($run) {
    Write-Host "Running lurds2.exe"
    & .\lurds2.exe
  }
  
  if ($publish) {
    Write-Host "Copying files to 'publish' directory"
    $unused = [System.IO.Directory]::CreateDirectory("publish")
    Copy-Item -LiteralPath "lurds2.exe" -Destination "publish"
    Copy-Item -LiteralPath "res" -Destination "publish" -Recurse
  }
}