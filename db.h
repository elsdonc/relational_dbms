#ifndef DB_H_
#define DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

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
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
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
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

typedef struct {
    StatementType type;
    Row rowToInsert;
} Statement;

typedef struct {
    Table* table;
    uint32_t rowNum;
    bool endOfTable;
} Cursor;

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

// structure that will access page cache and the file
typedef struct {
    int fileDescriptor;
    uint32_t fileLength;
    void* pages[TABLE_MAX_PAGES];
} Pager;

// structure to represent in-memory database table
typedef struct {
    uint32_t numRows;
    Pager* pager;
} Table;

// constructor for an input buffer
InputBuffer* newInputBuffer();

// opening database file, initializing pager and table
Table* dbOpen(const char* filename);

// flush cache to disk, close database file, frees memory for Pager and Table
void dbClose(Table* table);

// opens database file, tracks its size, and initializes page caches to NULL
Pager* pagerOpen(const char* filename);

// flushes page cache to disk
void pagerFlush(Pager* pager, uint32_t pageNum, uint32_t size);

// handles cache miss
void* getPage(Pager* pager, uint32_t pageNum);

// create new cursors at start and end of table
Cursor* tableStart(Table* table);
Cursor* tableEnd(Table* table);

// prints a prompt to the user
void printPrompt();

// reads user input into the inputBuffer
void readInput(InputBuffer* inputBuffer);

// free allocated memory for an inputBuffer
void closeInputBuffer(InputBuffer* inputBuffer);

// executes a meta command (meta commands start with a '.' character)
MetaCommandResult execMetaCommand(InputBuffer* inputBuffer, Table* table);

// prepares the statement by identifying keywords and setting statement->type
PrepareResult prepareStatement(InputBuffer* InputBuffer, Statement* statement);

// check length of each string in statement to avoid buffer overflow
PrepareResult prepareInsert(InputBuffer* inputBuffer, Statement* statement);

// identifies statement type and executes statement
ExecuteResult executeStatement(Statement* statement, Table* table);

// Execute specific commands
ExecuteResult executeInsert(Statement* statement, Table* table);
ExecuteResult executeSelect(Statement* statement, Table* table);

// convert to compact representation of row
void serializeRow(Row* source, void* destination);

// convert from compact representation of row
void deserializeRow(void* source, Row* destination);

// returns a pointer to the position described by the cursor
void* cursorValue(Cursor* cursor);

// advance cursor to the next row
void cursorAdvance(Cursor* cursor);

// prints a row's data to standard output
void printRow(Row* row);

#endif // DB_H_