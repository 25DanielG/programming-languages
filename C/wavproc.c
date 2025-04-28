/**
 * @file fread.c
 * @author Daniel Gergov
 * @brief The following file contains a helper function to read a file
 * into memory. The code uses functions to open, read, and close
 * the file. The function returns a pointer to the memory
 * containing the file contents.
 * 
 * gcc -Wall wavproc.c
 * 
 * @date 2025-04-25
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include "atcs.h"

#define DEFAULT_FILENAME "test.txt"
#define EXPECTED_ARGS (2)

struct ERR
    {
    int err; // 0 or 1
    char* msg;
    };

struct INTRO
    {
    char chunkID[4]; // has "RIFF"
    DWORD chunkSize;
    char format[4];  // has "WAVE"
    };

struct SBCHUNK1
    {
    char subchunk1ID[4]; // has "fmt "
    DWORD subchunk1Size;
    WORD audioFormat;    // pcm = 1
    WORD numChannels;
    DWORD sampleRate;
    DWORD byteRate;
    WORD blockAlign;
    WORD bitsPerSample;
    };

struct SBCHUNK2
    {
    char subchunk2ID[4]; // has "data"
    DWORD subchunk2Size;
    BYTE data[4];        // more data can extend the 4 bytes w/ malloc
    };

struct WAV
    {
    struct INTRO intro;
    struct SBCHUNK1 subchunk1;
    struct SBCHUNK2 subchunk2;
    };

typedef struct WAV wav;
typedef struct ERR err;

void silentFail(const char *msg, const char *fname, const off_t *len);
off_t flength(int unit);
char* fload(char* fname);

/**
 * @brief The following function is used to fail silently. The
 * function prints a given error message.
 * 
 * @param msg the error message to print
 * @param fname the name of the file that caused the error
 * @param len the length of the file that caused the error
 */
void silentFail(const char *msg, const char *fname, const off_t *len)
    {
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
    off_t pos = lseek(unit, (off_t)0, SEEK_CUR);
    off_t len = (off_t)-1;

    if (pos != (off_t)-1)
        {
        len = lseek(unit, (off_t)0, SEEK_END);
        if (len != (off_t)-1)
            {
            if (lseek(unit, pos, SEEK_SET) == (off_t)-1)
                {
                fprintf(stderr, "Error when resetting file position\n");
                }
            }
        else
            {
            fprintf(stderr, "Error when seeking to end of file\n");
            }
        }
    else
        {
        fprintf(stderr, "Error when getting current file position\n");
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
 * @return char* a pointer to memory containing the file contents
 * @postcondition the caller is responsible for freeing the memory
 */
char* fload(char* fname)
    {
    int unit = -1;
    off_t len;
    char* pmem = NULL;
    ssize_t bytes;

    if (fname != NULL)
        {
        unit = open(fname, O_RDONLY | O_BINARY);
        if (unit != -1)
            {
            len = flength(unit);
            if (len > 0)
                {
                    pmem = (char*)malloc((size_t)len);
                if (pmem != NULL)
                    {
                    bytes = read(unit, pmem, len);
                    if ((off_t)bytes != len)
                        {
                        fprintf(stderr, "Error during file reading: %s, with length %lld bytes\n", fname, (off_t)len);
                        free(pmem);
                        pmem = NULL;
                        }
                    }
                else
                    {
                    silentFail("Error at malloc for file size", fname, &len);
                    }
                }
            else
                {
                silentFail("Error when retrieving file length", fname, NULL);
                }
            close(unit);
            }
        else
            {
            silentFail("Error when opening file", fname, NULL);
            }
        }
    else
        {
        silentFail("Error: filename is null", NULL, NULL);
        }

    return(pmem);
    }

err enforceWav(wav* wav)
    {
    err result;
    result.err = 0;
    result.msg = NULL;

    if (wav == NULL)
        {
        result.err = 1;
        result.msg = "Wav object is null";
        }
    
    if (!result.err && strncmp(wav->intro.chunkID, "RIFF", 4) != 0)
        {
        result.err = 1;
        result.msg = "Wav intro chunkID is not 'RIFF'";
        }
    
    if (!result.err && strncmp(wav->intro.format, "WAVE", 4) != 0)
        {
        result.err = 1;
        result.msg = "Wav intro format is not 'WAVE'";
        }
    
    if (!result.err && strncmp(wav->subchunk1.subchunk1ID, "fmt ", 4) != 0)
        {
        result.err = 1;
        result.msg = "Wav subchunk1ID is not 'fmt '";
        }
    
    if (!result.err && strncmp(wav->subchunk2.subchunk2ID, "data", 4) != 0)
        {
        result.err = 1;
        result.msg = "Wav subchunk2ID is not 'data'";
        }
    
    return(result);
    }

err enforceSubformat(wav* wav)
    {
    err result;
    result.err = 0;
    result.msg = NULL;

    if (wav == NULL)
        {
        result.err = 1;
        result.msg = "Wav object is null";
        }
    
    if (!result.err && wav->subchunk1.audioFormat != 1)
        {
        result.err = 1;
        result.msg = "Wav audioFormat is not PCM";
        }
    
    if (!result.err && wav->subchunk1.subchunk1Size != 16)
        {
        result.err = 1;
        result.msg = "Wav subchunk1Size is not 16";
        }
    
    return(result);
    }

/**
 * @brief the main function is the starting point of the program.
 * The function calls the fload function to load a file into memory.
 * @param argc the number of arguments
 * @param argv the array of arguments
 */
int main(int argc, char* argv[])
    {
    char *fname = NULL;
    char *fcontent = NULL;
    wav *sound = NULL;
    err loaded, processed;
    int proceed = FALSE;

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
        }
    
    sound = (wav *)fcontent;
    loaded = enforceWav(sound);
    if (loaded.err)
        {
        fprintf(stderr, "Error loading WAV file: %s\n", loaded.msg);
        free(fcontent);
        fcontent = NULL;
        }
    else
        {
        processed = enforceSubformat(sound);
        if (processed.err)
            {
            fprintf(stderr, "Error processing WAV file: %s\n", processed.msg);
            free(fcontent);
            fcontent = NULL;
            }
        else
            {
            proceed = TRUE;
            }
        }
    
    if (proceed)
        {
        printf("WAV file is valid\n");
        }
    
    exit(0);
    }
