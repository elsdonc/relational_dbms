#include "db.h"

InputBuffer* newInputBuffer() {
    InputBuffer* inputBuffer = (InputBuffer*) malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->bufferLen = 0;
    inputBuffer->inputLen = 0;
    return inputBuffer;
}

Table* newTable() {
    Table* table = (Table*) malloc(sizeof(Table));
    table->numRows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table* table) {
    for (int i = 0; table->pages[i]; i++) {
	    free(table->pages[i]);
    }
    free(table);
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

MetaCommandResult execMetaCommand(InputBuffer* inputBuffer) {
    if (strcmp(inputBuffer->buffer, ".exit") == 0) {
        closeInputBuffer(inputBuffer);
        exit(EXIT_SUCCESS);
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
    if (table->numRows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* rowToInsert = &(statement->rowToInsert);

    serializeRow(rowToInsert, rowSlot(table, table->numRows));
    table->numRows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult executeSelect(Statement* statement, Table* table) {
    Row row;
    for (uint32_t i = 0; i < table->numRows; i++) {
        deserializeRow(rowSlot(table, i), &row);
        printRow(&row);
    }
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

void* rowSlot(Table* table, uint32_t rowNum) {
    uint32_t page_num = rowNum / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if (page == NULL) {
        // Allocate memory only when we try to access page
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = rowNum % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

void printRow(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

int main(int argc, char* argv[]) {
    Table* table = newTable();
    // infinite read-execute-print loop (REPL)
    InputBuffer* inputBuffer = newInputBuffer();
    while (true) {
        printPrompt();
        readInput(inputBuffer);

        // handle meta commands
        if (inputBuffer->buffer[0] == '.') {
            switch (execMetaCommand(inputBuffer)) {
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
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
                break;
        }
    }
}