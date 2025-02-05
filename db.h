#ifndef DB_H_
#define DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    char* buffer;
    size_t bufferLen;
    size_t inputLen;
} InputBuffer;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct {
    StatementType type;
} Statement;

// constructor for an input buffer
InputBuffer* newInputBuffer();

// prints a prompt to the user
void printPrompt();

// reads user input into the inputBuffer
void readInput(InputBuffer* inputBuffer);

// free allocated memory for an inputBuffer
void closeInputBuffer(InputBuffer* inputBuffer);

// executes a meta command (meta commands start with a '.' character)
MetaCommandResult execMetaCommand(InputBuffer* inputBuffer);

// prepares the statement by identifying keywords and setting statement->type
PrepareResult prepareStatement(InputBuffer* InputBuffer, Statement* statement);

// identifies statement type and executes statement
void executeStatement(Statement* statement);

#endif // DB_H_