#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_PATH 256
#define CPU_PATH "/sys/devices/system/cpu"

static int is_valid_number(const char *str)
{
    if (str == NULL || *str == '\0')
    {
        return 0;
    }

    size_t len = strlen(str);
    if (len > 2)
    { // No valid input needs more than 2 digits
        return 0;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (!isdigit((unsigned char)str[i]))
        {
            return 0;
        }
    }
    return 1;
}

static void drop_privileges(void)
{
    uid_t real_uid = getuid();
    if (setreuid(real_uid, real_uid) != 0)
    {
        perror("Failed to drop privileges");
        exit(1);
    }
}

static void set_perf_bias(int value)
{
    DIR *dir = NULL;
    struct dirent *entry;
    char path[MAX_PATH];
    int fd;
    char value_str[4];

    // Open CPU directory with minimal privileges
    if ((dir = opendir(CPU_PATH)) == NULL)
    {
        perror("Failed to open CPU directory");
        exit(1);
    }

    // Convert value to string once
    snprintf(value_str, sizeof(value_str), "%d", value);

    while ((entry = readdir(dir)) != NULL)
    {
        // Strict validation of CPU directory entry
        if (strncmp(entry->d_name, "cpu", 3) == 0 &&
            strlen(entry->d_name) > 3 &&
            strlen(entry->d_name) <= 6 && // cpu0 to cpu9999
            isdigit((unsigned char)entry->d_name[3]))
        {

            // Construct path safely
            if (snprintf(path, MAX_PATH, "%s/%s/power/energy_perf_bias",
                         CPU_PATH, entry->d_name) >= MAX_PATH)
            {
                fprintf(stderr, "Path too long for CPU %s\n", entry->d_name);
                continue;
            }

            // Open with specific flags
            fd = open(path, O_WRONLY | O_NOFOLLOW);
            if (fd == -1)
            {
                fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
                continue;
            }

            // Write the value
            if (write(fd, value_str, strlen(value_str)) == -1)
            {
                fprintf(stderr, "Failed to write to %s: %s\n", path, strerror(errno));
            }

            close(fd);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    // Store real uid before any privilege changes
    // uid_t real_uid = getuid();

    // Verify effective root privileges
    if (geteuid() != 0)
    {
        fprintf(stderr, "Error: This program needs root privileges\n");
        return 1;
    }

    // Basic argument checking
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <value>\n", argv[0]);
        fprintf(stderr, "Value must be between 1 and 15\n");
        return 1;
    }

    // Input validation
    if (!is_valid_number(argv[1]))
    {
        fprintf(stderr, "Error: Please provide a valid number\n");
        return 1;
    }

    // Safe conversion and range check
    char *endptr;
    long value = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0' || value < 1 || value > 15 || errno == ERANGE)
    {
        fprintf(stderr, "Error: Value must be between 1 and 15\n");
        return 1;
    }

    // Set the bias value
    set_perf_bias((int)value);

    // Drop privileges before printing (which might involve environment variables)
    drop_privileges();

    printf("Successfully set performance bias to %ld for all CPUs\n", value);
    return 0;
}