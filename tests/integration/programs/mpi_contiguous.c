#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	const char* bats_dir = getenv("BATS_TEST_DIRNAME");
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // printf("Hello from rank %d out of %d processes\n", rank, size);

    // Each rank writes to its own file
    char filename[256];
	snprintf(filename, sizeof(filename), "%s/out/rank_%d_file.txt", bats_dir, rank);

    FILE* f = fopen(filename, "w+"); // create or open for read/write
    if (!f) {
        perror("fopen");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Do multiple writes
    for (int i = 0; i < 5; ++i) {
        fprintf(f, "Rank %d writing line %d\n", rank, i);
    }

    // Flush to make sure data is written
    fflush(f);

    // Move back to beginning and read multiple lines
    fseek(f, 0, SEEK_SET);
    char buffer[128];
    // printf("Rank %d reading file contents:\n", rank);
    // while (fgets(buffer, sizeof(buffer), f) != NULL) {
    //     printf("Rank %d read: %s", rank, buffer);
    // }

    // Do another write after reading
    fprintf(f, "Rank %d appending after read\n", rank);
    fflush(f);

    // Rewind and read again
    fseek(f, 0, SEEK_SET);
    // printf("Rank %d reading file again:\n", rank);
    while (fgets(buffer, sizeof(buffer), f) != NULL) {
        // printf("Rank %d read: %s", rank, buffer);
    }

    fclose(f);

    MPI_Finalize();
    return 0;
}
