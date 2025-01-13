#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

typedef struct {
    char **names;
    int count;
    int capacity;
} NameList;

NameList types, attributes;

void init_name_list(NameList *list) {
    list->count = 0;
    list->capacity = 100;
    list->names = (char **)malloc(list->capacity * sizeof(char *));
}

void add_to_name_list(NameList *list, const char *name, int debug) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->names = (char **)realloc(list->names, list->capacity * sizeof(char *));
    }
    list->names[list->count] = strdup(name);
    if (debug) {
        printf("strdup(%s)\n", name); // Debugging information
    }
    list->count++;
}

void free_name_list(NameList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->names[i]);
    }
    free(list->names);
}

int name_exists(NameList *list, const char *name) {
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

typedef struct {
    char *type_name;
    int *lines;
    int line_count;
    int line_capacity;
} TypeError;

void init_type_error(TypeError *error, const char *type_name) {
    error->type_name = strdup(type_name);
    error->line_count = 0;
    error->line_capacity = 10;
    error->lines = (int *)malloc(error->line_capacity * sizeof(int));
}

void add_line_to_type_error(TypeError *error, int line_number) {
    if (error->line_count >= error->line_capacity) {
        error->line_capacity *= 2;
        error->lines = (int *)realloc(error->lines, error->line_capacity * sizeof(int));
    }
    error->lines[error->line_count++] = line_number;
}

void print_type_error(TypeError *error) {
    printf("Error: Type '%s' not found as type or attribute on line", error->type_name);
    for (int i = 0; i < error->line_count; i++) {
        printf(" %d", error->lines[i]);
    }
    printf("\n");
}

void free_type_error(TypeError *error) {
    free(error->type_name);
    free(error->lines);
}

typedef struct {
    TypeError **errors;
    int count;
    int capacity;
} ErrorList;

void init_error_list(ErrorList *list) {
    list->count = 0;
    list->capacity = 10;
    list->errors = (TypeError **)malloc(list->capacity * sizeof(TypeError *));
}

void add_error_to_list(ErrorList *list, TypeError *error) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->errors = (TypeError **)realloc(list->errors, list->capacity * sizeof(TypeError *));
    }
    list->errors[list->count++] = error;
}

void free_error_list(ErrorList *list) {
    for (int i = 0; i < list->count; i++) {
        free_type_error(list->errors[i]);
    }
    free(list->errors);
}

TypeError *find_error_by_type(ErrorList *list, const char *type_name) {
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->errors[i]->type_name, type_name) == 0) {
            return list->errors[i];
        }
    }
    return NULL;
}

void read_declarations(FILE *file, NameList *types, NameList *attributes, int debug) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        char *stripped_line = line;
        stripped_line[strcspn(stripped_line, "\n")] = '\0'; // Remove newline character

        if (*stripped_line == '#' || *stripped_line == '\0') {
            continue; // Ignore comments and empty lines
        } else if (strncmp(stripped_line, "attribute ", 10) == 0) {
            char *name = stripped_line + 10;
            char *end = strchr(name, ';');
            if (end) {
                *end = '\0';
                trim_whitespace(name); // Trim any leading/trailing whitespace
                add_to_name_list(attributes, name, debug);
                if (debug) {
                    printf("Line %d: Added attribute: %s\n", line_number, name);
                }
            }
        } else if (strncmp(stripped_line, "type ", 5) == 0) {
            char *name = stripped_line + 5;
            char *end = strchr(name, ',');
            if (!end) {
                end = strchr(name, ';');
            }
            if (end) {
                *end = '\0';
                trim_whitespace(name); // Trim any leading/trailing whitespace
                add_to_name_list(types, name, debug);
                if (debug) {
                    printf("Line %d: Added type: %s\n", line_number, name);
                }
            }
        }
    }
}

void check_rule(const char *rule, int start_line_number, ErrorList *errors) {
    char buffer[MAX_LINE_LENGTH];
    strcpy(buffer, rule);

    char *start = buffer;
    if (strncmp(start, "allow ", 6) == 0) {
        start += 6;
    } else if (strncmp(start, "type_transition ", 16) == 0) { // Corrected offset to 16
        start += 16;
    } else {
        return; // Not a valid rule to check
    }

    skip_whitespace(&start);

    // Extract source type
    char *source_end = strchr(start, ' ');
    if (!source_end) {
        return; // Invalid rule format
    }
    *source_end = '\0';
    char *source_type = start;
    trim_whitespace(source_type);
    if (!name_exists(&types, source_type) && !name_exists(&attributes, source_type)) {
        add_unknown_type_error(errors, source_type, start_line_number);
    }

    start = source_end + 1;
    skip_whitespace(&start);

    // Extract target type before ':'
    char *target_colon = strchr(start, ':');
    if (!target_colon) {
        return; // Invalid rule format
    }
    *target_colon = '\0';
    char *target_type = start;
    trim_whitespace(target_type);
    if (!name_exists(&types, target_type) && !name_exists(&attributes, target_type)) {
        add_unknown_type_error(errors, target_type, start_line_number);
    }

    start = target_colon + 1;
    skip_whitespace(&start);

    // Skip everything inside {}
    if (*start == '{') {
        int in_braces = 1;
        start++;
        while (*start && in_braces > 0) {
            if (*start == '{') {
                in_braces++;
            } else if (*start == '}') {
                in_braces--;
            }
            start++;
        }
    }

    // Continue parsing after the block
    skip_whitespace(&start);
    if (*start != ';' && *start != '\0') {
        // Handle additional parts of the rule if needed
    }
}

void skip_whitespace(char **str) {
    while (**str == ' ' || **str == '\t') {
        (*str)++;
    }
}

void parse_file(FILE *file, ErrorList *errors, int debug) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    char multi_line_buffer[MAX_LINE_LENGTH] = "";
    int multi_line_start = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        char *stripped_line = line;
        stripped_line[strcspn(stripped_line, "\n")] = '\0'; // Remove newline character

        if (*stripped_line == '#' || *stripped_line == '\0') {
            continue; // Ignore comments and empty lines
        }

        if (multi_line_start) {
            strcat(multi_line_buffer, " ");
            strcat(multi_line_buffer, stripped_line);

            if (strchr(stripped_line, '}')) {
                multi_line_start = 0;
                check_rule(multi_line_buffer, line_number, errors);
                multi_line_buffer[0] = '\0';
            }
        } else {
            if (strchr(stripped_line, '{')) {
                strcpy(multi_line_buffer, stripped_line);
                multi_line_start = 1;
            } else {
                check_rule(stripped_line, line_number, errors);
            }
        }

        // Debugging information
        if (debug) {
            printf("Line %d: %s\n", line_number, stripped_line);
        }
    }

    if (multi_line_start) {
        printf("Error: Incomplete rule starting on line %d\n", line_number);
    }
}

void trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while(*str == ' ' || *str == '\t') str++;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\t')) end--;
    *(end+1) = '\0';
}

void add_unknown_type_error(ErrorList *errors, const char *type_name, int line_number) {
    TypeError *error = find_error_by_type(errors, type_name);
    if (!error) {
        error = (TypeError *)malloc(sizeof(TypeError));
        init_type_error(error, type_name);
        add_error_to_list(errors, error);
    }
    add_line_to_type_error(error, line_number);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-t] <input.te> [optional.conf]\n", argv[0]);
        return 1;
    }

    int debug = 0;
    FILE *te_file = NULL;
    FILE *conf_file = NULL;
    char *te_filename = NULL;
    char *conf_filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            debug = 1;
        } else if (te_filename == NULL) {
            te_filename = argv[i];
        } else if (conf_filename == NULL) {
            conf_filename = argv[i];
        } else {
            fprintf(stderr, "Too many arguments provided.\n");
            fprintf(stderr, "Usage: %s [-t] <input.te> [optional.conf]\n", argv[0]);
            return 1;
        }
    }

    if (te_filename == NULL) {
        fprintf(stderr, "Input TE file must be specified.\n");
        fprintf(stderr, "Usage: %s [-t] <input.te> [optional.conf]\n", argv[0]);
        return 1;
    }

    te_file = fopen(te_filename, "r");
    if (!te_file) {
        perror("Failed to open input TE file");
        return 1;
    }

    init_name_list(&types);
    init_name_list(&attributes);

    if (conf_filename) {
        conf_file = fopen(conf_filename, "r");
        if (!conf_file) {
            perror("Failed to open optional configuration file");
            fclose(te_file);
            free_name_list(&types);
            free_name_list(&attributes);
            return 1;
        }
        read_declarations(conf_file, &types, &attributes, debug);
        fclose(conf_file);
    }

    read_declarations(te_file, &types, &attributes, debug);
    rewind(te_file);

    ErrorList errors;
    init_error_list(&errors);

    parse_file(te_file, &errors, debug);

    for (int i = 0; i < errors.count; i++) {
        print_type_error(errors.errors[i]);
    }

    fclose(te_file);

    free_name_list(&types);
    free_name_list(&attributes);
    free_error_list(&errors);

    return 0;
}



