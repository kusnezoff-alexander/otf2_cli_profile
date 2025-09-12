/**
 * For testing that scorep works
 */
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv); // Initialize MPI

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get process rank
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get number of processes

    printf("Hello from rank %d out of %d processes\n", rank, size);

    MPI_Finalize(); // Finalize MPI
    return 0;
}
