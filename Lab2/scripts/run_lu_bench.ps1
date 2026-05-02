param(
    [string[]]$Algorithms = @("none", "row", "column", "global"),
    [string[]]$NValues = @("10", "50", "100", "500", "1000"),
    [string[]]$Processes = @("1", "2", "4", "8"),
    [int]$Repeats = 5
)

$ErrorActionPreference = "Stop"

$labDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildDir = Join-Path $labDir "build"
$exePath = Join-Path $buildDir "Release\comp_lu.exe"
$exeCommand = ".\build\Release\comp_lu.exe"
$resultsDir = Join-Path $labDir "results"
$resultsCsv = Join-Path $resultsDir "results_lu.csv"
$logsDir = Join-Path $resultsDir "logs"
$vcvars = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

function Convert-ToStringList {
    param([string[]]$Values)

    return $Values |
        ForEach-Object { $_ -split "," } |
        Where-Object { $_ -ne "" } |
        ForEach-Object { $_.Trim() }
}

function Convert-ToIntList {
    param([string[]]$Values)

    return Convert-ToStringList $Values | ForEach-Object { [int]$_ }
}

$Algorithms = Convert-ToStringList $Algorithms
$NValues = Convert-ToIntList $NValues
$Processes = Convert-ToIntList $Processes

foreach ($algorithm in $Algorithms) {
    if ($algorithm -notin @("none", "row", "column", "global")) {
        throw "Unsupported algorithm: $algorithm"
    }
}

if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat was not found: $vcvars"
}

New-Item -ItemType Directory -Force -Path $resultsDir, $logsDir | Out-Null

cmd /d /s /c "`"$vcvars`" && cmake -S `"$labDir`" -B `"$buildDir`" -G `"NMake Makefiles`" -DCMAKE_BUILD_TYPE=Release && cmake --build `"$buildDir`""
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path $exePath)) {
    throw "Executable was not found after build: $exePath"
}

"algorithm,n,procs,repeat,time_seconds,reconstruction_error,residual" | Set-Content -Path $resultsCsv -Encoding utf8

Push-Location $labDir
try {
    foreach ($algorithm in $Algorithms) {
        foreach ($n in $NValues) {
            foreach ($p in $Processes) {
                Write-Host "Running pivot=$algorithm n=$n procs=$p repeats=$Repeats"

                $output = & mpiexec -n $p $exeCommand --n $n --pivot $algorithm --repeats $Repeats 2>&1
                $exitCode = $LASTEXITCODE
                $text = $output -join "`n"

                if ($exitCode -ne 0) {
                    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
                    $logName = "lu_${algorithm}_n${n}_p${p}_${timestamp}.log"
                    $logPath = Join-Path $logsDir $logName

                    @(
                        "command: mpiexec -n $p $exeCommand --n $n --pivot $algorithm --repeats $Repeats"
                        "exit_code: $exitCode"
                        ""
                        $text
                    ) | Set-Content -Path $logPath -Encoding utf8

                    Write-Warning "Run failed. Diagnostic output saved to $logPath"
                    continue
                }

                $csvLines = $output | Where-Object {
                    $_ -match "^lu_hilbert_.*,\d+,\d+,\d+,"
                }

                if ($csvLines.Count -eq 0) {
                    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
                    $logName = "lu_${algorithm}_n${n}_p${p}_${timestamp}_no_csv.log"
                    $logPath = Join-Path $logsDir $logName

                    @(
                        "command: mpiexec -n $p $exeCommand --n $n --pivot $algorithm --repeats $Repeats"
                        "exit_code: $exitCode"
                        "reason: no CSV output detected"
                        ""
                        $text
                    ) | Set-Content -Path $logPath -Encoding utf8

                    Write-Warning "No CSV output detected. Diagnostic output saved to $logPath"
                    continue
                }

                $csvLines | Add-Content -Path $resultsCsv -Encoding utf8
            }
        }
    }
} finally {
    Pop-Location
}

Write-Host "Saved results to $resultsCsv"
Write-Host "Diagnostic logs directory: $logsDir"
