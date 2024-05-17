param (
  [switch]$run = $false,
  [switch]$test = $false,
  [switch]$publish = $false,
  [switch]$clean = $false)

$ErrorActionPreference = "Stop"

# this function exists because powershell on linux seems to eat stderr without it?
# from https://github.com/PowerShell/PowerShell/issues/20747
function RunCommand {
  param(
    [Parameter(ValueFromRemainingArguments=$true)][String[]]$arguments
  )
  & ($IsWindows ? 'cmd' : 'sh') ($IsWindows ? '/c' : '-c') $arguments ($IsWindows ? '&' : ';') 2>&1
  if ($lastExitCode -ne 0) {
    exit 1
  }
}

Set-Location -Path $PSScriptRoot

if ($clean) {
  Write-Host "Removing build folders..."
  Remove-Item "lua-5.4.2" -Recurse -ErrorAction Ignore 
  Remove-Item "obj" -Recurse -ErrorAction Ignore 
  Remove-Item "publish" -Recurse -ErrorAction Ignore 
  Remove-Item "tcc" -Recurse -ErrorAction Ignore 
  Remove-Item "glfw" -Recurse -ErrorAction Ignore
  exit 0
}

if (-not (Test-Path -Path "tcc/tcc.exe")) {
  $archivePath = "deps/tcc-0.9.27-win32-bin.zip"
  $fileInfo = Get-Item $archivePath
  if ($fileInfo.Length -le 500) {
    throw "$archivePath unexpected file size... most likely git LFS is not installed. Install it and re-clone the repo."
  }
  Write-Host "Extracting tcc"
  Expand-Archive -LiteralPath $archivePath -DestinationPath "." -Force
  if (-not (Test-Path -Path "tcc/tcc.exe")) {
    throw "tried to expand TCC files, but they didn't end up in the right spot"
  }
}

if ($IsWindows -and -not (Test-Path -Path "tcc/include/winapi/mmsystem.h")) {
  $archivePath = "deps/tcc-winapi-full-for-0.9.27.zip"
  $fileInfo = Get-Item $archivePath
  if ($fileInfo.Length -le 500) {
    throw "$archivePath unexpected file size... most likely git LFS is not installed. Install it and re-clone the repo."
  }
  Write-Host "Extracting tcc additional windows api"
  Expand-Archive -LiteralPath $archivePath -DestinationPath "tcc" -Force
  Copy-Item "tcc/winapi-full-for-0.9.27/include" "tcc" -Force -Recurse
  if (-not (Test-Path -Path "tcc/include/winapi/mmsystem.h")) {
    throw "tried to expand TCC winAPI files, but they didn't end up in the right spot"
  }
}

if (-not (Test-Path -Path "lua-5.4.2/src/lapi.h")) {
  $archivePath = "deps/lua-5.4.2.zip"
  $fileInfo = Get-Item $archivePath
  if ($fileInfo.Length -le 500) {
    throw "$archivePath unexpected file size... most likely git LFS is not installed. Install it and re-clone the repo."
  }
  Write-Host "Extracting lua"
  Expand-Archive -LiteralPath $archivePath -DestinationPath "." -Force
  if (-not (Test-Path -Path "lua-5.4.2/src/lapi.h")) {
    throw "tried to expand lua files, but they didn't end up in the right spot"
  }
}

if (-not (Test-Path -Path "glfw/src/glfw.rc.in")) {
  $archivePath = "deps/glfw_3.4.zip"
  $fileInfo = Get-Item $archivePath
  if ($fileInfo.Length -le 500) {
    throw "$archivePath unexpected file size... most likely git LFS is not installed. Install it and re-clone the repo."
  }
  Write-Host "Extracting glfw"
  Expand-Archive -LiteralPath $archivePath -DestinationPath "./glfw" -Force
  if (-not (Test-Path -Path "glfw/src/glfw.rc.in")) {
    throw "tried to expand glfw files, but they didn't end up in the right spot"
  }
  
  # it needs a modification to compile with tcc correctly
  (Get-Content "glfw/src/CMakeLists.txt") | 
    Foreach-Object {
        if ($_ -match "add_library") 
        {
            "add_definitions(-D__STDC_NO_VLA__=1)"
        }
        $_ # send the current line to output
    } | Set-Content "glfw/src/CMakeLists.txt"
}

## TODO: CMake for windows, download from https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-windows-i386.zip

if ((-not $IsWindows) -and -not (Test-Path -Path "glfw/tcc_build/Makefile")) {
  if ($env:XDG_SESSION_TYPE -eq "x11") {
    $glfwArgs = @("-D", "GLFW_BUILD_WAYLAND=0")
  }
  elseif ($env:XDG_SESSION_TYPE -eq "wayland") {
    $glfwArgs = @("-D", "GLFW_BUILD_X11=0")
  }
  else {
    throw "Unrecognized / unsupported XDG_SESSION_TYPE=$($env:XDG_SESSION_TYPE)"
  }
  $glfwArgs += "-DCMAKE_C_COMPILER=tcc"
  $glfwArgs += "-DX11_X11_LIB=who_cares" # see https://github.com/glfw/glfw/issues/1957#issuecomment-1891056066
  #$glfwArgs += "-D__STDC_NO_VLA__=1" # because glfw includes regex.h but tcc doesn't support _REGEX_NELTS correctly somehow
  Write-Host "Generating glfw compilation project"
  & cmake -S glfw -B glfw/tcc_build $glfwArgs
  if ($lastExitCode -ne 0) { exit 1 }
  if (-not (Test-Path -Path "glfw/tcc_build/Makefile")) {
    throw "tried to cmake the glfw files, but the expected makefile was not generated"
  }
}

if ((-not $IsWindows) -and -not (Test-Path -Path "glfw/tcc_build/src/libglfw3.a")) {
  Write-Host "Compiling glfw"
  Push-Location
  Set-Location "glfw/tcc_build/src"
  & make
  if ($lastExitCode -ne 0) { exit 1 }
  Pop-Location
  if (-not (Test-Path -Path "glfw/tcc_build/src/libglfw3.a")) {
    throw "tried to compile glfw, but the expected output file was not generated"
  }
}

$tcc = "tcc"
if ($IsWindows) { $tcc = "./tcc/tcc.exe" }

if (-not (Test-Path -Path "obj/lua.o")) {
  Write-Host "Compiling lua.o"
  $luaFiles = (
    "lapi.c", "lcode.c", "lctype.c", "ldebug.c", "ldo.c", "ldump.c", 
    "lfunc.c", "lgc.c", "llex.c", "lmem.c", "lobject.c", "lopcodes.c", "lparser.c", 
    "lstate.c", "lstring.c", "ltable.c", "ltm.c", "lundump.c", "lvm.c", "lzio.c", 
    "lauxlib.c", "lbaselib.c", "lcorolib.c", "ldblib.c", "liolib.c", "lmathlib.c", 
    "loadlib.c", "loslib.c", "lstrlib.c", "ltablib.c", "lutf8lib.c", "linit.c"
    ) | ForEach-Object { "./lua-5.4.2/src/$($_)" }
  $unused = [System.IO.Directory]::CreateDirectory("obj");
  & $tcc -r -o obj/lua.o $luaFiles
  if ($lastExitCode -ne 0) { exit 1 }
}

$unused = [System.IO.Directory]::CreateDirectory("obj");
$asmObjects = @()
Get-ChildItem "$PSScriptRoot/src" -Filter "*.s" | Foreach-Object {
  $oFilePath = "obj/$([System.IO.Path]::ChangeExtension($_.Name, ".o"))"
  $oFileExists = [System.IO.File]::Exists($oFilePath)
  $oFileTime = if ($oFileExists) { (Get-Item $oFilePath).LastWriteTime } else { Get-Date -Year 1900 }
  
  $sFilePath = $_.FullName
  $sFileExists = [System.IO.File]::Exists($sFilePath)
  $sFileTime = if ($sFileExists) { (Get-Item $sFilePath).LastWriteTime } else { Get-Date -Year 1900 }
  
  # for debugging this
  #Write-Host $sFilePath $sFileExists $sFileTime
  #Write-Host $oFilePath $oFileExists $oFileTime
  if ($sFileTime -gt $oFileTime) {
    Write-Host "Compiling ./src/$($_.Name)"
    & $tcc -r -o $oFilePath $_.FullName
    if ($lastExitCode -ne 0) { exit 1 }
  }
  $asmObjects += $oFilePath
}

if ($test) {
  Get-ChildItem "$PSScriptRoot/test" -Filter *.c | Foreach-Object {
    Write-Host "Compiling " $_.FullName "..."
    if ($IsWindows) {
      & $tcc -g -o "./obj/$($_.Name).exe" $_.FullName ./obj/lua.o $asmObjects "-I./lua-5.4.2/src" "-I./src"
      if ($lastExitCode -ne 0) { exit 1 }
    }
    else {
      & $tcc -g -o "./obj/$($_.Name).exe" $_.FullName ./obj/lua.o $asmObjects "-I./lua-5.4.2/src" "-I./src" -lm
      if ($lastExitCode -ne 0) { exit 1 }
    }
    
    Write-Host "Running tests..."
    RunCommand "./obj/$($_.Name).exe"
  }
  Write-Host "All tests returned exit code 0"

  if ($IsWindows) {
    Write-Host "Compiling lurds2_testApp.exe"
    & $tcc -g -lwinmm -lopengl32 -o lurds2_testApp.exe ./src/lurds2_testApp.c ./obj/lua.o $asmObjects "-I./lua-5.4.2/src"
    if ($lastExitCode -ne 0) { exit 1 }

    Write-Host "Running lurds2_testApp.exe"
    & ./lurds2_testApp.exe
  }
}
else {
  # delete old publish directory first, so there's some time between deleting it and recreating it (because delete is async)
  if ($publish) {
    Write-Host "Removing old 'publish' directory"
    Remove-Item "publish" -Recurse -ErrorAction Ignore 
  }

  Write-Host "Compiling lurds2.exe"
  if ($IsWindows) {
    & $tcc -g -lwinmm -lopengl32 -o lurds2.exe src/lurds2_main.c obj/lua.o $asmObjects "-Ilua-5.4.2/src"
    if ($lastExitCode -ne 0) { exit 1 }
  } else {
    & $tcc -g -o lurds2.exe ./src/lurds2_main.c ./obj/lua.o $asmObjects ./glfw/tcc_build/src/libglfw3.a "-I./lua-5.4.2/src" "-I./glfw/include"
    if ($lastExitCode -ne 0) { exit 1 }
  }

  if ($run) {
    Write-Host "Running lurds2.exe"
    & ./lurds2.exe
  }
  
  if ($publish) {
    Write-Host "Copying files to 'publish' directory"
    $unused = [System.IO.Directory]::CreateDirectory("publish")
    Copy-Item -LiteralPath "lurds2.exe" -Destination "publish"
    Copy-Item -LiteralPath "res" -Destination "publish" -Recurse
  }
}
