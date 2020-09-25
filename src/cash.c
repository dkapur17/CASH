// Custom Header Files
#include "definitions.h"
#include "functions.h"
#include "utilities.h"

char SHELL_NAME[] = "CASH";
char GREETING[100] = "Welcome to CASH -> Cliche Average SHell\n";

// Important global variables
char PS[LOGIN_NAME_MAX + HOST_NAME_MAX + PATH_MAX + 5];
char INVOC_LOC[PATH_MAX + 1];
char PREV_LOC[PATH_MAX + 1];
char INPUT_STRING[MAX_COMMAND_LEN + 1];
char fgP;

char **__environ;

int main()
{
    // Setting up the SIGCHILD action
    installHandlers();

    // Initialzing child pool with empty process info instances
    initChildren();

    // Set flag to 0 indicating there are no foreground processes running on top of the shell
    fgP = 0;
    // Generate the intial prompt string and handle error if any
    clear();
    int psState = generatePS(1, PS, INVOC_LOC);
    psError(psState);

    // Enter the infinite loop of printing the prompt string, getting the command and executing it
    while (1)
    {
        printf("%s", PS);
        // If Ctrl-D is recieved, exit from the shell
        if (!fgets(INPUT_STRING, MAX_COMMAND_LEN, stdin))
            break;
        else if (strcmp(INPUT_STRING, "\n"))
            parseInputString(INPUT_STRING);
        else
            printf("\n");
        generatePS(0, PS, INVOC_LOC);
    }
    exit(0);
}