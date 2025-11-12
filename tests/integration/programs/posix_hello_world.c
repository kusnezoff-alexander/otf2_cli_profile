#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char *filename = "working_dir/posix_hello_world.txt";
    const char *message = "Hello world";
    char buffer[100];
    ssize_t bytes_written, bytes_read;

    // open file for writing (create if not exists, truncate if exists)
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for write");
        exit(EXIT_FAILURE);
    }

    bytes_written = write(fd, message, strlen(message));
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    // open file for reading
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open for read");
        exit(EXIT_FAILURE);
    }

    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0'; // null-terminate

    printf("Read from file: %s\n", buffer);

    close(fd);
    return 0;
}
