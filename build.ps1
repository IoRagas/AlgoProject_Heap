param(
    [string]$Config = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Arch = "x64",
    [string]$BuildDir = "build",
    [switch]$Clean,
    [switch]$SkipRun,
    [switch]$SameWindow
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildPath = Join-Path $root $BuildDir
$exePathWithConfig = Join-Path $buildPath "$Config/AlgoProject.exe"
$exePathFlat = Join-Path $buildPath "AlgoProject.exe"

if ($Clean -and (Test-Path $buildPath)) {
    Write-Host "Removing build directory: $buildPath"
    Remove-Item -Recurse -Force $buildPath
}

if (!(Test-Path $buildPath)) {
    New-Item -ItemType Directory -Path $buildPath | Out-Null
}

$cachePath = Join-Path $buildPath "CMakeCache.txt"

if (Test-Path $cachePath) {
    $cacheContent = Get-Content $cachePath
    $cachedGeneratorLine = $cacheContent | Where-Object { $_ -like "CMAKE_GENERATOR:INTERNAL=*" } | Select-Object -First 1
    $cachedPlatformLine = $cacheContent | Where-Object { $_ -like "CMAKE_GENERATOR_PLATFORM:INTERNAL=*" } | Select-Object -First 1
    $cachedGenerator = $null
    $cachedPlatform = $null
    if ($cachedGeneratorLine) { $cachedGenerator = $cachedGeneratorLine.Split('=')[-1] }
    if ($cachedPlatformLine) { $cachedPlatform = $cachedPlatformLine.Split('=')[-1] }

    $platformTarget = if ($Arch -eq "") { "" } else { $Arch }
    if (($cachedGenerator -and $cachedGenerator -ne $Generator) -or ($cachedPlatform -ne $platformTarget)) {
        Write-Host "[cmake] Generator/platform changed (cache: '$cachedGenerator'/'$cachedPlatform'). Recreating $BuildDir."
        Remove-Item -Recurse -Force $buildPath
        New-Item -ItemType Directory -Path $buildPath | Out-Null
    }
}

Write-Host "[cmake] Configuring ($Generator, $Arch)"
$archArgs = @()
if ($Arch -ne "") { $archArgs += @("-A", $Arch) }
cmake -S $root -B $buildPath -G "$Generator" @archArgs | Write-Host

Write-Host "[cmake] Building ($Config)"
cmake --build $buildPath --config $Config | Write-Host

if (-not $SkipRun) {
    $exeToRun = if (Test-Path $exePathWithConfig) { $exePathWithConfig } elseif (Test-Path $exePathFlat) { $exePathFlat } else { $null }
    if (-not $exeToRun) {
        throw "Executable not found under $buildPath."
    }

    if ($SameWindow) {
        Write-Host "[run] Launching in current window: $exeToRun"
        & $exeToRun
    } else {
        Write-Host "[run] Launching in new window: $exeToRun"
        Start-Process -FilePath $exeToRun -WorkingDirectory (Split-Path $exeToRun)
    }
}
