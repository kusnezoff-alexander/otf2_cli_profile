#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

// Hardcoded offsets for random.txt writes
const off_t random_offsets[5] = {1050, 350, 4096, 20000, 777};

int main() {
	// 0. Generate correct file paths
	const char* bats_dir = getenv("BATS_TEST_DIRNAME");

	const char* contiguous_file_name = "out/contiguous.txt";
	const char* strided_file_name = "out/strided.txt";
	const char* random_file_name = "out/random.txt";

    char* contiguous_file_full_path = NULL;
    if (asprintf(&contiguous_file_full_path, "%s/%s", bats_dir, contiguous_file_name) == -1) {
        perror("asprintf failed");
        return 1;
    }
    char* strided_file_full_path = NULL;
    if (asprintf(&strided_file_full_path, "%s/%s", bats_dir, strided_file_name) == -1) {
        perror("asprintf failed");
        return 1;
    }
    char* random_file_full_path = NULL;
    if (asprintf(&random_file_full_path, "%s/%s", bats_dir, random_file_name) == -1) {
        perror("asprintf failed");
        return 1;
    }

    // -----------------------------
    // File 1: contiguous.txt (append writes)
    // -----------------------------
    FILE* contiguous_file = fopen(contiguous_file_full_path, "a");
    if (!contiguous_file) {
		fprintf(stderr, "Error opening file: %s\n", contiguous_file_full_path);
        return 1;
    }

    for (int i = 0; i < 3; ++i) {
        fprintf(contiguous_file, "Append write #%d\n", i + 1);
    }

    fclose(contiguous_file);

    // -----------------------------
    // File 2: strided.txt (strided reads)
    // -----------------------------
    FILE* strided_file = fopen(strided_file_full_path, "r");
    if (!strided_file) {
		fprintf(stderr, "Error opening file: %s\n", strided_file_full_path);
        return 1;
    }

    char buffer[11];  // 10 bytes + null terminator
    buffer[10] = '\0';

    for (int i = 0; i < 3; ++i) {
        long offset = i * 256;
        if (fseek(strided_file, offset, SEEK_SET) != 0) {
            perror("fseek failed");
            fclose(strided_file);
            return 1;
        }

        size_t bytesRead = fread(buffer, 1, 10, strided_file);
        if (bytesRead < 10) {
            fprintf(stderr, "Warning: read less than 10 bytes at stride %d\n", i);
        }

        printf("Stride read %d: '%.*s'\n", i + 1, (int)bytesRead, buffer);
    }

    fclose(strided_file);

    // -----------------------------
    // File 3: random.txt (random writes)
    // -----------------------------

    // Sizes to write
    const char chars[] = {'A', 'B', 'C', 'D', 'E'};
    int random_file = open(random_file_full_path, O_WRONLY | O_CREAT, 0644);
    if (random_file < 0) {
		fprintf(stderr, "Error opening file: %s\n", random_file_full_path);
        return 1;
    }

    for (int i = 0; i < sizeof(chars); ++i) {
        off_t offset = random_offsets[i];
        if (lseek(random_file, offset, SEEK_SET) == (off_t)-1) {
            perror("lseek failed");
            close(random_file);
            return 1;
        }

        if (write(random_file, chars, 1) != 1) { // 1 char=1 byte
            perror("write failed");
            close(random_file);
            return 1;
        }

        printf("Random write %d: %d bytes at offset %lld\n", i + 1, 1, (long long)offset);
    }

    close(random_file);

    return 0;
}

