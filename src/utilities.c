// utilities.c -> Contains all shell related utility functions

// Custom Header Files
#include "definitions.h"
#include "functions.h"
#include "utilities.h"

// Number of currently active children processe
int childCount = 0;

// Variables declared elsewhere
extern char INVOC_LOC[];
extern char SHELL_NAME[];
extern char GREETING[];
extern char PS[];

// Temporary buffer to print arbitrary messages to the terminal
char OSTRING[512];

// Child process pool
struct pData children[MAX_CHLD_COUNT];

// Method to clear the terminal
void clear()
{
    printf("\e[1;1H\e[2J");
}

// Clean out extra spaces in each command. Then execute if the cleaned command is a non-zero length string
void cleanCommand(char *COMMAND, char *exitBool)
{
    // Temporary pointer to point to each space seperated word
    char *TOKEN_POINTER;
    // Temporary array will store each space seperated word along with a trailing space
    char TOKEN[PATH_MAX + 1];
    // Array to store the final cleaned command
    char COMMAND_CLEANED[MAX_INPUT_LEN + 1] = "";

    // Start splitting the command string and reforming into the cleaned command
    TOKEN_POINTER = strtok(COMMAND, " ");
    while (TOKEN_POINTER != NULL)
    {
        sprintf(TOKEN, "%s ", TOKEN_POINTER);
        strcat(COMMAND_CLEANED, TOKEN);
        // strcat(COMMAND_CLEANED, TOKEN);
        TOKEN_POINTER = strtok(NULL, " ");
    }
    // Remove trailing space
    COMMAND_CLEANED[strlen(COMMAND_CLEANED) - 1] = '\0';
    // If the command has some length, look to execute it
    if (strlen(COMMAND_CLEANED))
        execCommand(COMMAND_CLEANED, exitBool);
}

// Debugging method. Lists all active children processes.
void cproc()
{
    printf("%d child process%s\n", childCount, childCount != 1 ? "es" : "");
    for (int i = 0; i < MAX_CHLD_COUNT; i++)
        if (children[i].pid != -1)
            printf("%d -> %s\n", (int)children[i].pid, children[i].pName);
}

// Utility function to get the number of digits in a number
int digitCount(long long x)
{
    int dig = 1;
    while (x)
    {
        x /= 10;
        if (x == 0)
            break;
        dig++;
    }

    return dig;
}

// Case-wise execution of cleaned commands
void execCommand(char *COMMAND, char *exitBool)
{
    char TEMP[MAX_INPUT_LEN + 1];
    strcpy(TEMP, COMMAND);
    char *COMMAND_NAME = strtok(TEMP, " ");
    if (!strcmp(COMMAND_NAME, "exit"))
        *exitBool = 1;
    else if (!strcmp(COMMAND_NAME, "clear"))
        clear();
    else if (!strcmp(COMMAND_NAME, "pwd"))
    {
        if (pwd())
            perrorHandle(0);
    }
    else if (!strcmp(COMMAND_NAME, "cd"))
    {
        if (cd(COMMAND))
            perrorHandle(0);
    }
    else if (!strcmp(COMMAND_NAME, "echo"))
        echo(COMMAND);
    else if (!strcmp(COMMAND_NAME, "ls"))
    {
        if (ls(COMMAND))
            perrorHandle(0);
    }
    else if (!strcmp(COMMAND_NAME, "pinfo"))
    {
        if (pinfo(COMMAND))
            perrorHandle(0);
    }
    else if (!strcmp(COMMAND_NAME, "cproc"))
        cproc();
    else if (!strcmp(COMMAND_NAME, "nightswatch"))
        nightswatch(COMMAND);
    else
    {
        if (COMMAND[strlen(COMMAND) - 1] == '&')
        {
            if (bExec(COMMAND))
                perrorHandle(0);
        }
        else
        {
            if (fExec(COMMAND))
                perrorHandle(0);
        }
    }
}

// Method to generate the prompt string. Takes arguments init (inital prompt string? 1 or 0), PS (pointer to the string), INVOC_LOC (Shell invocation path)
int generatePS(char init, char *PS, char *INVOC_LOC)
{
    // Get the username and handle potential error
    struct passwd *U_INFO = NULL;
    U_INFO = getpwuid(getuid());
    char UNAME[LOGIN_NAME_MAX];
    strcpy(UNAME, U_INFO->pw_name);
    if (UNAME == NULL)
        return -1;

    // Get system name and hadle potential error
    char SYSNAME_BUFF[HOST_NAME_MAX];
    int sysname;
    sysname = gethostname(SYSNAME_BUFF, HOST_NAME_MAX);
    if (sysname == -1)
        return -2;

    // If this is the first call, save the current directory name as the Invocation Location and handle potential error
    if (init)
    {
        if (getcwd(INVOC_LOC, PATH_MAX) == NULL)
            return -3;
    }

    // Get current directory and handle potential error
    char PWD[PATH_MAX + 1];
    if (getcwd(PWD, PATH_MAX) == NULL)
        return -4;

    // Shorten PWD if pwd is a sub-directory of the invocation location
    shortenPath(INVOC_LOC, PWD);
    // If all goes well, generate the prompt string and make PS point to it
    // If the current directory is the same as the invocation location, then display ~ instead of the whole path
    sprintf(PS, "<%s@%s:%s> ", UNAME, SYSNAME_BUFF, PWD);
    return 0;
}

// Initialize the child pool to empty processes
void initChildren()
{
    for (int i = 0; i < MAX_CHLD_COUNT; i++)
    {
        children[i].pid = -1;
        strcpy(children[i].pName, "");
    }
}

// Insert a child into the pool given its name and pid
int insertChild(int pid, char *pName)
{
    // Exit if pool is full
    if (childCount == MAX_CHLD_COUNT)
        return -1;
    // Otherwise find the first empty place
    for (int i = 0; i < MAX_CHLD_COUNT; i++)
        if (children[i].pid == -1)
        {
            children[i].pid = pid;
            strcpy(children[i].pName, pName);
            childCount++;
            break;
        }
    return 0;
}

// Set up the signal handler
void installHandler()
{
    // Create a sigaction instance
    struct sigaction action;
    // Initialize its memory space to 0
    memset(&action, 0, sizeof(action));
    // Set its handler function
    action.sa_handler = sigchldHandler;
    // After signal is handled, restart halted syscalls
    action.sa_flags = SA_RESTART;
    // No flags to mask
    sigemptyset(&action.sa_mask);
    // Run the handler for ever SIGCHLD signal recieved
    sigaction(SIGCHLD, &action, NULL);
}

// Utility function to get the max of two numbers
int max(int a, int b)
{
    return a > b ? a : b;
}

// Utility function to parse the inputted command string, extract individual commands and call the required functions
void parseInputString(char *COMMAND_STRING, char *exitBool)
{
    int len = strlen(COMMAND_STRING);
    int commandCount = 0;
    // Replacing all tabs with spaces and terminating newline with ;
    for (int i = 0; i < len; i++)
    {
        if (COMMAND_STRING[i] == '\t')
            COMMAND_STRING[i] = ' ';
        else if (COMMAND_STRING[i] == '\n')
            COMMAND_STRING[i] = ';';
        if (COMMAND_STRING[i] == ';')
            commandCount++;
    }

    // One pointer for each command
    char *COMMANDS[MAX_COMMANDS + 1] = {NULL};

    // Assign one pointer to each ; seperated command
    commandCount = 0;
    COMMANDS[commandCount] = strtok(COMMAND_STRING, ";");
    while (COMMANDS[commandCount] != NULL && commandCount < MAX_COMMANDS)
    {
        commandCount++;
        COMMANDS[commandCount] = strtok(NULL, ";");
    }

    // For every ; seperated command, clean and execute it
    for (int i = 0; i < commandCount; i++)
        cleanCommand(COMMANDS[i], exitBool);
}

// Method to print the perror messages and exit if necessary
void perrorHandle(int quit)
{
    sprintf(OSTRING, "%s Error", SHELL_NAME);
    perror(OSTRING);
    if (quit)
        exit(1);
}

// Method to handle all prompt string generation issues
void psError(int psState)
{
    if (psState == 0)
        printf("%s", GREETING);
    else if (psState == -1)
        fprintf(stderr, "Couldn't retrieve username. Unable to launch shell.\n");
    else if (psState == -2)
        fprintf(stderr, "Couldn't retrieve system name. Unable to launch shell.\n");
    else if (psState == -3)
        fprintf(stderr, "Couldn't retrieve directory name. Unable to launch shell.\n");
    else if (psState = -4)
        fprintf(stderr, "Couldn't retrieve directory name. Terminating Process.\n");
    if (psState)
        perrorHandle(1);
}

// Method to shorten PWD if it is a sub-directory of INVOC_LOC
void shortenPath(char *prefix, char *path)
{
    // Get their lengths
    int invoc_len = strlen(prefix);
    int pwd_len = strlen(path);

    // If INVOC_LOC is the inital substring of PWD, then PWD must be its sub-directory, make it shorter
    if (!strncmp(prefix, path, invoc_len))
    {
        char temp[PATH_MAX + 1] = "~";
        for (int i = invoc_len, j = 1; i < pwd_len; i++, j++)
            temp[j] = path[i];

        strcpy(path, temp);
    }
}

// Method to handle the termination of a child process
void sigchldHandler(int sigNum)
{
    // Declare variables for pid, status, process name and a flag to check if one of the children from the pool was terminated
    pid_t pid;
    int status;
    int procKill = 0;
    // For every child ready to be reaped
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        status = WIFEXITED(status);
        for (int i = 0; i < MAX_CHLD_COUNT; i++)
            if (children[i].pid == pid)
            {
                // Print the termination message and set the flag
                sprintf(OSTRING, "\n%s with pid %d exited %s\n", children[i].pName, (int)pid, status != 0 ? "normally" : "abnormally");
                write(STDERR, OSTRING, strlen(OSTRING));
                children[i].pid = -1;
                childCount--;
                break;
            }
        procKill++;
    }
    // If a process from the pool was killed, print a new prompt (for aesthetics)
    if (procKill)
    {
        write(STDOUT, "\n", 1);
        write(STDOUT, PS, strlen(PS));
    }
}
