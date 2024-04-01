#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "ec_config.h"


int read_file_to_buffer(const char *filename, char *data, int size)
{
    FILE *fp;
    if ((fp = fopen(filename, "rb")) == NULL)
        ERROR_RETURN_VALUE("Fail open file");

    /* Read file to data */
    int size_fread;
    size_fread = fread(data, sizeof(char), size, fp);

    /* Padding data */
    if (size_fread < size)
    {
        for (int j = size_fread; j < size; j++)
            data[j] = '0';
    }
    fclose(fp);
    return EC_OK;
}

int open_write_file(const char *filename, const char *mode, char *data, int size)
{
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    if (fwrite(data, sizeof(char), (size_t)size, fp) != (size_t)size)
        ERROR_RETURN_VALUE("Fail write file");
    fclose(fp);
    return EC_OK;
}

int open_read_file(const char *filename, const char *mode, char *data, int size)
{
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    if (fread(data, sizeof(char), (size_t)size, fp) != (size_t)size)
        ERROR_RETURN_VALUE("Fail read file");
    fclose(fp);
    return EC_OK;
}

int open_write_file_mul(const char *filename, const char *mode, char **data, int size, int num)
{
    int i;
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    for (i = 0; i < num; i++)
        if (fwrite(data[i], sizeof(char), (size_t)size, fp) != (size_t)size)
            ERROR_RETURN_VALUE("Fail write file");
    fclose(fp);
    return EC_OK;
}

int clear_file(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    fclose(fp);
    return EC_OK;
}

int get_size_file(const char *filename)
{
    struct stat file_status;
    stat(filename, &file_status);
    return file_status.st_size;
}
