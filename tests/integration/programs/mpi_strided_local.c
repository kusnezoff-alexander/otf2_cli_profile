#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    const char* bats_dir = getenv("BATS_TEST_DIRNAME");
    if (!bats_dir) {
        fprintf(stderr, "BATS_TEST_DIRNAME not set\n");
        return 1;
    }

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Each rank writes to its own file
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/out/rank_%d_file.txt", bats_dir, rank);

    FILE* f = fopen(filename, "w+b"); // binary mode allows strided fseek
    if (!f) {
        perror("fopen");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Example strided writes: offsets and sizes
    size_t offsets[] = {0, 40, 80, 120, 160};
    const char* messages[] = {
        "Rank writing 20 bytes!!",  // 20 bytes
        "Rank writes10B",           // 10 bytes
        "7bytes!",                   // 7 bytes
        "Another stride write...",  // example
        "End of writes"             // example
    };
    size_t num_writes = sizeof(offsets) / sizeof(offsets[0]);

    for (size_t i = 0; i < num_writes; ++i) {
        fseek(f, offsets[i], SEEK_SET);
        fwrite(messages[i], 1, strlen(messages[i]), f);
    }

    fflush(f);

    // Optional: read back the file in binary mode
    fseek(f, 0, SEEK_SET);
    unsigned char buffer[200] = {0};
    fread(buffer, 1, sizeof(buffer) - 1, f);

    // Uncomment to print file contents (will include gaps as nulls)
    /*
    printf("Rank %d file contents:\n", rank);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        if (buffer[i] == 0)
            putchar('.');
        else
            putchar(buffer[i]);
    }
    putchar('\n');
    */

    fclose(f);
    MPI_Finalize();
    return 0;
}
