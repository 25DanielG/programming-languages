#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "atcs.h"

#define DEFAULT_FILENAME "test.txt"

off_t flength(int unit);
char* fload(char* fname);

/**
 * @brief 
 * 
 * @precondition the unit parameter must be a valid file handle
 * @param unit the file handle of the file to get the length of
 * @return off_t the length of the file in bytes
 * @postcondition the function does not close the file handle
 */
off_t flength(int unit)
    {
    errno = 0;
    off_t pos = lseek(unit, 0, SEEK_CUR);
    off_t len = -1;

    if (errno == 0)
        {
        len = lseek(unit, 0, SEEK_END);
        }
    if (errno == 0)
        {
        lseek(unit, pos, SEEK_SET);
        }
    if (errno != 0)
        {
        fprintf(stderr, "Error when getting file length\n");
        }
    return len;
    }

char* fload(char* fname)
    {
    int suc = 1;
    int unit = -1;
    off_t len;
    char* buffer = NULL;
    ssize_t bytes;

    if (fname == NULL)
        {
        fprintf(stderr, "Error: filename is null\n");
        suc = 0;
        }
    
    if (suc)
        {
        unit = open(fname, O_RDONLY | O_BINARY);
        if (unit == -1)
            {
            suc = 0;
            fprintf(stderr, "Error when opening file: %s\n", fname);
            }
        }
        
    if (suc)
        {
        len = flength(unit);
        if (len <= 0)
            {
            suc = 0;
            fprintf(stderr, "Error when retrieving file length: %s\n", fname);
            }
        }
    
    if (suc)
        {
        buffer = (char*)malloc((size_t)len);
        if (buffer == NULL)
            {
            suc = 0;
            fprintf(stderr, "Error at malloc for file buffer: %s, with length %lld bytes\n", fname, (off_t)len);
            }
        }
    
    if (suc)
        {
        bytes = read(unit, buffer, len);
        if ((off_t)bytes != len)
            {
            fprintf(stderr, "Error during file reading: %s, with length %lld bytes\n", fname, (off_t)len);
            free(buffer);
            buffer = NULL;
            }
        }
    
    if (unit != -1)
        {
        close(unit);
        }
    return buffer;
    }

int main(int argc, char* argv[])
    {
    char* fname = NULL;
    if (argc != 2)
        {
        fname = DEFAULT_FILENAME;
        printf("No filename given in arguments, proceeding with <%s>\n", DEFAULT_FILENAME);
        }
    else
        {
        fname = argv[1];
        }

    char* fcontent = fload(fname);

    if (fcontent == NULL)
        {
        fprintf(stderr, "Failed to load file into memory.\n");
        }
    else
        {
        printf("Loaded the file successfully.\n");
        free(fcontent);
        }
    exit(0);
    }