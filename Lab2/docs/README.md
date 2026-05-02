# Lab2: LU decomposition with MPI

Source:

```text
Lab2\src\comp_lu.cpp
```

Build from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Build and run the benchmark from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab2\scripts\run_lu_bench.ps1
```

The benchmark builds the Lab2 CMake project in:

```text
Lab2\build
```

It runs all pivoting modes for `n = 10, 50, 100, 500, 1000`,
`procs = 1, 2, 4, 8`, `repeats = 5`, and saves:

```text
Lab2\results\results_lu.csv
```

If a run fails, diagnostic output is saved to:

```text
Lab2\results\logs
```

Analyze benchmark results and build report-ready plots:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab2\scripts\analyze_lu_results.ps1
```

The analysis script saves:

```text
Lab2\results\results_lu_agg.csv
Lab2\results\plots
```

Run:

```powershell
mpiexec -n 4 .\build-msvc\comp_lu.exe --n 500 --pivot none --repeats 3
mpiexec -n 4 .\build-msvc\comp_lu.exe --n 500 --pivot row --repeats 3
mpiexec -n 4 .\build-msvc\comp_lu.exe --n 500 --pivot column --repeats 3
mpiexec -n 4 .\build-msvc\comp_lu.exe --n 500 --pivot global --repeats 3
```

Arguments:

```text
--n <int>          Hilbert matrix size
--pivot none      LU decomposition without pivoting
--pivot row       choose max abs value in row k among columns k..n-1
--pivot column    choose max abs value in column k among rows k..n-1
--pivot global    choose max abs value in the active submatrix
--repeats <int>   number of repeated measurements
```

Output format:

```text
algorithm,n,procs,repeat,time_seconds,reconstruction_error,residual
```

The matrix is the Hilbert matrix `A[i][j] = 1.0 / (i + j + 1)`.
Rows are owned by MPI ranks using `owner(i) = i % world_size`.

For `--pivot row`, the program stores the column permutation `Q`.
The reconstruction error is computed as:

```text
||A_original * Q - L * U||_F / ||A_original||_F
```

For `--pivot column`, the program stores the row permutation `P`.
Rows can be owned by different MPI ranks, so row swaps are performed with
`MPI_Sendrecv`. The reconstruction error is computed as:

```text
||P * A_original - L * U||_F / ||A_original||_F
```

For `--pivot global`, the program stores both row permutation `P` and
column permutation `Q`. The pivot is found in the whole active submatrix
with a custom MPI reduction over `value,row,col`. The reconstruction error is:

```text
||P * A_original * Q - L * U||_F / ||A_original||_F
```
