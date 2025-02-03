#ifndef DB_H_
#define DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char* buffer;
    size_t bufferLen;
    size_t inputLen;
} InputBuffer;

// constructor for an input buffer
InputBuffer* newInputBuffer();

// prints a prompt to the user
void printPrompt();

// reads user input into the inputBuffer
void readInput(InputBuffer* inputBuffer);

// free allocated memory for an inputBuffer
void closeInputBuffer(InputBuffer* inputBuffer);

#endif // DB_H_