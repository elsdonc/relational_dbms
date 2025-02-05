#include "db.h"

InputBuffer* newInputBuffer() {
    InputBuffer* inputBuffer = (InputBuffer*) malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->bufferLen = 0;
    inputBuffer->inputLen = 0;
    return inputBuffer;
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
        return PREPARE_SUCCESS;
    } else if (strncmp(inputBuffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    } else {
        return PREPARE_UNRECOGNIZED;
    }
}

void executeStatement(Statement* statement) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            printf("execute insert here...\n");
            break;
        case (STATEMENT_SELECT):
            printf("execute select here...\n");
            break;
    }
}

int main(int argc, char* argv[]) {
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
            case (PREPARE_UNRECOGNIZED):
                printf("Unrecognized keyword at start of '%s'\n", inputBuffer->buffer);
                continue;
        }
        executeStatement(&statement);
        printf("Executed.\n");
    }
}