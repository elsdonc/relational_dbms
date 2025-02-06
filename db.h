#ifndef DB_H_
#define DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

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
    PREPARE_UNRECOGNIZED,
    PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_SYTAX_ERROR,
    EXECUTE_TABLE_FULL
} ExecuteResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
    StatementType type;
    Row rowToInsert;
} Statement;

// sizes and offsets for row storage
#define sizeOfAttribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
const uint32_t ID_SIZE = sizeOfAttribute(Row, id);
const uint32_t USERNAME_SIZE = sizeOfAttribute(Row, username);
const uint32_t EMAIL_SIZE = sizeOfAttribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET  = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

// table storage parameters
const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

// structure to represent in-memory database table
typedef struct {
    uint32_t numRows;
    void* pages[TABLE_MAX_PAGES];
} Table;

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
ExecuteResult executeStatement(Statement* statement, Table* table);

// Execute specific commands
ExecuteResult executeInsert(Statement* statement, Table* table);
ExecuteResult executeSelect(Statement* statement, Table* table);

// convert to compact representation of row
void serializeRow(Row* source, void* destination);

// convert from compact representation of row
void deserializeRow(void* source, Row* destination);

// figure out where to read/write in memory for a row
void* rowSlot(Table* table, uint32_t rowNum);

// prints a row's data to standard output
void printRow(Row* row);

#endif // DB_H_