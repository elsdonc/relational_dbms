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
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY
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
    uint32_t numPages;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    Pager* pager;
    uint32_t rootPageNum;
} Table;

typedef struct {
    Table* table;
    uint32_t pageNum;
    uint32_t cellNum;
    bool endOfTable;
} Cursor;

typedef enum {
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

// Node Header Layout
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// Leaf Node Header Layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

// Leaf Node Body Layout
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

// Internal Node Header Layout
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

// Internal Node Body Layout
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

// constructor for an input buffer
InputBuffer* newInputBuffer();

// opening database file, initializing pager and table
Table* dbOpen(const char* filename);

// flush cache to disk, close database file, frees memory for Pager and Table
void dbClose(Table* table);

// opens database file, tracks its size, and initializes page caches to NULL
Pager* pagerOpen(const char* filename);

// flushes page cache to disk
void pagerFlush(Pager* pager, uint32_t pageNum);

// handles cache miss
void* getPage(Pager* pager, uint32_t pageNum);

// allocate new pages
uint32_t getUnusedPageNum(Pager* pager);

// create new cursors at start of table
Cursor* tableStart(Table* table);

// search tree for a key
Cursor* tableFind(Table* table, uint32_t key);

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

// access keys, values, and metadata
uint32_t* leafNodeNumCells(void* node);
void* leafNodeCell(void* node, uint32_t cellNum);
uint32_t* leafNodeKey(void* node, uint32_t cellNum);
void* leafNodeValue(void* node, uint32_t cellNum);

// initializing nodes
void initializeLeafNode(void* node);
void initializeInternalNode(void* node);

// insert key/value pair into a leaf node
void leafNodeInsert(Cursor* cursor, uint32_t key, Row* value);

// binary search for a node
Cursor* leafNodeFind(Table* table, uint32_t pageNum, uint32_t key);
Cursor* internalNodeFind(Table* table, uint32_t pageNum, uint32_t key);

// get and set node type
NodeType getNodeType(void* node);
void setNodeType(void* node, NodeType type);

// split full leaf node in half, allocate a new leaf node, and update or create new parent
void leafNodeSplitAndInsert(Cursor* cursor, uint32_t key, Row* value);

// create new root node
void createNewRoot(Table* table, uint32_t rightChildPageNum);

// reading and writing to an internal node
uint32_t* internalNodeNumKeys(void* node);
uint32_t* internalNodeRightChild(void* node);
uint32_t* internalNodeCell(void* node, uint32_t cellNum);
uint32_t* internalNodeChild(void* node, uint32_t childNum);
uint32_t* internalNodeKey(void* node, uint32_t keyNum);
uint32_t getNodeMaxKey(void* node);

// getters and setters for root
bool isNodeRoot(void* node);
void setNodeRoot(void* node, bool is_root);

// visualize btree
void printTree(Pager* pager, uint32_t page_num, uint32_t indentation_level);
void indent(uint32_t level);

#endif // DB_H_