param(
    [string[]]$NValues = @(),
    [string[]]$PiNValues = @("100", "1000", "10000", "100000", "1000000", "10000000", "100000000", "1000000000"),
    [string[]]$PrimeNValues = @("100", "1000", "10000", "100000", "1000000", "10000000"),
    [string[]]$Processes = @("1", "2", "4", "8"),
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$labDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$buildScript = Join-Path $root "build.ps1"
$buildDir = Join-Path $root "build-msvc"
$resultsDir = Join-Path $labDir "results"
$resultsPi = Join-Path $resultsDir "results_pi.csv"
$resultsPrimes = Join-Path $resultsDir "results_primes.csv"

function Convert-ToIntList {
    param([string[]]$Values)

    return $Values |
        ForEach-Object { $_ -split "," } |
        Where-Object { $_ -ne "" } |
        ForEach-Object { [int]$_ }
}

if ($NValues.Count -gt 0) {
    $PiNValues = $NValues
    $PrimeNValues = $NValues
}

$PiNValues = Convert-ToIntList $PiNValues
$PrimeNValues = Convert-ToIntList $PrimeNValues
$Processes = Convert-ToIntList $Processes

powershell -ExecutionPolicy Bypass -File $buildScript -Config $Config

New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

"program,n,procs,time_seconds" | Set-Content -Path $resultsPi
"program,n,procs,time_seconds" | Set-Content -Path $resultsPrimes

function Invoke-MpiProgram {
    param(
        [string]$Program,
        [int]$N,
        [int]$ProcessCount
    )

    $exe = Join-Path $buildDir "$Program.exe"
    $output = & mpiexec -n $ProcessCount $exe $N 2>&1
    $text = $output -join "`n"

    if ($text -match "time\s*=\s*([0-9.eE+-]+)") {
        return $matches[1]
    }

    if ($text -match "Exec time:\s*([0-9.eE+-]+)") {
        return $matches[1]
    }

    throw "Cannot parse runtime for $Program, n=$N, procs=$ProcessCount. Output: $text"
}

foreach ($n in $PiNValues) {
    foreach ($p in $Processes) {
        Write-Host "Running comp_pi: n=$n procs=$p"
        $time = Invoke-MpiProgram -Program "comp_pi" -N $n -ProcessCount $p
        "comp_pi,$n,$p,$time" | Add-Content -Path $resultsPi
    }
}

foreach ($n in $PrimeNValues) {
    foreach ($p in $Processes) {
        Write-Host "Running count_primes: n=$n procs=$p"
        $time = Invoke-MpiProgram -Program "count_primes" -N $n -ProcessCount $p
        "count_primes,$n,$p,$time" | Add-Content -Path $resultsPrimes
    }
}

Write-Host "Saved $resultsPi"
Write-Host "Saved $resultsPrimes"
