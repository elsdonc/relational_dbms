#include "db.h"

InputBuffer* newInputBuffer() {
    InputBuffer* inputBuffer = (InputBuffer*) malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->bufferLen = 0;
    inputBuffer->inputLen = 0;
    return inputBuffer;
}

Table* dbOpen(const char* filename) {
    Pager* pager = pagerOpen(filename);

    Table* table = (Table*) malloc(sizeof(Table));
    table->pager = pager;
    table->rootPageNum = 0;

    if(pager->numPages == 0) {
        // new db file. Initialize page 0 as leaf node.
        void* rootNode = getPage(pager, 0);
        initializeLeafNode(rootNode);
    }

    return table;
}

void dbClose(Table* table) {
    Pager* pager = table->pager;

    for (uint32_t i = 0; i < pager->numPages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }
        pagerFlush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    if (close(pager->fileDescriptor) == -1) {
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}

Pager* pagerOpen(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, 0200 | 0400);

    if (fd == -1) {
        printf("Error opening file\n");
        exit(EXIT_FAILURE);
    }

    off_t fileLength = lseek(fd, 0, SEEK_END);

    Pager* pager = malloc(sizeof(Pager));
    pager->fileDescriptor = fd;
    pager->fileLength = fileLength;
    pager->numPages = (fileLength / PAGE_SIZE);

    if (fileLength % PAGE_SIZE != 0) {
        printf("DB file is corrupt. It isn't a whole number of pages.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

void* getPage(Pager* pager, uint32_t pageNum) {
    if (pageNum > TABLE_MAX_PAGES) {
        printf("Page number out of bounds. %d > %d\n", pageNum, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[pageNum] == NULL) {
        // cache miss. allocate memory, load from file
        void* page = malloc(PAGE_SIZE);
        uint32_t numPages = pager->fileLength / PAGE_SIZE;

        // handle partial page at end of file
        if (pager->fileLength % PAGE_SIZE) {
            numPages += 1;
        }

        if (pageNum <= numPages) {
            lseek(pager->fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);
            ssize_t bytesRead = read(pager->fileDescriptor, page, PAGE_SIZE);
            if (bytesRead == -1) {
                perror("Error reading file\n");
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[pageNum] = page;

        if (pageNum >= pager->numPages) {
            pager->numPages = pageNum + 1;
        }
    }

    return pager->pages[pageNum];
}

void pagerFlush(Pager* pager, uint32_t pageNum) {
    if (pager->pages[pageNum] == NULL) {
        printf("Error flushing null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->fileDescriptor, pageNum * PAGE_SIZE, SEEK_SET);

    if (offset == -1) {
        perror("Seeking\n");
        exit(EXIT_FAILURE);
    }

    ssize_t bytesWritten = write(pager->fileDescriptor, pager->pages[pageNum], PAGE_SIZE);

    if (bytesWritten == -1) {
        perror("Error writing file\n");
        exit(EXIT_FAILURE);
    }
}

Cursor* tableStart(Table* table) {
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->pageNum = table->rootPageNum;
    cursor->cellNum = 0;

    void* rootNode = getPage(table->pager, table->rootPageNum);
    uint32_t numCells = *leafNodeNumCells(rootNode);
    cursor->endOfTable = (numCells == 0);

    return cursor;
}

Cursor* tableFind(Table* table, uint32_t key) {
    uint32_t rootPageNum = table->rootPageNum;
    void* rootNode = getPage(table->pager, rootPageNum);

    if (getNodeType(rootNode) == NODE_LEAF) {
        return leafNodeFind(table, rootPageNum, key);
    } else {
        printf("will implement searching an internal node here\n");
        exit(EXIT_FAILURE);
    }
}

Cursor* leafNodeFind(Table* table, uint32_t pageNum, uint32_t key) {
    void* node = getPage(table->pager, pageNum);
    uint32_t numCells = *leafNodeNumCells(node);

    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->pageNum = pageNum;

    // binary search
    uint32_t l = 0;
    uint32_t r = numCells;
    while (l < r) {
        uint32_t mid = (l + r) / 2;
        uint32_t keyAtMid = *leafNodeKey(node, mid);
        if (key == keyAtMid) {
            cursor->cellNum = mid;
            return cursor;
        } else if (key < keyAtMid) {
            r = mid;
        } else {
            l = mid + 1;
        }
    }

    cursor->cellNum = l;
    return cursor;
}

NodeType getNodeType(void* node) {
    uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
    return (NodeType)value;
}

void setNodeType(void* node, NodeType type) {
    uint8_t value = type;
    *((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}

void printPrompt() {
    printf("db > ");
}

void readInput(InputBuffer* inputBuffer) {
    size_t bytesRead = getline(&(inputBuffer->buffer), &(inputBuffer->bufferLen), stdin);
    
    if (bytesRead <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }
    
    // remove trailing newline that came from calling getline()
    inputBuffer->buffer[bytesRead - 1] = 0;
    inputBuffer->inputLen = bytesRead - 1;
}

void closeInputBuffer(InputBuffer* inputBuffer) {
    free(inputBuffer->buffer);
    free(inputBuffer);
}

MetaCommandResult execMetaCommand(InputBuffer* inputBuffer, Table* table) {
    if (strcmp(inputBuffer->buffer, ".exit") == 0) {
        dbClose(table);
        exit(EXIT_SUCCESS);
    } else if (strcmp(inputBuffer->buffer, ".btree") == 0) {
        printf("Tree:\n");
        printLeafNode(getPage(table->pager, 0));
        return META_COMMAND_SUCCESS;
    } else {
        return META_COMMAND_UNRECOGNIZED;
    }
}

PrepareResult prepareStatement(InputBuffer* inputBuffer, Statement* statement) {
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;
        int args = sscanf(
            inputBuffer->buffer, "insert %d %s %s", &(statement->rowToInsert.id), statement->rowToInsert.username, statement->rowToInsert.email
        );
        if (args < 3) {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    } else if (strncmp(inputBuffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    } else {
        return PREPARE_UNRECOGNIZED;
    }
}

PrepareResult prepareInsert(InputBuffer* inputBuffer, Statement* statement) {
    statement->type = STATEMENT_INSERT;

    char* keyword = strtok(inputBuffer->buffer, " ");
    char* idString = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if (idString == NULL || username == NULL || email == NULL) {
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(idString);
    if (id < 0) {
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(username) > COLUMN_USERNAME_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    statement->rowToInsert.id = id;
    strcpy(statement->rowToInsert.username, username);
    strcpy(statement->rowToInsert.email, email);

    return PREPARE_SUCCESS;
}

ExecuteResult executeStatement(Statement* statement, Table* table) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return executeInsert(statement, table);
        case (STATEMENT_SELECT):
            return executeSelect(statement, table);
    }
}

ExecuteResult executeInsert(Statement* statement, Table* table) {
    void* node = getPage(table->pager, table->rootPageNum);
    uint32_t numCells = (*leafNodeNumCells(node));
    if (numCells >= LEAF_NODE_MAX_CELLS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* rowToInsert = &(statement->rowToInsert);
    uint32_t keyToInsert = rowToInsert->id;
    Cursor* cursor = tableFind(table, keyToInsert);

    if (cursor->cellNum < numCells) {
        uint32_t keyAtIndex = *leafNodeKey(node, cursor->cellNum);
        if (keyAtIndex == keyToInsert) {
            return EXECUTE_DUPLICATE_KEY;
        }
    }

    leafNodeInsert(cursor, rowToInsert->id, rowToInsert);

    return EXECUTE_SUCCESS;
}

ExecuteResult executeSelect(Statement* statement, Table* table) {
    Cursor* cursor = tableStart(table);

    Row row;
    while (!(cursor->endOfTable)) {
        deserializeRow(cursorValue(cursor), &row);
        printRow(&row);
        cursorAdvance(cursor);
    }

    free(cursor);
    
    return EXECUTE_SUCCESS;
}

void serializeRow(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserializeRow(void* source, Row* destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

void* cursorValue(Cursor* cursor) {
    uint32_t pageNum = cursor->pageNum;
    void* page = getPage(cursor->table->pager, pageNum);
    return leafNodeValue(page, cursor->cellNum);
}

void cursorAdvance(Cursor* cursor) {
    uint32_t pageNum = cursor->pageNum;
    void* node = getPage(cursor->table->pager, pageNum);

    cursor->cellNum += 1;
    if (cursor->cellNum >= (*leafNodeNumCells(node))) {
        cursor->endOfTable = true;
    }
}

void printRow(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

uint32_t* leafNodeNumCells(void* node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leafNodeCell(void* node, uint32_t cellNum) {
    return node + LEAF_NODE_HEADER_SIZE + cellNum * LEAF_NODE_CELL_SIZE;
}

uint32_t* leafNodeKey(void* node, uint32_t cellNum) {
    return leafNodeCell(node, cellNum);
}

void* leafNodeValue(void* node, uint32_t cellNum) {
    return leafNodeCell(node, cellNum) + LEAF_NODE_KEY_SIZE;
}

void initializeLeafNode(void* node) {
    setNodeType(node, NODE_LEAF);
    *leafNodeNumCells(node) = 0;
}

void leafNodeInsert(Cursor* cursor, uint32_t key, Row* value) {
    void* node = getPage(cursor->table->pager, cursor->pageNum);

    uint32_t numCells = *leafNodeNumCells(node);
    if (numCells >= LEAF_NODE_MAX_CELLS) {
        // node full
        printf("Splitting leaf node logic will go here.\n");
        exit(EXIT_FAILURE);
    }

    if (cursor->cellNum < numCells) {
        // make room for new cell
        for (uint32_t i = numCells; i > cursor->cellNum; i--) {
            memcpy(leafNodeCell(node, i), leafNodeCell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leafNodeNumCells(node)) += 1;
    *(leafNodeKey(node, cursor->cellNum)) = key;
    serializeRow(value, leafNodeValue(node, cursor->cellNum));
}

void printLeafNode(void* node) {
    uint32_t numCells = *leafNodeNumCells(node);
    printf("leaf (size %d)\n", numCells);
    for (uint32_t i = 0; i < numCells; i++) {
        uint32_t key = *leafNodeKey(node, i);
        printf(" - %d : %d\n", i, key);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Must state a database filename.\n");
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    Table* table = dbOpen(filename);

    // infinite read-execute-print loop (REPL)
    InputBuffer* inputBuffer = newInputBuffer();
    while (true) {
        printPrompt();
        readInput(inputBuffer);

        // handle meta commands
        if (inputBuffer->buffer[0] == '.') {
            switch (execMetaCommand(inputBuffer, table)) {
                case (META_COMMAND_SUCCESS):
                    continue;
                case (META_COMMAND_UNRECOGNIZED):
                    printf("Unrecognized command %s\n", inputBuffer->buffer);
                    continue;
            }
        }

        // prepare and execute the statement
        Statement statement;
        switch (prepareStatement(inputBuffer, &statement)) {
            case (PREPARE_SUCCESS):
                break;
            case (PREPARE_NEGATIVE_ID):
                printf("ID must be positive.\n");
                continue;
            case (PREPARE_STRING_TOO_LONG):
                printf("String is too long.\n");
                continue;
            case (PREPARE_SYNTAX_ERROR):
                printf("Syntax error. Couldn't parse statement\n");
                continue;
            case (PREPARE_UNRECOGNIZED):
                printf("Unrecognized keyword at start of '%s'\n", inputBuffer->buffer);
                continue;
        }

        switch (executeStatement(&statement, table)) {
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;
            case (EXECUTE_DUPLICATE_KEY):
                printf("Error: Duplicate key.\n");
                break;
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
                break;
        }
    }
}