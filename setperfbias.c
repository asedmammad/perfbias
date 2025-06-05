#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_PATH 256
#define CPU_PATH "/sys/devices/system/cpu"
#define PERF_BIAS_MIN 1
#define PERF_BIAS_MAX 15

typedef enum
{
    SET_PERFBIAS_SUCCESS = 0,
    SET_PERFBIAS_INVALID_VALUE,
    SET_PERFBIAS_NO_ROOT,
    SET_PERFBIAS_INVALID_ARG,
    SET_PERFBIAS_OPEN_FAILED,
    SET_PERFBIAS_WRITE_FAILED,
    SET_PERFBIAS_LOGGED_ERROR
} SetPerfBiasStatus;

/**
 * Checks if a string is a valid number.
 * @param str - The string to check.
 * @return 1 if valid, 0 otherwise.
 */
static int is_valid_number(const char *str)
{
    if (str == NULL || *str == '\0')
        return 0;

    size_t len = strlen(str);
    if (len > 2)
        return 0;

    for (size_t i = 0; i < len; i++)
    {
        if (!isdigit((unsigned char)str[i]))
            return 0;
    }
    return 1;
}

/**
 * Logs an error message with the specified status.
 * @param message - The message to log.
 * @param status - The error status.
 */
static void log_error(const char *message, SetPerfBiasStatus status)
{
    switch (status)
    {
    case SET_PERFBIAS_OPEN_FAILED:
        fprintf(stderr, "Failed to open %s: %s\n", message, strerror(errno));
        break;
    case SET_PERFBIAS_WRITE_FAILED:
        fprintf(stderr, "Failed to write to %s: %s\n", message, strerror(errno));
        break;
    default:
        perror(message);
    }
}

/**
 * Drops root privileges by setting the effective user ID to the real user ID.
 */
static void drop_privileges(void)
{
    uid_t real_uid = getuid();
    if (setreuid(real_uid, real_uid) != 0)
    {
        log_error("Failed to drop privileges", SET_PERFBIAS_LOGGED_ERROR);
        exit(1);
    }
}

/**
 * Checks if the system supports energy performance bias hints.
 * @return 1 if supported, 0 otherwise.
 */
static int check_energy_perf_bias_support(void)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_PATH];

    dir = opendir(CPU_PATH);
    if (!dir)
        return 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "cpu", 3) == 0 &&
            strlen(entry->d_name) > 3 &&
            strlen(entry->d_name) <= 6 &&
            isdigit((unsigned char)entry->d_name[3]))
        {

            snprintf(path, MAX_PATH, "%s/%s/power/energy_perf_bias",
                     CPU_PATH, entry->d_name);

            if (access(path, F_OK) == 0)
            {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

/**
 * Sets the performance bias for all CPUs.
 * @param value - The performance bias value.
 * @return SET_PERFBIAS_SUCCESS on success, error code otherwise.
 */
static int set_perf_bias(int value)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_PATH];
    int fd;

    dir = opendir(CPU_PATH);
    if (!dir)
        return SET_PERFBIAS_OPEN_FAILED;

    char value_str[4];
    snprintf(value_str, sizeof(value_str), "%d", value);

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "cpu", 3) == 0 &&
            strlen(entry->d_name) > 3 &&
            strlen(entry->d_name) <= 6 &&
            isdigit((unsigned char)entry->d_name[3]))
        {

            snprintf(path, MAX_PATH, "%s/%s/power/energy_perf_bias",
                     CPU_PATH, entry->d_name);

            fd = open(path, O_WRONLY | O_NOFOLLOW);
            if (fd == -1)
            {
                log_error(path, SET_PERFBIAS_OPEN_FAILED);
                continue;
            }

            if (write(fd, value_str, strlen(value_str)) == -1)
            {
                log_error(path, SET_PERFBIAS_WRITE_FAILED);
            }
            close(fd);
        }
    }

    closedir(dir);
    return SET_PERFBIAS_SUCCESS;
}

/**
 * Main function to set performance bias.
 * @param argc - Number of command-line arguments.
 * @param argv - Array of command-line arguments.
 * @return Exit status code.
 */
int main(int argc, char *argv[])
{
    if (geteuid() != 0)
    {
        fprintf(stderr, "Error: This program needs root privileges\n");
        return SET_PERFBIAS_NO_ROOT;
    }

    if (!check_energy_perf_bias_support())
    {
        fprintf(stderr, "Error: System does not support energy_perf_bias hints.\n");
        return SET_PERFBIAS_LOGGED_ERROR;
    }

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <value>\n", argv[0]);
        fprintf(stderr, "Value must be between %d and %d\n", PERF_BIAS_MIN, PERF_BIAS_MAX);
        return SET_PERFBIAS_INVALID_ARG;
    }

    if (!is_valid_number(argv[1]))
    {
        fprintf(stderr, "Error: Please provide a valid number\n");
        return SET_PERFBIAS_INVALID_VALUE;
    }

    char *endptr;
    long value = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0' || value < PERF_BIAS_MIN || value > PERF_BIAS_MAX || errno == ERANGE)
    {
        fprintf(stderr, "Error: Value must be between %d and %d\n", PERF_BIAS_MIN, PERF_BIAS_MAX);
        return SET_PERFBIAS_INVALID_VALUE;
    }

    if (set_perf_bias((int)value) != SET_PERFBIAS_SUCCESS)
    {
        log_error("Failed to set performance bias", SET_PERFBIAS_LOGGED_ERROR);
        return SET_PERFBIAS_LOGGED_ERROR;
    }

    drop_privileges();
    printf("Successfully set performance bias to %ld for all CPUs\n", value);

    return 0;
}