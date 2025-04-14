/*
 *
 * The wav.c program is a C program that defines the structure of a WAV file
 * and prints the size of each structure.
 *
 * Daniel Gergov
 * 4/14/25
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "atcs.h"

struct INTRO
    {
    char chunkID[4]; // has "RIFF"
    DWORD chunkSize;
    char format[4]; // has "WAVE"
    };

struct SBCHUNK1
    {
    char subchunk1ID[4]; // has "fmt "
    DWORD subchunk1Size;
    WORD audioFormat; // pcm = 1
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
    BYTE data[4]; // more data can extend the 4 bytes w/ malloc
    };

struct WAV
    {
    struct INTRO intro;
    struct SBCHUNK1 subchunk1;
    struct SBCHUNK2 subchunk2;
    };

int main(int argc, char* argv[])
    {
    printf("intro size: %lu bytes\n", sizeof(struct INTRO));
    printf("subchunk1 size: %lu bytes\n", sizeof(struct SBCHUNK1));
    printf("subchunk2 size: %lu bytes\n", sizeof(struct SBCHUNK2));
    printf("wav size: %lu bytes\n", sizeof(struct WAV));    

    exit(0);
    }
