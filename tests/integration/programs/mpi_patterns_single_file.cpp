#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Example data per rank
    char buf[50];
    snprintf(buf, sizeof(buf), "Rank %d data\n", rank);
    size_t buf_len = strlen(buf);

    // --------------------
    // 1) Contiguous access
    // --------------------
    MPI_File contiguous_fh;
    MPI_File_open(MPI_COMM_WORLD, "contiguous.txt",
                  MPI_MODE_CREATE | MPI_MODE_WRONLY,
                  MPI_INFO_NULL, &contiguous_fh);

    // Example hardcoded starting offsets per rank
    std::vector<MPI_Offset> contiguous_offsets{0, 100, 231}; // specify offsets at which each rank is going to access the file
    // Extend as needed for more ranks
    MPI_Offset contig_offset = contiguous_offsets[rank];

    MPI_File_write_at(contiguous_fh, contig_offset, buf, buf_len, MPI_CHAR, MPI_STATUS_IGNORE);
    MPI_File_close(&contiguous_fh);

    // --------------------
    // 2) Strided access
    // --------------------
    MPI_File strided_fh;
    MPI_File_open(MPI_COMM_WORLD, "strided.txt",
                  MPI_MODE_CREATE | MPI_MODE_WRONLY,
                  MPI_INFO_NULL, &strided_fh);

    MPI_Offset stride = size * buf_len; // each rank interleaved by total size
    MPI_Offset start_offset = rank * buf_len;

    for (int i = 0; i < 5; ++i) { // write 5 strided blocks
        MPI_File_write_at(strided_fh, start_offset + i * stride,
                          buf, buf_len, MPI_CHAR, MPI_STATUS_IGNORE);
    }

    MPI_File_close(&strided_fh);

    // --------------------
    // 3) Random access
    // --------------------
    MPI_File random_fh;
    MPI_File_open(MPI_COMM_WORLD, "random.txt",
                  MPI_MODE_CREATE | MPI_MODE_WRONLY,
                  MPI_INFO_NULL, &random_fh);

    // Hardcoded random offsets per rank
    std::vector<MPI_Offset> random_offsets = {10 + rank*30, 5 + rank*50, 25 + rank*40};

    for (MPI_Offset off : random_offsets) {
        MPI_File_write_at(random_fh, off, buf, buf_len, MPI_CHAR, MPI_STATUS_IGNORE);
    }

    MPI_File_close(&random_fh);

    MPI_Finalize();
    return 0;
}
