/**
 * @file filter.c
 * @author Daniel Gergov
 * @brief The following file contains a helper function to read a file
 * into memory. The code uses functions to open, read, and close
 * the file. The function returns a pointer to the memory
 * containing the file contents. The file then overlays the wave file structure
 * onto the memory. The program checks if the file is a valid wave file
 * and if the subformat is PCM. The program also checks if the fields
 * are correct and calculates any missing fields.
 * 
 * After verifying the wav file, the program applies a given filter to the file
 * and saves the wav file to a given file name. The code has
 * 3 filters:
 * 0. Print important header information
 * 1. Modify the sample rate
 * 2. Reverse the sound
 * 3. Create 8D audio
 * 
 * gcc -Wall filter.c
 * 
 * @date 2025-05-12
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <math.h>

#include "atcs.h"

#define DEFAULT_FILENAME "test.txt"
#define OUT_FILENAME "out.wav"
#define DEFAULT_FILTER (1)
#define NUM_FILTERS (3)
#define EXPECTED_ARGS (4)

#define ARG0 (0)
#define ARG1 (1)
#define ARG2 (2)
#define ARG3 (3)
#define ARG4 (4)

#define FIRST (0)

#define FILTER0 (0)
#define FILTER1 (1)
#define DEFAULT_FILTER1 ((double)40000.0)
#define FILTER2 (2)
#define FILTER3 (3)
#define DEFAULT_FILTER3 ((double)0.15)

#define BITS_PER_BYTE (8)
#define WAV_STRING_BYTES (4)
#define TWO_CHANNELS (2)

#define ONE_BYTE (1)
#define TWO_BYTES (2)
#define THREE_BYTES (3)
#define FOUR_BYTES (4)

#define MSBYTE_24BITS (2)
#define MIDBYTE_24BITS (1)
#define LSBYTE_24BITS (0)

#define FOUR_BITS (4)
#define EIGHT_BITS (8)
#define TWELVE_BITS (12)
#define SIXTEEN_BITS (16)
#define TWENTY_FOUR_BITS (24)
#define THIRTY_TWO_BITS (32)

#define SIGN_MASK_24BITS (0x80)
#define SIGN_EXTEND_24BITS (0xFF000000)
#define NO_SIGN_EXTEND_24BITS (0x00000000)
#define LOW_BYTE_MASK (0xFF)
#define HIGH_12BYTE_MASK (0xFFF0)

#define UINT8_MIDPOINT (128)

#define PI (3.14159265)

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
void parseArgs(int argc, char *argv[], char **fname, int *filter, char **out, double **fargs, int *num_fargs);
void sampleRate(struct WAV *sound, int rate);
void reverseSound(struct WAV *sound);
int32_t readSample(BYTE *data, int index, int bpsample);
void writeSample(int32_t left, int32_t right, BYTE *data, int index, DWORD frameSize, WORD bpsample);
void audio8D(struct WAV *sound, double rps, off_t *length);
void printFilterUsage();
void applyFilter(struct WAV *sound, int filter, char *out, off_t *length, double *fargs, int num_fargs);

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
    result.err = 1;
    result.msg = NULL;

    if (wav == NULL)
        {
        result.msg = "Wav object is null";
        }
    else if (strncmp(wav->intro.chunkID, "RIFF", 4) != 0)
        {
        result.msg = "Wav intro chunkID is not 'RIFF'";
        }
    else if (strncmp(wav->intro.format, "WAVE", 4) != 0)
        {
        result.msg = "Wav intro format is not 'WAVE'";
        }
    else if (strncmp(wav->subchunk1.subchunk1ID, "fmt ", 4) != 0)
        {
        result.msg = "Wav subchunk1ID is not 'fmt '";
        }
    else if (strncmp(wav->subchunk2.subchunk2ID, "data", 4) != 0)
        {
        result.msg = "Wav subchunk2ID is not 'data'";
        }
    else
        {
        result.err = 0;
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
    result.err = 1;
    result.msg = NULL;

    if (wav == NULL)
        {
        result.msg = "Wav object is null";
        }
    else if (wav->subchunk1.audioFormat != 1)
        {
        result.msg = "Wav audioFormat is not PCM";
        }
    else if (wav->subchunk1.subchunk1Size != 16)
        {
        result.msg = "Wav subchunk1Size is not 16";
        }
    else
        {
        result.err = 0;
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

    blockAlign = wav->subchunk1.numChannels * wav->subchunk1.bitsPerSample / ((WORD)BITS_PER_BYTE);
    byteRate = wav->subchunk1.sampleRate * ((DWORD)blockAlign);
    header = ((DWORD)sizeof(struct INTRO)) + ((DWORD)sizeof(struct SBCHUNK1)) + ((DWORD)BITS_PER_BYTE);
    subchunk2Size = (DWORD)(*length) - header;
    chunkSize = ((DWORD)WAV_STRING_BYTES) + ((DWORD)BITS_PER_BYTE + wav->subchunk1.subchunk1Size) + ((DWORD)BITS_PER_BYTE + subchunk2Size);

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

/**
 * @brief The saveWav function saves the wav file. The function
 * opens the file for writing, writes the wav data to the file,
 * and closes the file. The function returns 1 if the file was saved
 * successfully and 0 if there was an error.
 * 
 * @param sound the wav object to save
 * @param len the length of the wav object
 * @param fname the name of the file to save
 * @return int 1 if the file was saved successfully, 0 if there was an error
 */
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
        fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
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

/**
 * @brief The parseArgs function parses the command line arguments.
 * The function checks if the arguments are valid and sets the
 * default values if they are not. The function sets the filename,
 * output filename, filter, and filter arguments.
 * 
 * @param argc the number of arguments
 * @param argv the array of arguments
 * @param fname the name of the file to open
 * @param filter the filter to apply
 * @param out the name of the output file
 * @param fargs the filter arguments
 * @param num_args the number of filter arguments
 */
void parseArgs(int argc, char *argv[], char **fname, int *filter, char **out, double **fargs, int *num_fargs)
    {
    int i;

    if (argc < EXPECTED_ARGS)
        {
        printFilterUsage();
        printf("Proceeding with default arguments, file: %s, filter %d, out: %s\n", DEFAULT_FILENAME, DEFAULT_FILTER, OUT_FILENAME);
        *fname = DEFAULT_FILENAME;
        *out = OUT_FILENAME;
        *filter = DEFAULT_FILTER;
        *fargs = NULL;
        *num_fargs = 0;
        }
    else
        {
        *fname = argv[ARG1];
        *out = argv[ARG2];
        *filter = atoi(argv[ARG3]);
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

        if (*filter < 0 || *filter > NUM_FILTERS)
            {
            fprintf(stderr, "Invalid filter, proceeding with default filter: %d\n", DEFAULT_FILTER);
            *filter = DEFAULT_FILTER;
            }

        *num_fargs = argc - EXPECTED_ARGS;
        }

    if (*num_fargs > 0)
        {
        *fargs = (double *)malloc(sizeof(double) * (*num_fargs));
        for (i = 0; i < *num_fargs; ++i)
            {
            (*fargs)[i] = atof(argv[EXPECTED_ARGS + i]);
            if ((*fargs)[i] <= 0)
                {
                fprintf(stderr, "Invalid filter argument #%d, defaulting to 0\n", i + 1);
                (*fargs)[i] = 0.0;
                }
            }
        }
    else
        {
        *fargs = NULL;
        }

    printf("File: %s, filter: %d, out: %s, num filter args: %d\n", *fname, *filter, *out, *num_fargs);    
    return;
    }

/**
 * @brief The printHeader function prints the header of the wav file.
 * It prints useful information about the file such as the number of channels,
 * the sample rate, the byte rate, and the bits per sample.
 * 
 * @param sound the sound to print the header of
 */
void printHeader(struct WAV *sound)
    {
    printf("WAV file header:\n");
    printf("Num channels: %hu\n", sound->subchunk1.numChannels);
    printf("Sample rate: %u\n", sound->subchunk1.sampleRate);
    printf("Byte rate: %u\n", sound->subchunk1.byteRate);
    printf("Bits per sample: %hu\n", sound->subchunk1.bitsPerSample);
    
    return;
    }

/**
 * @brief The sampleRate function changes the sample rate of the wav file.
 * The function recalculates the byteRate and blockAlign based on
 * the new sample rate.
 * 
 * @param sound the wav object to change the sample rate of
 * @param rate the new sample rate
 * @precondition sound is a valid pointer to a wav object
 */
void sampleRate(struct WAV *sound, int rate)
    {
    DWORD byteRate;
    WORD blockAlign;

    sound->subchunk1.sampleRate = rate;
    blockAlign = sound->subchunk1.numChannels * sound->subchunk1.bitsPerSample / ((WORD)BITS_PER_BYTE);
    byteRate = sound->subchunk1.sampleRate * ((DWORD)blockAlign);

    sound->subchunk1.byteRate = byteRate;

    printf("Sample rate changed to %u\n", sound->subchunk1.sampleRate);

    return;
    }

/**
 * @brief The reverseSound function reverses the sound in the wav file.
 * The function swaps the samples of the wav file, accounting for the
 * number of channels (and the bits per sample).
 * 
 * @param sound the wav object to reverse
 * @precondition sound is a valid pointer to a wav object
 */
void reverseSound(struct WAV *sound)
    {
    WORD bpsample, channels, j;
    DWORD sb2size, bsize, nBlocks, i;
    BYTE *data, *start, *end;
    BYTE tmp;

    bpsample = sound->subchunk1.bitsPerSample;
    channels = sound->subchunk1.numChannels;
    sb2size = sound->subchunk2.subchunk2Size;
    bsize = (bpsample / BITS_PER_BYTE) * channels;

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
    
        printf("Reversed %lu blocks of sound\n", (unsigned long)nBlocks);
        }

    return;
    }

/**
 * @brief The readSample function reads a sample from the wav file.
 * The function reads the sample based on the bits per sample
 * and returns the sample as an int32_t. It handles
 * 8, 12, 16, 24, and 32 bit samples.
 * 
 * @param data the data to read from
 * @param index the index to read from
 * @param bpsample the bits per sample
 * @return int32_t the sample read from the data
 * @precondition data is a valid pointer to a byte array
 */
int32_t readSample(BYTE *data, int index, int bpsample)
    {
    int32_t sample = 0;
    uint8_t tmp8;
    uint16_t tmp12 = 0;
    int16_t tmp16;
    uint8_t tmp24[THREE_BYTES];
    int32_t tmp32;

    if (bpsample == EIGHT_BITS)
        {
        memcpy(&tmp8, &data[index], ONE_BYTE);
        sample = (int32_t)((int16_t)tmp8 - UINT8_MIDPOINT);
        }
    else if (bpsample == TWELVE_BITS)
        {
        memcpy(&tmp12, &data[index], TWO_BYTES);
        sample = (int32_t)((int16_t)(tmp12 << FOUR_BITS)); // sign-extend the 12-bit value
        sample >>= FOUR_BITS;                              // restore the value
        }
    else if (bpsample == SIXTEEN_BITS)
        {
        memcpy(&tmp16, &data[index], TWO_BYTES);
        sample = (int32_t)tmp16;
        }
    else if (bpsample == TWENTY_FOUR_BITS)
        {
        memcpy(tmp24, &data[index], THREE_BYTES);
        sample = (tmp24[MSBYTE_24BITS] & SIGN_MASK_24BITS) ? SIGN_EXTEND_24BITS : NO_SIGN_EXTEND_24BITS;
        sample |= (tmp24[MSBYTE_24BITS] << SIXTEEN_BITS) | (tmp24[MIDBYTE_24BITS] << EIGHT_BITS) | tmp24[LSBYTE_24BITS];
        }
    else if (bpsample == THIRTY_TWO_BITS)
        {
        memcpy(&tmp32, &data[index], FOUR_BYTES);
        sample = tmp32;
        }
    
    return(sample);
    }

/**
 * @brief The writeSample function writes a sample to the wav file.
 * The function writes the sample based on the bits per sample
 * and handles 8, 12, 16, 24, and 32 bit samples. The function takes
 * the left and right samples representing 2 channels
 * and writes them to the data array.
 * 
 * @param left the left sample to write
 * @param right the right sample to write
 * @param data the data to write to
 * @param index the index to write to
 * @param frameSize the size of the frame
 * @param bpsample the bits per sample
 * @precondition data is a valid pointer to a byte array
 */
void writeSample(int32_t left, int32_t right, BYTE *data, int index, DWORD frameSize, WORD bpsample)
    {
    BYTE l8, r8;
    int16_t l16, r16;
    uint16_t l12, r12;
    DWORD b;

    if (bpsample == EIGHT_BITS)
        {
        l8 = (BYTE)(left + UINT8_MIDPOINT); // 8bit pcm is unsigned, so we shift lbyte and rbyte up by 128
        r8 = (BYTE)(right + UINT8_MIDPOINT);
        data[frameSize * index] = l8;
        data[frameSize * index + 1U] = r8;
        }
    else if (bpsample == TWELVE_BITS)
        {
        l12 = (uint16_t)((left << FOUR_BITS) & HIGH_12BYTE_MASK);
        r12 = (uint16_t)((right << FOUR_BITS) & HIGH_12BYTE_MASK);
        memcpy(&data[frameSize * index], &l12, TWO_BYTES);
        memcpy(&data[frameSize * index + TWO_BYTES], &r12, TWO_BYTES);
        }
    else if (bpsample == SIXTEEN_BITS)
        {
        l16 = (left > INT16_MAX) ? INT16_MAX : (left < INT16_MIN ? INT16_MIN : left); // clamp to 16bit signed range
        r16 = (right > INT16_MAX) ? INT16_MAX : (right < INT16_MIN ? INT16_MIN : right);
        memcpy(&data[frameSize * index], &l16, sizeof(int16_t));
        memcpy(&data[frameSize * index + sizeof(int16_t)], &r16, sizeof(int16_t));
        }
    else if (bpsample == TWENTY_FOUR_BITS)
        {
        for (b = 0U; b < THREE_BYTES; ++b)
            {
            data[(frameSize * index) + b] = (BYTE)(left >> (EIGHT_BITS * b)) & LOW_BYTE_MASK; // manually write the 3 bytes into each channel
            data[(frameSize * index) + THREE_BYTES + b] = (BYTE)(right >> (EIGHT_BITS * b)) & LOW_BYTE_MASK;
            }
        }
    else if (bpsample == THIRTY_TWO_BITS)
        {
        memcpy(&data[frameSize * index], &left, FOUR_BYTES);
        memcpy(&data[frameSize * index + FOUR_BYTES], &right, FOUR_BYTES);
        }

    return;
    }

/**
 * @brief The audio8D function creates 8D audio from the wav file.
 * The function takes the wav file and applies a rotation per second
 * to the sound. The function creates stereo sound and supports
 * 8, 12, 16, 24, and 32 bit sound. The function modifies the passed
 * sound wav object and returns the length of the sound
 * (through the passed parameter).
 * 
 * @param sound the wav object to modify
 * @param rps the rotations per second
 * @param length the length of the wav object
 * @precondition sound is a valid pointer to a wav object
 */
void audio8D(struct WAV *sound, double rps, off_t *length)
    {
    WORD nchannels, bpsample, c;
    DWORD sampleRate, dsize, bytepsample, frameSize, nframes, i, stereoFrameSize;
    BYTE *data, *modified;
    size_t modSize;
    double t, angle, lpan, rpan, mono, left, right;
    int32_t sval, lval, rval;
    int index;

    if (sound == NULL)
        {
        fprintf(stderr, "rotations per second is invalid\n");
        }
    else
        {
        nchannels = sound->subchunk1.numChannels;
        bpsample = sound->subchunk1.bitsPerSample;
        sampleRate = sound->subchunk1.sampleRate;
        dsize = sound->subchunk2.subchunk2Size;
        data = sound->subchunk2.data;

        if (bpsample == EIGHT_BITS || bpsample != TWELVE_BITS || bpsample != SIXTEEN_BITS || bpsample != TWENTY_FOUR_BITS
                                   || bpsample != THIRTY_TWO_BITS)
            {
            bytepsample = (DWORD)bpsample / BITS_PER_BYTE;
            frameSize = (DWORD)(nchannels) * bytepsample;
            nframes = dsize / frameSize;

            stereoFrameSize = TWO_CHANNELS * bytepsample; // 2 channels in stereo
            modSize = (size_t)(nframes * stereoFrameSize);
            modified = (BYTE *)malloc(modSize);

            if (modified == NULL)
                {
                fprintf(stderr, "Failed malloc for stereo output\n");
                }
            else
                {
                for (i = 0U; i < nframes; ++i)
                    {
                    t = (double)i / (double)sampleRate;   // time (seconds)
                    angle = 2.0 * PI * rps * t;
                    lpan = sin(angle);
                    rpan = cos(angle);

                    mono = 0.0;
                    for (c = 0U; c < nchannels; ++c)
                        {
                        index = (i * frameSize) + ((DWORD)c * bytepsample);
                        sval = readSample(data, index, bpsample);
                        mono += (double)sval;
                        }
                    
                    mono /= (double)nchannels;            // average to mono

                    left = mono * (1.0 - lpan);
                    right = mono * (1.0 + rpan);

                    lval = (int32_t)left;
                    rval = (int32_t)right;

                    writeSample(lval, rval, modified, i, stereoFrameSize, bpsample);
                    }

                memcpy(&(sound->subchunk2.data[0]), modified, modSize);
                free(modified);
                sound->subchunk1.numChannels = TWO_CHANNELS;
                sound->subchunk1.blockAlign = sound->subchunk1.numChannels * sound->subchunk1.bitsPerSample / ((WORD)BITS_PER_BYTE);
                sound->subchunk1.byteRate = sampleRate * sound->subchunk1.blockAlign;
                sound->subchunk2.subchunk2Size = nframes * sound->subchunk1.numChannels * bytepsample;
                sound->intro.chunkSize = ((DWORD)WAV_STRING_BYTES) + ((DWORD)BITS_PER_BYTE + sound->subchunk1.subchunk1Size) + ((DWORD)BITS_PER_BYTE + sound->subchunk2.subchunk2Size);

                if (length != NULL)
                    {
                    *length = sizeof(struct INTRO) + sizeof(struct SBCHUNK1) + EIGHT_BITS + sound->subchunk2.subchunk2Size;
                    }

                printf("Created 8D audio at %.2f rotations/sec\n", rps);
                }
            }
        else
            {
            fprintf(stderr, "8d audio only supports 8,12,16,24,32-bit sound\n");
            }
        }

    return;
    }

/**
 * @brief The printFilterUsage function prints the usage of the filters.
 * The function prints the usage of the filters and their exepcted # of arguments.
 */
void printFilterUsage()
    {
    printf("Usage: ./<code> <in_filename> <out_filename> <filter> [<filter_arg1> <filter_arg2> ...]\n");
    printf("Filters:\n");
    printf("0: Print header, # of args: 0\n");
    printf("1: Change sample rate, # of args: 1\n");
    printf("2: Reverse sound, # of args: 0 \n");
    printf("3: Create 8D audio, # of args: 1\n");

    return;
    }

/**
 * @brief The applyFilter function applies the filter to the wav file.
 * The function applies the filter based on the filter number by calling
 * the respective filter's function. The function then saves the wav file.
 * 
 * @param sound the wav object to apply the filter to
 * @param filter the filter to apply
 * @param out the name of the output file
 * @param length the length of the wav object
 * @param arg the argument for the filter
 * @precondition sound is a valid pointer to a wav object
 */
void applyFilter(struct WAV *sound, int filter, char *out, off_t *length, double *fargs, int num_fargs)
    {
    int expectFargs = 0;
    expectFargs = (filter == FILTER1 || filter == FILTER3) ? 1 : 0;
    if (num_fargs != expectFargs)
        {
        printFilterUsage();
        fprintf(stderr, "Invalid number of filter arguments for filter %d, expected %d, got %d\n", filter, expectFargs, num_fargs);
        }
    else
        {
        switch (filter)
            {
            case FILTER0:
                printHeader(sound);
                break;

            case FILTER1:
                sampleRate(sound, (int)fargs[FIRST]);
                break;
            
            case FILTER2:
                reverseSound(sound);
                break;

            case FILTER3:
                audio8D(sound, fargs[FIRST], length);
                break;

            default:
                break;
            }
        
        saveWav(sound, *length, out);
        }

    return;
    }

/**
 * @brief the main function is the starting point of the program.
 * The function calls the fload function to load a file into memory.
 * The function checks if the file is a valid wav file and if the
 * subformat is PCM. The function also checks if the fields are correct
 * and calculates any missing fields. The function then applies
 * the filter to the wav file and saves the wav file.
 * 
 * @param argc the number of arguments
 * @param argv the array of arguments
 */
int main(int argc, char* argv[])
    {
    char *fname = NULL, *out = NULL;
    struct MEM fcontent;
    struct WAV *sound = NULL;
    int allocatedLength = FALSE, allocatedMem = FALSE, filter = 0, num_fargs = 0;
    double *fargs = NULL;
    
    parseArgs(argc, argv, &fname, &filter, &out, &fargs, &num_fargs);

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
        applyFilter(sound, filter, out, fcontent.len, fargs, num_fargs);
        }
    
    if (allocatedMem) free(fcontent.pmem);
    if (fargs != NULL) free(fargs);
    if (allocatedLength) free(fcontent.len);
    exit(0);
    }
