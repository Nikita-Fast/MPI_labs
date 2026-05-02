$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$python = Join-Path $root ".venv\Scripts\python.exe"
$script = Join-Path $PSScriptRoot "analyze_lu_results.py"

if (-not (Test-Path $python)) {
    throw "Python virtual environment was not found: $python"
}

& $python $script
