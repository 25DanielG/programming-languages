/**
 * @file fread.c
 * @author Daniel Gergov
 * @brief The following file contains a helper function to read a file
 * into memory. The code uses functions to open, read, and close
 * the file. The function returns a pointer to the memory
 * containing the file contents.
 * 
 * gcc -Wall fread.c
 * 
 * @date 2025-04-22
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "atcs.h"

#define DEFAULT_FILENAME "test.txt"
#define EXPECTED_ARGS (2)

void silentFail(int* suc, const char *msg, const char *fname, const off_t *len);
off_t flength(int unit);
char* fload(char* fname);

/**
 * @brief The following function is used to fail silently. The
 * function prints a given error message and sets a success pointer
 * to false.
 * 
 * @param suc a pointer to a success variable
 * @param msg the error message to print
 * @param fname the name of the file that caused the error
 * @param len the length of the file that caused the error
 */
void silentFail(int* suc, const char *msg, const char *fname, const off_t *len)
    {
    *suc = FALSE;
    fprintf(stderr, "%s ", msg);
    if (fname != NULL)
        {
        fprintf(stderr, "file: %s ", fname);
        }
    if (len != NULL)
        {
        fprintf(stderr, "length: %lld ", (long long)*len);
        }
    fprintf(stderr, "\n");
    return;
    }

/**
 * @brief the flength function returns the length of a file
 * in bytes. The function uses the lseek function to get the
 * current position in the file, then it seeks to the end of
 * the file to get the length, and it seeks back
 * to the original position.
 * 
 * @precondition the unit parameter must be a valid file handle
 * @param unit the file handle of the file to get the length of
 * @return off_t the length of the file in bytes
 * @postcondition the function does not close the file handle
 */
off_t flength(int unit)
    {
    errno = 0;
    off_t pos = lseek(unit, (off_t)0, SEEK_CUR);
    off_t len = (off_t)-1;

    if (errno == 0)
        {
        len = lseek(unit, (off_t)0, SEEK_END);
        }
    if (errno == 0)
        {
        lseek(unit, pos, SEEK_SET);
        }
    if (errno != 0)
        {
        fprintf(stderr, "Error when getting file length\n");
        }
    return(len);
    }

/**
 * @brief the fload function loads a file into memory. The function
 * opens the file, gets the length of the file, allocates memory
 * for the file contents, reads the file into memory, and closes
 * the file. The function returns a pointer to the memory
 * containing the file contents.
 * @param fname the name of the file to open
 * @return char* a pointer to the buffer containing the file contents
 * @postcondition the caller is responsible for freeing the buffer
 */
char* fload(char* fname)
    {
    int suc = TRUE;
    int unit = -1;
    off_t len;
    char* buffer = NULL;
    ssize_t bytes;

    if (fname == NULL) silentFail(&suc, "Error: filename is null", NULL, NULL);
    
    if (suc)
        {
        unit = open(fname, O_RDONLY | O_BINARY);
        if (unit == -1) silentFail(&suc, "Error when opening file", fname, NULL);
        }
        
    if (suc)
        {
        len = flength(unit);
        if (len <= 0) silentFail(&suc, "Error when retrieving file length", fname, NULL);
        }
    
    if (suc)
        {
        buffer = (char*)malloc((size_t)len);
        if (buffer == NULL) silentFail(&suc, "Error at malloc for file buffer", fname, &len);
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

    return(buffer);
    }

/**
 * @brief the main function is the starting point of the program.
 * The function calls the fload function to load a file into memory.
 * @param argc the number of arguments
 * @param argv the array of arguments
 */
int main(int argc, char* argv[])
    {
    char* fname = NULL;
    char* fcontent = NULL;

    if (argc != EXPECTED_ARGS)
        {
        fname = DEFAULT_FILENAME;
        printf("No filename given in arguments, proceeding with <%s>\n", DEFAULT_FILENAME);
        }
    else
        {
        fname = argv[1];
        }

    fcontent = fload(fname);

    if (fcontent == NULL)
        {
        fprintf(stderr, "Failed to load file into memory\n");
        }
    else
        {
        printf("Loaded the file successfully\n");
        free(fcontent);
        }
    exit(0);
    }