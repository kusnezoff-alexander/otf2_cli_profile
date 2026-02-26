#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

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

    FILE* f = fopen(filename, "w+b"); // binary mode allows random access
    if (!f) {
        perror("fopen");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Hardcoded random offsets and sizes
    size_t offsets[] = { 0, 37, 92, 150, 211 };  // arbitrary offsets
    size_t sizes[]   = { 15, 22, 7, 30, 18 };    // arbitrary lengths

    const char* messages[] = {
        "Hello from rank!!!",
        "This is a longer message from MPI rank",
        "7bytes!!",
        "Another somewhat longer stride write...",
        "End message for testing"
    };

    size_t num_writes = sizeof(offsets) / sizeof(offsets[0]);

    for (size_t i = 0; i < num_writes; ++i) {
        fseek(f, offsets[i], SEEK_SET);

        // Only write up to the specified size (truncate if message is longer)
        size_t msg_len = strlen(messages[i]);
        size_t write_len = (sizes[i] < msg_len) ? sizes[i] : msg_len;

        fwrite(messages[i], 1, write_len, f);
    }

    fflush(f);

    // // Optional: read back file randomly (just as an example)
    // for (size_t i = 0; i < num_writes; ++i) {
    //     fseek(f, offsets[i], SEEK_SET);
    //
    //     unsigned char buf[50] = {0};
    //     size_t read_len = (sizes[i] < sizeof(buf)) ? sizes[i] : sizeof(buf);
    //     fread(buf, 1, read_len, f);
    //
    //     // Print read back
    //     printf("Rank %d read at offset %zu (%zu bytes): ", rank, offsets[i], read_len);
    //     for (size_t j = 0; j < read_len; ++j) {
    //         putchar(buf[j] ? buf[j] : '.'); // show nulls as dots
    //     }
    //     putchar('\n');
    // }

    fclose(f);
    MPI_Finalize();
    return 0;
}
