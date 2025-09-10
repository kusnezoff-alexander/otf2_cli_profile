#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

// Hardcoded offsets for random.txt writes
const off_t random_offsets[5] = {1050, 350, 4096, 20000, 777};

int main() {
    // -----------------------------
    // File 1: contiguous.txt (append writes)
    // -----------------------------
    FILE* file1 = fopen("working_dir/contiguous.txt", "a");
    if (!file1) {
        perror("Error opening contiguous.txt");
        return 1;
    }

    for (int i = 0; i < 3; ++i) {
        fprintf(file1, "Append write #%d\n", i + 1);
    }

    fclose(file1);

    // -----------------------------
    // File 2: strided.txt (strided reads)
    // -----------------------------
    FILE* file2 = fopen("working_dir/strided.txt", "r");
    if (!file2) {
        perror("Error opening strided.txt");
        return 1;
    }

    char buffer[11];  // 10 bytes + null terminator
    buffer[10] = '\0';

    for (int i = 0; i < 3; ++i) {
        long offset = i * 256;
        if (fseek(file2, offset, SEEK_SET) != 0) {
            perror("fseek failed");
            fclose(file2);
            return 1;
        }

        size_t bytesRead = fread(buffer, 1, 10, file2);
        if (bytesRead < 10) {
            fprintf(stderr, "Warning: read less than 10 bytes at stride %d\n", i);
        }

        printf("Stride read %d: '%.*s'\n", i + 1, (int)bytesRead, buffer);
    }

    fclose(file2);

    // -----------------------------
    // File 3: random.txt (random writes)
    // -----------------------------

    // Sizes to write
    const char chars[] = {'A', 'B', 'C', 'D', 'E'};
    int file3 = open("working_dir/random.txt", O_WRONLY | O_CREAT, 0644);
    if (file3 < 0) {
        perror("Error opening random.txt");
        return 1;
    }

    for (int i = 0; i < sizeof(chars); ++i) {
        off_t offset = random_offsets[i];
        if (lseek(file3, offset, SEEK_SET) == (off_t)-1) {
            perror("lseek failed");
            close(file3);
            return 1;
        }

        if (write(file3, chars, 1) != 1) { // 1 char=1 byte
            perror("write failed");
            close(file3);
            return 1;
        }

        printf("Random write %d: %d bytes at offset %lld\n", i + 1, 1, (long long)offset);
    }

    close(file3);

    return 0;
}

