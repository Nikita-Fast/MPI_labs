#include <mpi.h>
#include <stdio.h>

// проверка на простоту
int is_prime(int num) {
    if (num < 2) return 0;
    if (num == 2) return 1;
    if (num % 2 == 0) return 0;

    for (int i = 3; i * i <= num; i += 2) {
        if (num % i == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char** argv) {
    int rank, size;
    int n;
    double time_start, time_finish;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ввод только на процессе 0
    if (rank == 0) {
        printf("Enter n: ");
        fflush(stdout);

        scanf("%d", &n);
    }

    time_start = MPI_Wtime();

    // Рассылка значения всем процессам
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //printf("process %d, n=%d, size=%d\n", rank, n, size);
    //fflush(stdout);

    int local_count = 0;

    for (int i = rank + 1; i <= n; i += size) {
        if (is_prime(i)) {
            local_count++;
            //printf("process %d: %d is prime\n", rank, i);
        }
    }

    int global_count;

    // Суммирование результатов
    MPI_Reduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Вывод результата
    if (rank == 0) {
        time_finish = MPI_Wtime();
        printf("Number of primes from 1 to %d: %d\n", n, global_count);
        printf("Exec time: %f", time_finish - time_start);
    }

    MPI_Finalize();
    return 0;
}
