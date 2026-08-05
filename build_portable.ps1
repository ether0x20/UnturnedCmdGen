<#
.SYNOPSIS
    Build or package a self-contained portable (USB-friendly) zip of the
    Unturned Command Generator for Windows.

.DESCRIPTION
    Runs the full CMake build and install (including windeployqt) so that every
    Qt runtime DLL lands next to the executable, then zips everything into a
    single portable archive.  No NSIS is required.

    Produces:
        build-win/portable/unturnedcmdgen-<ver>-win64-portable.zip

    The archive can be extracted onto any USB drive and run directly - no
    installation, no Qt runtime, no registry modifications.

.PARAMETER BuildDir
    CMake build directory (default: build-win).  Must already contain a
    configured CMake project (run build.ps1 first).

.PARAMETER BuildType
    Build configuration (default: Release).

.PARAMETER Generator
    CMake generator. Defaults to "Ninja" when Ninja is available; otherwise
    falls back to "Visual Studio 17 2022".  Only used when -Build is passed.

.PARAMETER Build
    Perform a full build + install before packaging.  When omitted the script
    assumes the project has already been built and packages whatever is
    currently in the build directory.

.PARAMETER OutputDir
    Directory where the final zip is placed (default: $BuildDir/portable).

.EXAMPLE
    .\build_portable.ps1

    Package the existing build-win into a portable zip (no rebuild).

.EXAMPLE
    .\build_portable.ps1 -Build

    Full build + install + portable zip in one shot.
#>
param(
    [string]$BuildDir = "build-win",
    [string]$BuildType = "Release",
    [string]$Generator = "",
    [switch]$Build,
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot

function Write-Step([string]$Msg) {
    Write-Host "==> $Msg" -ForegroundColor Cyan
}

# --- Read version from CMakeLists.txt ----------------------------------------
$cmakeLists = Join-Path $Root "CMakeLists.txt"
if (-not (Test-Path -LiteralPath $cmakeLists)) {
    throw "CMakeLists.txt not found at $cmakeLists"
}
$cmakeContent = Get-Content -LiteralPath $cmakeLists -Raw
if ($cmakeContent -match 'project\(\s*unturnedCmdGen\s+VERSION\s+([0-9.]+)') {
    $Version = $Matches[1]
} else {
    throw "Could not parse version from CMakeLists.txt"
}
Write-Step "Project version: $Version"

# --- Default output path -----------------------------------------------------
if (-not $OutputDir) {
    $OutputDir = Join-Path $Root (Join-Path $BuildDir "portable")
}

# --- Build (optional) --------------------------------------------------------
$buildDirAbs = Join-Path $Root $BuildDir
if (-not (Test-Path -LiteralPath $buildDirAbs)) {
    throw "Build directory '$BuildDir' not found. Run build.ps1 first or pass -Build."
}
if (-not (Test-Path -LiteralPath (Join-Path $buildDirAbs "CMakeCache.txt"))) {
    throw "Build directory '$BuildDir' is not configured. Run build.ps1 first or pass -Build."
}

if ($Build) {
    # --- Locate tools ---------------------------------------------------------
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        $cmakeBin = "C:\Program Files\CMake\bin"
        if (Test-Path -LiteralPath (Join-Path $cmakeBin "cmake.exe")) {
            $env:PATH = "$cmakeBin;$env:PATH"
        }
    }
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "cmake is required. Install it from https://cmake.org/download/."
    }

    # MSVC
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
            Write-Step "Setting up MSVC"
            cmd /c "call `"$vcvars`" > nul 2>&1 && set" | ForEach-Object {
                if ($_ -match '^([^=]+)=(.*)$') {
                    Set-Item -Path ("env:" + $Matches[1]) -Value $Matches[2]
                }
            }
        }
    }

    # Generator
    if (-not $Generator) {
        $ninja = Get-Command ninja -ErrorAction SilentlyContinue
        if ($ninja -and (Get-Command cl -ErrorAction SilentlyContinue)) {
            $Generator = "Ninja"
        } else {
            $Generator = "Visual Studio 17 2022"
        }
    }

    Write-Step "Building ($Generator, $BuildType)"
    cmake --build $buildDirAbs --config $BuildType
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }

    Write-Step "Installing to portable staging area (windeployqt runs here)"
    cmake --install $buildDirAbs --prefix $OutputDir --config $BuildType
    if ($LASTEXITCODE -ne 0) { throw "Install failed." }
} else {
    # When not rebuilding, copy from the existing NSIS staging area produced
    # by a prior build.ps1 run.  This avoids re-running the install step and
    # windeployqt when everything is already up to date.
    $stagingSource = Join-Path $buildDirAbs "_CPack_Packages\win64\NSIS\unturnedcmdgen-$Version-win64"
    if (Test-Path -LiteralPath $stagingSource) {
        Write-Step "Copying portable layout from NSIS staging area"
        New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
        Get-ChildItem -LiteralPath $stagingSource | Copy-Item -Destination $OutputDir -Recurse -Force
    } else {
        Write-Step "NSIS staging not found; running install to portable staging"
        cmake --install $buildDirAbs --prefix $OutputDir --config $BuildType
        if ($LASTEXITCODE -ne 0) { throw "Install failed." }
    }
}

# --- Verify the executable is there ------------------------------------------
$exe = Join-Path $OutputDir "UnturnedCmdGen.exe"
if (-not (Test-Path -LiteralPath $exe)) {
    throw "Expected executable not found: $exe"
}
Write-Host "Executable: $exe"

# --- Zip ---------------------------------------------------------------------
$zipName = "unturnedcmdgen-$Version-win64-portable.zip"
$zipPath = Join-Path $Root (Join-Path $BuildDir $zipName)

Write-Step "Creating portable zip: $zipName"
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

# Use .NET's System.IO.Compression for zip creation (available on all Windows
# PowerShell >= 5, no external dependency).
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($OutputDir, $zipPath,
    [System.IO.Compression.CompressionLevel]::Optimal, $false)

Write-Step "Cleaning staging directory"
Remove-Item -LiteralPath $OutputDir -Recurse -Force

# --- Summary -----------------------------------------------------------------
$zipInfo = Get-Item -LiteralPath $zipPath
$sizeMB = [math]::Round($zipInfo.Length / 1MB, 1)

Write-Host ""
Write-Host "Portable zip: $zipPath" -ForegroundColor Green
Write-Host "Size: $sizeMB MB"
Write-Host ""
Write-Host "Usage: extract $zipName to a USB drive and run UnturnedCmdGen.exe" -ForegroundColor DarkGray
