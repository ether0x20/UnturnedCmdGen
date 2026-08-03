<#
.SYNOPSIS
    Build the Unturned Command Generator for Windows and (optionally) package
    an NSIS installer.

.DESCRIPTION
    Configures and builds the project with CMake, then runs windeployqt (via
    the CMake install step) to bundle the Qt runtime and produces an installer
    with CPack + NSIS.

    Works from a plain PowerShell: it locates CMake/Ninja/NSIS and sets up the
    MSVC environment (vcvars64.bat) automatically.

    Produces:
        build-win/UnturnedCmdGen.exe              - the executable
        build-win/unturnedcmdgen-<ver>-win64.exe  - the NSIS installer

.PARAMETER BuildDir
    CMake build directory (default: build-win).

.PARAMETER BuildType
    Build configuration (default: Release).

.PARAMETER Generator
    CMake generator. Defaults to "Ninja" when Ninja is available; otherwise
    falls back to "Visual Studio 17 2022".

.PARAMETER SkipInstaller
    Only build the executable, do not run CPack/NSIS.

.EXAMPLE
    .\build.ps1

    Build with defaults and produce the installer.

.EXAMPLE
    .\build.ps1 -BuildDir out -SkipInstaller

    Only build the executable into the "out" directory.
#>
param(
    [string]$BuildDir = "build-win",
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [switch]$SkipInstaller
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot

function Write-Step([string]$Msg) {
    Write-Host "==> $Msg" -ForegroundColor Cyan
}

# --- Locate tools and put them on PATH --------------------------------------
# CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $cmakeDir = "C:\Program Files\CMake\bin"
    if (Test-Path -LiteralPath (Join-Path $cmakeDir "cmake.exe")) {
        $env:PATH = "$cmakeDir;$env:PATH"
    }
}
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake is required. Install it from https://cmake.org/download/."
}

# Ninja (optional)
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if (-not $ninja) {
    $ninjaDir = "C:\Program Files\CMake\bin"
    if (Test-Path -LiteralPath (Join-Path $ninjaDir "ninja.exe")) {
        $env:PATH = "$ninjaDir;$env:PATH"
        $ninja = Get-Command ninja -ErrorAction SilentlyContinue
    }
}

# MSVC environment (vcvars64.bat) so cl.exe etc. are available
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    $vsEditions = @("C:\Program Files (x86)\Microsoft Visual Studio\2022",
                    "C:\Program Files\Microsoft Visual Studio\2022")
    $vcvars = $null
    foreach ($vs in $vsEditions) {
        foreach ($ed in @("BuildTools", "Community", "Professional", "Enterprise")) {
            $candidate = Join-Path $vs (Join-Path $ed "VC\Auxiliary\Build\vcvars64.bat")
            if (Test-Path -LiteralPath $candidate) { $vcvars = $candidate; break }
        }
        if ($vcvars) { break }
    }
    if ($vcvars) {
        Write-Step "Setting up MSVC environment via $vcvars"
        $tmp = Join-Path $env:TEMP "vcvars_out_$PID.txt"
        cmd /c "call `"$vcvars`" > `"$tmp`" 2>&1 && set" | ForEach-Object {
            if ($_ -match '^([^=]+)=(.*)$') {
                Set-Item -Path ("env:" + $Matches[1]) -Value $Matches[2]
            }
        }
        Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
    } elseif (-not $Generator) {
        Write-Warning "Could not locate vcvars64.bat; will try the Visual Studio generator instead."
    }
}

# --- Generator selection ----------------------------------------------------
if (-not $Generator) {
    if ($ninja -and (Get-Command cl -ErrorAction SilentlyContinue)) {
        $Generator = "Ninja"
    } else {
        Write-Step "Using the Visual Studio generator"
        $Generator = "Visual Studio 17 2022"
    }
}

# --- Locate Qt (for CMake's find_package(Qt6)) -------------------------------
$qtRoot = ""
if (Test-Path "C:\Qt") {
    foreach ($ver in Get-ChildItem "C:\Qt" -Directory -ErrorAction SilentlyContinue) {
        if ($ver.Name -notmatch '^\d+\.') { continue }
        foreach ($kit in Get-ChildItem $ver.FullName -Directory -ErrorAction SilentlyContinue) {
            if ($kit.Name -notmatch 'msvc|mingw') { continue }
            if (Test-Path -LiteralPath (Join-Path $kit.FullName "lib\cmake\Qt6\Qt6Config.cmake")) {
                if (-not $qtRoot) { $qtRoot = $kit.FullName }
            }
        }
    }
}
if (-not $qtRoot) {
    throw "Qt6 not found. Install Qt (with the MSVC/Mingw kit) under C:\Qt or set CMAKE_PREFIX_PATH manually."
}
Write-Step "Using Qt from $qtRoot"

$isMultiConfig = $Generator -like "*Visual Studio*"

Write-Step "Configuring ($Generator, $BuildType) in $BuildDir"
$configureArgs = @("-S", ".", "-B", $BuildDir, "-G", $Generator,
                   "-DCMAKE_PREFIX_PATH=$qtRoot")
if (-not $isMultiConfig) {
    $configureArgs += @("-DCMAKE_BUILD_TYPE=$BuildType")
}
cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

Write-Step "Building executable"
cmake --build $BuildDir --config $BuildType
if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }

$exe = Join-Path $Root (Join-Path $BuildDir "UnturnedCmdGen.exe")
if (-not (Test-Path -LiteralPath $exe)) {
    # Multi-config generators (Visual Studio) put binaries under <cfg>/
    $exe = Join-Path $Root (Join-Path $BuildDir (Join-Path $BuildType "UnturnedCmdGen.exe"))
}
if (-not (Test-Path -LiteralPath $exe)) {
    throw "Expected build output not found: $exe"
}
Write-Host "Executable: $exe"

if ($SkipInstaller) {
    Write-Host "Skipping installer as requested."
    exit 0
}

# --- NSIS ------------------------------------------------------------------
$nsis = Get-Command makensis -ErrorAction SilentlyContinue
if (-not $nsis) {
    $nsisDefault = "C:\Program Files (x86)\NSIS"
    if (Test-Path -LiteralPath (Join-Path $nsisDefault "makensis.exe")) {
        $env:PATH = "$nsisDefault;$env:PATH"
        $nsis = Get-Command makensis -ErrorAction SilentlyContinue
    }
}
if (-not $nsis) {
    throw "makensis not found. Install NSIS from https://nsis.sourceforge.io (or pass -SkipInstaller)."
}
Write-Step "Packaging NSIS installer (windeployqt runs during the CPack staging install)"
cpack --config "$BuildDir\CPackConfig.cmake" -G NSIS -C $BuildType -B $BuildDir
if ($LASTEXITCODE -ne 0) { throw "CPack failed." }

$installer = Get-ChildItem -LiteralPath (Join-Path $Root $BuildDir) -Filter "unturnedcmdgen-*-win64.exe" |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
Write-Host "Installer: $($installer.FullName)"
