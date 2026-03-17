#!/bin/bash

EXEC=./comp_pi
LOG=results.csv

# очищаем файл
echo "n,procs,time" > $LOG

# значения n
N_VALUES=(100 1000 10000 100000 1000000 10000000 100000000 1000000000)

# количество процессов
P_VALUES=(1 2 4 8)

for n in "${N_VALUES[@]}"
do
  for p in "${P_VALUES[@]}"
  do
    echo "Running n=$n procs=$p"

    # передаём n через stdin и парсим время
    output=$(echo $n | mpirun -np $p --use-hwthread-cpus $EXEC)

    # извлекаем время (предполагаем: "time = X sec")
    time=$(echo "$output" | grep "time" | awk '{print $3}')

    echo "$n,$p,$time" >> $LOG
  done
done

echo "Done. Results saved to $LOG"