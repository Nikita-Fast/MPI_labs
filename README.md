# Лабораторные работы по MPI

Репозиторий содержит C++17-программы для лабораторных работ по MPI под Windows 11.

## Среда

Используется:

- Windows 11;
- Microsoft MPI Runtime и Microsoft MPI SDK;
- MSVC из Visual Studio / Build Tools;
- CMake;
- PowerShell;
- Python `.venv` для анализа результатов и построения графиков.

## Структура

```text
Lab1/
  src/       исходные C++ файлы
  scripts/   скрипты сборов данных и построения графиков
  results/   CSV-файлы и PNG-графики
  docs/      задание, README и отчетные материалы

Lab2/
  src/       исходный C++ файл LU-разложения
  scripts/   benchmark и анализ результатов
  results/   CSV-файлы, агрегированные данные, графики и логи
  docs/      задание, README, LaTeX-отчет и PDF
```

Корневой `CMakeLists.txt` и `build.ps1` собирают общие исполняемые файлы для лабораторных.

## Общая сборка

Из корня проекта:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

После сборки исполняемые файлы появляются в:

```text
build-msvc\
```

## Запуск программ

Примеры:

```powershell
mpiexec -n 4 .\build-msvc\comp_pi.exe 1000000
mpiexec -n 4 .\build-msvc\count_primes.exe 1000000
mpiexec -n 4 .\build-msvc\comp_lu.exe --n 500 --pivot global --repeats 3
```

Можно также использовать wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File .\run.ps1 comp_pi -Processes 4 1000000
powershell -ExecutionPolicy Bypass -File .\run.ps1 count_primes -Processes 4 1000000
powershell -ExecutionPolicy Bypass -File .\run.ps1 comp_lu -Processes 4 --n 500 --pivot global --repeats 3
```

## Lab1

Сбор данных:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab1\scripts\benchmark_lab1.ps1
```

Построение графиков:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab1\scripts\plot_lab1_results.ps1
```

Подробности находятся в:

```text
Lab1\docs\README.md
```

## Lab2

Полный benchmark LU-разложения:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab2\scripts\run_lu_bench.ps1
```

Анализ CSV и построение графиков:

```powershell
powershell -ExecutionPolicy Bypass -File .\Lab2\scripts\analyze_lu_results.ps1
```

Основные результаты:

```text
Lab2\results\results_lu.csv
Lab2\results\results_lu_agg.csv
Lab2\results\plots\
```

Подробности находятся в:

```text
Lab2\docs\README.md
Lab2\docs\report_lab2.tex
```

## Очистка

Сборочные каталоги `build`, `build-msvc` и `Lab2\build` являются временными артефактами и игнорируются Git.
