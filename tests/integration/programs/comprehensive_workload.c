#include <fcntl.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MB (1024 * 1024)
#define KB 1024

void create_contiguous_file(const char* filename, int rank, int size) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s_contiguous.bin", filename);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return;

    char* buffer = (char*)malloc(10 * MB);
    memset(buffer, 'A' + (rank % 26), 10 * MB);

    for (int i = 0; i < 5; i++) {
        write(fd, buffer, 10 * MB);
    }

    free(buffer);
    close(fd);
}

void create_strided_file(const char* filename, int rank, int size) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s_strided.bin", filename);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return;

    char* buffer = (char*)malloc(MB);
    memset(buffer, 'S' + (rank % 26), MB);

    for (int i = 0; i < 50; i++) {
        lseek(fd, i * 10 * MB, SEEK_SET);
        write(fd, buffer, MB);
    }

    free(buffer);
    close(fd);
}

void create_random_file(const char* filename, int rank, int size) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s_random.bin", filename);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return;

    char* buffer = (char*)malloc(64 * KB);
    for (int i = 0; i < 64 * KB; i++) {
        buffer[i] = 'R' + (rank + i) % 26;
    }

    srand(rank * 12345);
    for (int i = 0; i < 100; i++) {
        off_t pos = (rand() % 500) * MB;
        lseek(fd, pos, SEEK_SET);
        write(fd, buffer, 64 * KB);
    }

    free(buffer);
    close(fd);
}

void write_posix_files(const char* dirname, int rank, int size) {
    char filename[512];

    snprintf(filename, sizeof(filename), "%s/posix_rank%d", dirname, rank);
    FILE* f = fopen(filename, "w");
    if (f) {
        for (int i = 0; i < 1000; i++) {
            fprintf(f, "POSIX I/O: Rank %d line %d\n", rank, i);
        }
        fflush(f);
        fclose(f);
    }
}

void write_stdout_stderr(const char* dirname, int rank) {
    char filename[512];

    snprintf(filename, sizeof(filename), "%s/stdout_rank%d.txt", dirname, rank);
    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "Output from rank %d\n", rank);
        for (int i = 0; i < 10; i++) {
            fprintf(f, "Log line %d: Processing data...\n", i);
        }
        fclose(f);
    }

    snprintf(filename, sizeof(filename), "%s/stderr_rank%d.txt", dirname, rank);
    f = fopen(filename, "w");
    if (f) {
        fprintf(f, "Error log from rank %d\n", rank);
        fclose(f);
    }
}

void mpi_io_collective(const char* filename, int rank, int size) {
    MPI_File   fh;
    MPI_Status status;

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s_mpi_collective.bin", filename);

    MPI_File_open(MPI_COMM_WORLD, filepath, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

    char* buffer = (char*)malloc(MB);
    memset(buffer, 'M', MB);

    MPI_Offset offset = rank * 10 * MB;
    MPI_File_write_at(fh, offset, buffer, MB, MPI_CHAR, &status);

    MPI_File_close(&fh);
    free(buffer);
}

void mpi_io_individual(const char* filename, int rank, int size) {
    MPI_File   fh;
    MPI_Status status;

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s_mpi_individual.bin", filename);

    MPI_File_open(MPI_COMM_SELF, filepath, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

    char* buffer = (char*)malloc(512 * KB);
    memset(buffer, 'I', 512 * KB);

    for (int i = 0; i < 10; i++) {
        MPI_File_write(fh, buffer, 512 * KB, MPI_CHAR, &status);
    }

    MPI_File_close(&fh);
    free(buffer);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const char* base_dir = "/tmp/otf2_workload";
    if (rank == 0) {
        system("mkdir -p /tmp/otf2_workload");
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Running comprehensive I/O workload with %d ranks...\n", size);
    }

    write_posix_files(base_dir, rank, size);

    char filename[512];
    snprintf(filename, sizeof(filename), "%s/file", base_dir);
    create_contiguous_file(filename, rank, size);
    create_strided_file(filename, rank, size);
    create_random_file(filename, rank, size);

    mpi_io_collective(filename, rank, size);
    mpi_io_individual(filename, rank, size);

    write_stdout_stderr(base_dir, rank);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Workload completed.\n");
    }

    MPI_Finalize();
    return 0;
}
