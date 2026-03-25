#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char* argv[])
{
    int myrank, nprocs;
    int i, j, k;

    int n = atoi(argv[1]); // размер матрицы из аргумента
    double** a, ** orig;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Выделение памяти
    a = (double**)malloc(n * sizeof(double*));
    orig = (double**)malloc(n * sizeof(double*));
    for (i = 0; i < n; i++) {
        a[i] = (double*)malloc(n * sizeof(double));
        orig[i] = (double*)malloc(n * sizeof(double));
    }

    // Генерация матрицы Гильберта
    if (myrank == 0) {
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++) {
                a[i][j] = 1.0 / (i + j + 1);
                orig[i][j] = a[i][j];
            }
    }

    // Рассылка
    for (i = 0; i < n; i++)
        MPI_Bcast(a[i], n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int* map = (int*)malloc(n * sizeof(int));
    for (i = 0; i < n; i++)
        map[i] = i % nprocs;

    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    // LU-разложение
    for (k = 0; k < n - 1; k++) {
        if (map[k] == myrank) {
            for (j = k + 1; j < n; j++)
                a[k][j] /= a[k][k];
        }

        MPI_Bcast(&a[k][k + 1], n - k - 1, MPI_DOUBLE, map[k], MPI_COMM_WORLD);

        for (i = k + 1; i < n; i++)
            if (map[i] == myrank)
                for (j = k + 1; j < n; j++)
                    a[i][j] -= a[i][k] * a[k][j];
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t2 = MPI_Wtime();

    // Оценка точности (норма ||A - LU||)
    double local_err = 0.0;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {

            double sum = 0.0;

            for (k = 0; k <= (i < j ? i : j); k++) {
                double L = (i == k) ? 1.0 : a[i][k];
                double U = a[k][j];
                sum += L * U;
            }

            double diff = fabs(orig[i][j] - sum);
            if (diff > local_err)
                local_err = diff;
        }
    }

    double global_err;
    MPI_Reduce(&local_err, &global_err, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (myrank == 0) {
        printf("n = %d\n", n);
        printf("Time = %lf sec\n", t2 - t1);
        printf("Error (max norm) = %e\n", global_err);
    }

    MPI_Finalize();
    return 0;
}