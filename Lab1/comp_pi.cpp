#include <iostream>
#include <mpi.h>

using namespace std;

double f(double x)
{
    return 1/(1+x*x);
}

int main(int argc, char *argv[])
{
    double pi, sum = 0, term, h;
    int myrank, nprocs, n, i;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if(myrank == 0)
    {
        cout << "Number of iterations=";
        cin >> n;
    }

    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);

    h = 1.0/n;

    // --- ДОБАВЛЕНО: синхронизация и старт времени ---
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    for(i = myrank+1; i <= n; i += nprocs)
        sum += f(h*(i-0.5));

    term = 4*h*sum;

    MPI_Reduce(&term, &pi, 1, MPI_DOUBLE, MPI_SUM,0,MPI_COMM_WORLD);

    // --- ДОБАВЛЕНО: конец времени ---
    double end = MPI_Wtime();

    if(myrank == 0) {
        cout << "Computed value of pi=" << pi << endl;
        cout << "time = " << (end - start) << endl;
    }

    MPI_Finalize();
    return 0;
}