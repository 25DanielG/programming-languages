/**
 * @file fread.c
 * @author Daniel Gergov
 * @brief The following file contains a helper function to read a file
 * into memory. The code uses functions to open, read, and close
 * the file. The function returns a pointer to the memory
 * containing the file contents. The file then overlays the wave file structure
 * onto the memory. The program checks if the file is a valid wave file
 * and if the subformat is PCM. The program also checks if the fields
 * are correct and calculates any missing fields.
 * 
 * gcc -Wall filter.c
 * 
 * @date 2025-05-04
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
#define OUT_FILENAME "out.wav"
#define DEFAULT_FILTER (1)
#define NUM_FILTERS (3)
#define EXPECTED_ARGS (4)

#define FILTER1 (1)
#define DEFAULT_FILTER1 (40000)
#define FILTER2 (2)
#define FILTER3 (3)

struct MEM
    {
    char *pmem; // pointer to memory
    off_t *len; // length of the file
    };

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

void silentFail(const char *msg, const char *fname, const off_t *len);
off_t flength(int unit);
char* fload(char* fname, off_t *length);
struct ERR enforceWav(struct WAV *wav);
struct ERR enforceSubformat(struct WAV *wav);
void calculateFields(struct WAV *wav, off_t *length);
int validateWav(struct WAV *sound);
int saveWav(struct WAV *sound, off_t len, const char *fname);
void parseArgs(int argc, char *argv[], char **fname, int *filter, char **out, int *fargs);
void sampleRate(struct WAV *sound, int rate);
void reverseSound(struct WAV *sound);
void applyFilter(struct WAV *sound, int filter, int length, int arg);

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
    off_t pos;
    off_t len = (off_t)-1;

    pos = lseek(unit, (off_t)0, SEEK_CUR);

    if (pos != (off_t)-1)
        {
        len = lseek(unit, (off_t)0, SEEK_END);
        if (len != (off_t)-1)
            {
            if (lseek(unit, pos, SEEK_SET) == (off_t)-1)
                {
                silentFail("Error when resetting file position", NULL, NULL);
                }
            }
        else
            {
            silentFail("Error when seeking to end of file", NULL, NULL);
            }
        }
    else
        {
        silentFail("Error when getting current file position", NULL, NULL);
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
 * @param length a pointer to the length of the file
 * @return char* a pointer to memory containing the file contents
 * @postcondition the caller is responsible for freeing the memory
 */
char* fload(char* fname, off_t *length)
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
                    else if (length != NULL)
                        {
                        *length = len; // store the length in a pointer, used to calculate blank fields
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

/**
 * @brief The enforceWav function checks if the wav file is valid.
 * The function checks if the file is a valid wav file by checking
 * the chunkID, format, subchunk1ID, and subchunk2ID and their expected values.
 * The function returns an error code and a message if the file is not valid.
 * 
 * @param wav a pointer to a wav object
 * @return err a struct containing an error code and a message
 */
struct ERR enforceWav(struct WAV *wav)
    {
    struct ERR result;
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

/**
 * @brief The enforceSubformat function checks if the wav file is a valid PCM file.
 * The function checks if the audioFormat is PCM and if the subchunk1Size is 16.
 * The function returns an error code and a message if the file is not valid.
 * 
 * @param wav a pointer to a wav object
 * @return err a struct containing an error code and a message
 */
struct ERR enforceSubformat(struct WAV *wav)
    {
    struct ERR result;
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
 * @brief The calculateFields function calculates the missing fields
 * in the wav file. The function calculates the blockAlign, byteRate,
 * subchunk2Size, and chunkSize fields. The function checks if the
 * fields are correct and fixes them if they are 0. If the fields are not
 * 0 but not what the function expects, the function prints a warning message but
 * does not correct the values.
 * 
 * @param wav a pointer to a valid non-null wav object
 * @param length a pointer to an off_t object containing the length of the file
 * @precondition wav is a valid pointer to a wav object and length is a valid pointer to an off_t object containing the length of the file
 */
void calculateFields(struct WAV *wav, off_t *length)
    {
    WORD blockAlign;
    DWORD byteRate;
    DWORD header;
    DWORD subchunk2Size;
    DWORD chunkSize;

    blockAlign = wav->subchunk1.numChannels * wav->subchunk1.bitsPerSample / ((WORD)8);
    byteRate = wav->subchunk1.sampleRate * ((DWORD)blockAlign);
    header = ((DWORD)sizeof(struct INTRO)) + ((DWORD)sizeof(struct SBCHUNK1)) + ((DWORD)8);
    subchunk2Size = (DWORD)(*length) - header;
    chunkSize = ((DWORD)4) + ((DWORD)8 + wav->subchunk1.subchunk1Size) + ((DWORD)8 + subchunk2Size);

    if (wav->subchunk1.blockAlign != blockAlign)
        {
        printf("Warning: blockAlign is %hu but expected %hu.\n", wav->subchunk1.blockAlign, blockAlign);
        if (wav->subchunk1.blockAlign == 0)
            {
            printf("Fixing blockAlign...\n");
            wav->subchunk1.blockAlign = blockAlign;
            }
        }
    
    if (wav->subchunk1.byteRate != byteRate)
        {
        printf("Warning: byteRate is %u but expected %u.\n", wav->subchunk1.byteRate, byteRate);
        if (wav->subchunk1.byteRate == 0)
            {
            printf("Fixing byteRate...\n");
            wav->subchunk1.byteRate = byteRate;
            }
        }
    
    if (wav->subchunk2.subchunk2Size != subchunk2Size)
        {
        printf("Warning: subchunk2Size is %u but expected %u.\n", wav->subchunk2.subchunk2Size, subchunk2Size);
        if (wav->subchunk2.subchunk2Size == 0)
            {
            printf("Fixing subchunk2Size...\n");
            wav->subchunk2.subchunk2Size = subchunk2Size;
            }
        }
    
    if (wav->intro.chunkSize != chunkSize)
        {
        printf("Warning: chunkSize is %u but expected %u.\n", wav->intro.chunkSize, chunkSize);
        if (wav->intro.chunkSize == 0)
            {
            printf("Fixing chunkSize...\n");
            wav->intro.chunkSize = chunkSize;
            }
        }
    
    return;
    }

/**
 * @brief The validateWav function checks if the wav file is valid.
 * The function checks if the file is a valid wav file by enforcing
 * the wav format and the subformat, thus returning an appropriate boolean.
 * 
 * @param sound the wav object to check
 * @return int whether the wav file is valid or not
 */
int validateWav(struct WAV *sound)
    {
    struct ERR loaded, processed;
    int valid = FALSE;

    loaded = enforceWav(sound);
    if (loaded.err)
        {
        fprintf(stderr, "Error loading WAV file: %s\n", loaded.msg);
        }
    else
        {
        processed = enforceSubformat(sound);
        if (processed.err)
            {
            fprintf(stderr, "Error processing WAV file: %s\n", processed.msg);
            }
        else
            {
            valid = TRUE;
            }
        }
    
    return(valid);
    }

int saveWav(struct WAV *sound, off_t len, const char *fname)
    {
    int success = 0;
    int fd;
    ssize_t written;

    if (sound == NULL || fname == NULL || len <= 0)
        {
        fprintf(stderr, "Invalid arguments to saveWav\n");
        }
    else
        {
        fd = open(fname, O_WRONLY | O_CREAT | O_BINARY);
        if (fd == -1)
            {
            silentFail("Failed to open output file for writing", fname, &len);
            }
        else
            {
            written = write(fd, (char *)sound, len);
            if (written != len)
                {
                silentFail("Failed to write WAV data", fname, &len);
                }
            else
                {
                printf("Saved WAV file at %s (%lld bytes)\n", fname, (long long)len);
                success = 1;
                }
            close(fd);
            }
        }

    return(success);
    }

void parseArgs(int argc, char *argv[], char **fname, int *filter, char **out, int *fargs)
    {
    int fargc = 0;

    if (argc < EXPECTED_ARGS)
        {
        fprintf(stderr, "Usage: %s <filename> <out_filename> <filter> <filter_args>\n", argv[0]);
        printf("Proceeding with default arguments, file: %s, filter %d, out: %s\n", DEFAULT_FILENAME, DEFAULT_FILTER, OUT_FILENAME);
        *fname = DEFAULT_FILENAME;
        *out = OUT_FILENAME;
        *filter = DEFAULT_FILTER;
        }
    else
        {
        *fname = argv[1];
        *out = argv[2];
        *filter = atoi(argv[3]);
        if (*fname == NULL)
            {
            fprintf(stderr, "Invalid filename, proceeding with default filename: %s\n", DEFAULT_FILENAME);
            *fname = DEFAULT_FILENAME;
            }

        if (*out == NULL)
            {
            fprintf(stderr, "Invalid output filename, proceeding with default output filename: %s\n", OUT_FILENAME);
            *out = OUT_FILENAME;
            }

        if (*filter <= 0 || *filter > NUM_FILTERS)
            {
            fprintf(stderr, "Invalid filter, proceeding with default filter: %d\n", DEFAULT_FILTER);
            *filter = DEFAULT_FILTER;
            }
        }
    
    if (*filter == FILTER1) fargc = 1; // filter logic for which filters have arguments

    if (fargc == 0)
        {
        *fargs = 0;
        }
    else if (argc == EXPECTED_ARGS + fargc && argc > EXPECTED_ARGS)
        {
        *fargs = atoi(argv[4]);

        if (*fargs <= 0)
            {
            fprintf(stderr, "Invalid filter argument, proceeding with default filter argument: %d\n", DEFAULT_FILTER1);
            *fargs = DEFAULT_FILTER1;
            }
        }
    else
        {
        fprintf(stderr, "Invalid filter arguments for filter %d, proceeding with default filter argument: %d\n", *filter, DEFAULT_FILTER1);
        *fargs = DEFAULT_FILTER1;
        }

    printf("File: %s, filter: %d, out: %s\n", *fname, *filter, *out);    
    return;
    }

void sampleRate(struct WAV *sound, int rate)
    {
    DWORD byteRate;
    WORD blockAlign;

    sound->subchunk1.sampleRate = rate;
    blockAlign = sound->subchunk1.numChannels * sound->subchunk1.bitsPerSample / ((WORD)8);
    byteRate = sound->subchunk1.sampleRate * ((DWORD)blockAlign);

    sound->subchunk1.byteRate = byteRate;

    printf("Sample rate changed to %u\n", sound->subchunk1.sampleRate);

    return;
    }

void reverseSound(struct WAV *sound)
    {
    WORD bpsample, channels, j;
    DWORD sb2size, bsize, nBlocks, i;
    BYTE *data, *start, *end;
    BYTE tmp;

    bpsample = sound->subchunk1.bitsPerSample;
    channels = sound->subchunk1.numChannels;
    sb2size = sound->subchunk2.subchunk2Size;
    bsize = (bpsample / 8) * channels;

    if (bsize == 0)
        {
        fprintf(stderr, "block size is 0\n");
        }
    else
        {
        data = sound->subchunk2.data;
        nBlocks = sb2size / bsize;
    
        for (i = 0; i < nBlocks / 2; ++i)
            {
            start = data + i * bsize;
            end = data + (nBlocks - 1 - i) * bsize;
    
            for (j = 0; j < bsize; ++j)
                {
                tmp = start[j];
                start[j] = end[j];
                end[j] = tmp;
                }
            }
    
        printf("Reversed %lu blocks of audio\n", (unsigned long)nBlocks);
        }

    return;
    }

void applyFilter(struct WAV *sound, int filter, int length, int arg)
    {
    switch (filter)
        {
        case 1:
            sampleRate(sound, arg);
            break;
        
        case 2:
            reverseSound(sound);
            break;

        default:
            break;
        }

    saveWav(sound, length, OUT_FILENAME);
    return;
    }

/**
 * @brief the main function is the starting point of the program.
 * The function calls the fload function to load a file into memory.
 * The function checks if the file is a valid wav file and if the
 * subformat is PCM. The function also checks if the fields are correct
 * and calculates any missing fields.
 * 
 * @param argc the number of arguments
 * @param argv the array of arguments
 */
int main(int argc, char* argv[])
    {
    char *fname = NULL, *out = NULL;
    struct MEM fcontent;
    struct WAV *sound = NULL;
    int allocatedLength = FALSE, allocatedMem = FALSE, filter = 0, farg = 0;
    
    parseArgs(argc, argv, &fname, &filter, &out, &farg);

    fcontent.pmem = NULL;
    fcontent.len = (off_t *)malloc(sizeof(off_t));
    fcontent.pmem = fload(fname, fcontent.len);

    if (fcontent.len == NULL || *(fcontent.len) <= 0)
        {
        silentFail("Error retrieving file length, ", fname, NULL);
        }
    else
        {
        allocatedLength = TRUE;
        }

    if (fcontent.pmem == NULL)
        {
        silentFail("Failed to load file into memory", NULL, NULL);
        }
    else
        {
        printf("Loaded the file successfully\n");
        allocatedMem = TRUE;
        }
    
    sound = (struct WAV *)fcontent.pmem;

    if (validateWav(sound))
        {
        printf("WAV file is valid\n");
        calculateFields(sound, fcontent.len);
        applyFilter(sound, filter, *(fcontent.len), farg);
        }
    
    if (allocatedMem) free(fcontent.pmem);

    if (allocatedLength) free(fcontent.len);
    exit(0);
    }
