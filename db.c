#include "db.h"

InputBuffer* newInputBuffer() {
    InputBuffer* inputBuffer = (InputBuffer*) malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->bufferLen = 0;
    inputBuffer->inputLen = 0;
    return inputBuffer;
}

int main(int argc, char* argv[]) {
    // infinite read-execute-print loop (REPL)
    InputBuffer* inputBuffer = newInputBuffer();
    while (true) {
        printPrompt();
        readInput(inputBuffer);

        if (strcmp(inputBuffer->buffer, ".exit") == 0) {
            closeInputBuffer(inputBuffer);
            exit(EXIT_SUCCESS);
        } else {
            printf("Unrecognized comand '%s'.\n", inputBuffer->buffer);
        }
    }
}