#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define MAX_LINES 30000

typedef struct {
    char *content;
    int is_commented;
} Line;

int compareLines(const void *a, const void *b) {
    return strcmp(((Line *)a)->content, ((Line *)b)->content);
}

void freeLines(Line *lines, int lineCount) {
    for (int i = 0; i < lineCount; i++) {
        free(lines[i].content);
    }
    free(lines);
}

void processFile(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    Line *lines = malloc(MAX_LINES * sizeof(Line));
    if (!lines) {
        perror("Failed to allocate memory for lines");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    int lineCount = 0;
    while (lineCount < MAX_LINES) {
        lines[lineCount].content = malloc(MAX_LINE_LENGTH * sizeof(char));
        if (!lines[lineCount].content) {
            perror("Failed to allocate memory for line content");
            freeLines(lines, lineCount);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        if (fgets(lines[lineCount].content, MAX_LINE_LENGTH, file) == NULL) {
            break;
        }
        // Remove newline character at the end of the line
        size_t len = strlen(lines[lineCount].content);
        if (len > 0 && lines[lineCount].content[len - 1] == '\n') {
            lines[lineCount].content[len - 1] = '\0';
        }
        lineCount++;
    }
    fclose(file);

    qsort(lines, lineCount, sizeof(Line), compareLines);

    for (int i = 0; i < lineCount; i++) {
        if (i == 0 || strcmp(lines[i].content, lines[i - 1].content) != 0 ||
            (strlen(lines[i].content) == strlen(lines[i - 1].content) + 1 &&
             strncmp(lines[i].content, lines[i - 1].content, strlen(lines[i - 1].content)) == 0 &&
             lines[i].content[strlen(lines[i - 1].content)] == '#')) {
            lines[i].is_commented = 0;
        } else {
            lines[i].is_commented = 1;
        }
    }

    // Separate commented and uncommented lines
    Line *uncommented_lines = malloc(lineCount * sizeof(Line));
    Line *commented_lines = malloc(lineCount * sizeof(Line));
    if (!uncommented_lines || !commented_lines) {
        perror("Failed to allocate memory for separated lines");
        freeLines(lines, lineCount);
        free(uncommented_lines);
        free(commented_lines);
        exit(EXIT_FAILURE);
    }

    int uncommented_count = 0;
    int commented_count = 0;
    for (int i = 0; i < lineCount; i++) {
        if (lines[i].is_commented == 0) {
            uncommented_lines[uncommented_count++] = lines[i];
        } else {
            commented_lines[commented_count++] = lines[i];
        }
    }

    FILE *outputFile = fopen("processed_type.te", "w");
    if (!outputFile) {
        perror("Failed to create output file");
        freeLines(lines, lineCount);
        free(uncommented_lines);
        free(commented_lines);
        exit(EXIT_FAILURE);
    }

    // Write uncommented lines first
    for (int i = 0; i < uncommented_count; i++) {
        fprintf(outputFile, "%s\n", uncommented_lines[i].content);
    }

    // Write commented lines next
    for (int i = 0; i < commented_count; i++) {
        fprintf(outputFile, "#%s\n", commented_lines[i].content);
    }

    fclose(outputFile);
    freeLines(lines, lineCount);
    free(uncommented_lines);
    free(commented_lines);
}

int main() {
    char filename[] = "/home/mark/桌面/fox/fox/fox_12.1/bootable/recovery/sepolicy/type.te";
    processFile(filename);
    printf("Processing complete. Output saved in processed_type.te\n");
    return 0;
}



