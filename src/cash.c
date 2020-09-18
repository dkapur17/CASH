// Custom Header Files
#include "definitions.h"
#include "functions.h"
#include "utilities.h"

char SHELL_NAME[] = "CASH";
char GREETING[100] = "Welcome to CASH -> Cliche Average SHell\n";

// Important global variables
char PS[LOGIN_NAME_MAX + HOST_NAME_MAX + PATH_MAX + 5];
char INVOC_LOC[PATH_MAX + 1];
char INPUT_STRING[MAX_COMMAND_LEN + 1];

char **__environ;

int main()
{
    // Setting up the SIGCHILD action
    installHandler();

    // Initialzing child pool with empty process info instances
    initChildren();

    umask(PERM);

    // Generate the intial prompt string and handle error if any
    clear();
    int psState = generatePS(1, PS, INVOC_LOC);
    psError(psState);

    // Enter the infinite loop of printing the prompt string, getting the command and executing it
    while (1)
    {
        printf("%s", PS);
        fgets(INPUT_STRING, MAX_COMMAND_LEN, stdin);
        if (strcmp(INPUT_STRING, "\n"))
            parseInputString(INPUT_STRING);
        else
            printf("\n");
        generatePS(0, PS, INVOC_LOC);
    }
    exit(0);
}