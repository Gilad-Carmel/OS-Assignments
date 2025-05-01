#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 256

// R <start> <end> command
int read_data(FILE* file, int start, int end, FILE* output_file) {

    // check if start and end are within bounds (0 <= start <= end < file size)
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (start < 0 || end < start || end >= file_size) {
        fprintf(stderr, "Invalid read range: %d to %d\n", start, end);
        return -1;
    }
    fseek(file, start, SEEK_SET);
    char buffer[BUFFER_SIZE];
    size_t bytes_to_read = end - start + 1;
    if (bytes_to_read > BUFFER_SIZE) {
        bytes_to_read = BUFFER_SIZE;
    }
    size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
    if (bytes_read == 0) {
        perror("fread");
        return -1;
    }

    fwrite(buffer, 1, bytes_read, output_file);
    fprintf(output_file, "\n"); // add newline
    return 0;
}

// W <offset> <data> command
int write_data(FILE* file, int offset, const char* data) {
    // check if offset is within bounds (0 <= offset < file size)
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);

    if (offset < 0 || offset > file_size) {
        fprintf(stderr, "Invalid write offset: %d\n", offset);
        return -1;
    }
    fseek(file, offset, SEEK_SET);
    size_t data_length = strlen(data);
    // write that at offset and shift the rest of the file
    // move the rest of the file to make space for new data
    size_t buffer_size = file_size - offset;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("malloc");
        return -1;
    }
    fread(buffer, 1, buffer_size, file);
    fseek(file, offset, SEEK_SET);
    size_t bytes_written = fwrite(data, 1, data_length, file);
    if (bytes_written != data_length) {
        perror("fwrite");
        free(buffer);
        return -1;
    }
    // write the rest of the file after the new data
    if (buffer_size > 0) {
        fwrite(buffer, 1, buffer_size, file);
    }
    free(buffer);

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <data_file> <requests_file>\n", argv[0]);
        return 1;
    }
    const char* data_file_name = argv[1];
    const char* requests_file_name = argv[2];
    FILE* data_file = fopen(data_file_name, "r+");
    if (data_file == NULL) {
        perror("data.txt");
        return 1;
    }

    FILE* requests_file = fopen(requests_file_name, "r");
    if (requests_file == NULL) {
        perror("requests.txt");
        fclose(data_file);
        return 1;
    }

    // open with o_creat | o_trunc
    FILE* output_file = fopen("read_results.txt", "w+");
    if (output_file == NULL) {
        perror("read_results.txt");
        fclose(data_file);
        fclose(requests_file);
        return -1;
    }
    char command[2];
    int start, end, offset;
    char data[BUFFER_SIZE];
    
    // while there are still cpmmnands
    while (fscanf(requests_file, "%s", command) != EOF) {
        // command is either R, W or Q
        if (strcmp(command, "R") == 0) {
            // read the start and end from the requests file
            fscanf(requests_file, "%d %d", &start, &end);
            read_data(data_file, start, end, output_file);
        } else if (strcmp(command, "W") == 0) {
            // read the offset and data from the requests file
            fscanf(requests_file, "%d %s", &offset, data);
            write_data(data_file, offset, data);
        } else if (strcmp(command, "Q") == 0) {
            // quit the program
            break;
        } else {
            fprintf(stderr, "Unknown command: %s\n", command);
        }
    }

    fclose(data_file);
    fclose(requests_file);
    fclose(output_file);
    return 0;
}