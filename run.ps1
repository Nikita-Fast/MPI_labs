param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("comp_pi", "count_primes", "comp_lu")]
    [string]$Program,

    [int]$Processes = 4,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ProgramArgs
)

$ErrorActionPreference = "Stop"

$exe = Join-Path $PSScriptRoot "build-msvc\$Program.exe"
if (-not (Test-Path $exe)) {
    throw "Executable was not found: $exe. Run .\build.ps1 first."
}

mpiexec -n $Processes $exe @ProgramArgs
