# Lab1

## Task 1: compute pi

The source file is:

```text
Lab1\src\comp_pi.cpp
```

Build from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Run with interactive input:

```powershell
mpiexec -n 4 .\build-msvc\comp_pi.exe
```

Run with `n` passed as an argument:

```powershell
mpiexec -n 4 .\build-msvc\comp_pi.exe 1000000
```

## Task 2: investigate `n` and process count for pi

Run the benchmark:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab1\scripts\benchmark_lab1.ps1
```

By default, the pi benchmark uses `n` up to `1000000000`.
You can set a custom list for pi without changing the prime-counting benchmark:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab1\scripts\benchmark_lab1.ps1 -PiNValues 1000000,10000000,100000000,1000000000 -Processes 1,2,4,8
```

The pi results are saved to:

```text
Lab1\results\results_pi.csv
```

## Task 3: count primes with MPI

Run manually:

```powershell
mpiexec -n 4 .\build-msvc\count_primes.exe 1000000
```

The same benchmark script also saves prime-counting results to:

```text
Lab1\results\results_primes.csv
```

To set a separate task-size list for prime counting:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab1\scripts\benchmark_lab1.ps1 -PrimeNValues 10000,100000,1000000,10000000
```

## Plot benchmark results

The repository uses a local Python virtual environment:

```powershell
C:\Users\K02\AppData\Local\Programs\Python\Python310\python.exe -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip matplotlib
```

Build both plots from the CSV files:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab1\scripts\plot_lab1_results.ps1
```

The images are saved to:

```text
Lab1\results\plots\runtime_pi.png
Lab1\results\plots\runtime_primes.png
```

You can also plot any CSV with the same columns:

```powershell
.\.venv\Scripts\python.exe .\Lab1\scripts\plot_runtime.py .\Lab1\results\results_pi.csv -o .\Lab1\results\plots\runtime_pi.png --title "Pi computation"
```
