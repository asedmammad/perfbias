#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define MAX_PATH 256
#define CPU_PATH "/sys/devices/system/cpu"

int is_valid_number(const char *str) {
    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

void set_perf_bias(int value) {
    DIR *dir;
    struct dirent *entry;
    char path[MAX_PATH];
    FILE *fp;

    dir = opendir(CPU_PATH);
    if (dir == NULL) {
        perror("Failed to open CPU directory");
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "cpu", 3) == 0 && isdigit(entry->d_name[3])) {
            snprintf(path, MAX_PATH, "%s/%s/power/energy_perf_bias", 
                    CPU_PATH, entry->d_name);
            
            fp = fopen(path, "w");
            if (fp == NULL) {
                fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
                continue;
            }
            
            fprintf(fp, "%d", value);
            fclose(fp);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Check if we have root privileges
    if (geteuid() != 0) {
        fprintf(stderr, "Error: This program needs root privileges\n");
        return 1;
    }

    // Check arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <value>\n", argv[0]);
        fprintf(stderr, "Value must be between 1 and 15\n");
        return 1;
    }

    // Validate input
    if (!is_valid_number(argv[1])) {
        fprintf(stderr, "Error: Please provide a valid number\n");
        return 1;
    }

    int value = atoi(argv[1]);
    if (value < 1 || value > 15) {
        fprintf(stderr, "Error: Value must be between 1 and 15\n");
        return 1;
    }

    set_perf_bias(value);
    printf("Successfully set performance bias to %d for all CPUs\n", value);
    return 0;
}
