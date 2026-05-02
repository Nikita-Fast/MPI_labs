param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$vcvars = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat was not found: $vcvars"
}

$buildDir = "build-msvc"

cmd /d /s /c "`"$vcvars`" && cmake -S . -B $buildDir -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=$Config && cmake --build $buildDir"
