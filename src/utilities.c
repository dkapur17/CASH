// utilities.c -> Contains all shell related utility functions

// Custom Header Files
#include "definitions.h"
#include "functions.h"
#include "utilities.h"

// Number of currently active children processe
int childCount = 0;

// Variables declared elsewhere
extern char INVOC_LOC[];
extern char PREV_LOC[];
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
void cleanCommand(char *COMMAND)
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
        handlePipes(COMMAND_CLEANED);
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
void execCommand(char *COMMAND)
{
    // Checking for IO Redirection
    int len = strlen(COMMAND);
    char redir = 0;
    //Redirection check
    for (int i = 0; i < len; i++)
        if (COMMAND[i] == '>' || COMMAND[i] == '<')
            redir = 1;

    // 0 if no stdin redirect, 1 if there is
    char readRedir = 0;
    // 0 if no stdout redirect, 1 if >, 2 if >>
    char writeRedir = 0;

    // Breaking the command into arguments
    char TEMP[MAX_INPUT_LEN + 1];
    strcpy(TEMP, COMMAND);

    char *args[MAX_ARGS] = {NULL};
    args[0] = strtok(TEMP, " ");
    for (int i = 1; i < MAX_ARGS; i++)
    {
        args[i] = strtok(NULL, " ");
        if (args[i] == NULL)
            break;
    }

    // Location of the redirection filenames within the command
    int infileLoc = -1;
    int outfileLoc = -1;

    // Setting redirection flags and finding filename locations
    for (int i = 0; args[i] != NULL; i++)
    {
        if (!strcmp(args[i], "<"))
        {
            readRedir = 1;
            infileLoc = i + 1;
        }
        else if (!strcmp(args[i], ">"))
        {
            writeRedir = 1;
            outfileLoc = i + 1;
        }
        else if (!strcmp(args[i], ">>"))
        {
            writeRedir = 2;
            outfileLoc = i + 1;
        }
    }

    // Redirecting STDIN and STDOUT as required
    int stdinBackup;
    int stdoutBackup;
    int ipFd, opFd;
    if (readRedir)
    {
        stdinBackup = dup(STDIN);
        ipFd = open(args[infileLoc], O_RDONLY);
        if (ipFd < 0)
        {
            close(stdinBackup);
            fprintf(stderr, "Unable to redirect input\n");
            perrorHandle(0);
            return;
        }
        if (dup2(ipFd, STDIN) == -1)
        {
            fprintf(stderr, "Unable to redirect input\n");
            perrorHandle(0);
            return;
        }
    }
    if (writeRedir)
    {
        stdoutBackup = dup(STDOUT);
        int opMethod = writeRedir == 1 ? O_TRUNC : O_APPEND;
        opFd = open(args[outfileLoc], O_CREAT | O_WRONLY | opMethod, PERM);
        if (opFd < 0)
        {
            close(stdoutBackup);
            fprintf(stderr, "Unable to redirect output\n");
            perrorHandle(0);
            return;
        }
        if (dup2(opFd, STDOUT) == -1)
        {
            fprintf(stderr, "Unable to redirect output\n");
            perrorHandle(0);
            return;
        }
    }

    // Going through the command and run the required function
    if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit"))
        exit(0);
    else if (!strcmp(args[0], "clear"))
        clear();
    else if (!strcmp(args[0], "pwd"))
        pwd();
    else if (!strcmp(args[0], "cd"))
    {
        if (cd(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "echo"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        echo(args);
    }
    else if (!strcmp(args[0], "ls"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        if (ls(args, writeRedir))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "pinfo"))
    {
        if (pinfo(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "nightswatch"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        nightswatch(args);
    }
    else if (!strcmp(args[0], "setenv"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        if (cash_setenv(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "unsetenv"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        if (cash_unsetenv(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "jobs"))
        jobs();
    else if (!strcmp(args[0], "kjob"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        if (kjob(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "fg"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        if (fg(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "bg"))
    {
        if (writeRedir != 0)
            args[outfileLoc - 1] = NULL;
        if (readRedir != 0)
            args[infileLoc - 1] = NULL;
        if (bg(args))
            perrorHandle(0);
    }
    else if (!strcmp(args[0], "overkill"))
    {
        if (overkill(args))
            perrorHandle(0);
    }
    // Debugging function
    else if (!strcmp(args[0], "env"))
        cash_env();
    else
    {
        if (COMMAND[strlen(COMMAND) - 1] == '&')
        {
            COMMAND[strlen(COMMAND) - 1] = '\0';
            if (writeRedir != 0)
                args[outfileLoc - 1] = NULL;
            if (readRedir != 0)
                args[infileLoc - 1] = NULL;
            if (bExec(args))
                perrorHandle(0);
        }
        else
        {
            if (writeRedir != 0)
                args[outfileLoc - 1] = NULL;
            if (readRedir != 0)
                args[infileLoc - 1] = NULL;
            if (fExec(args))
                perrorHandle(0);
        }
    }

    // Reset STDIN and STDOUT if required
    if (readRedir)
    {
        close(ipFd);
        dup2(stdinBackup, STDIN);
    }
    if (writeRedir)
    {
        close(opFd);
        dup2(stdoutBackup, STDOUT);
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
        strcpy(PREV_LOC, INVOC_LOC);
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

// Method to handle piping logic
void handlePipes(char *inputString)
{
    // Count the number of pipe separated commands
    int comCount = 1;
    int len = strlen(inputString);
    for (int i = 0; i < len; i++)
        if (inputString[i] == '|')
            comCount++;

    // If there is just one, no pipes, just run
    if (comCount == 1)
        execCommand(inputString);

    // Otherwise
    else
    {
        // Assign one pointer to each command
        char *commands[comCount];
        for (int i = 0; i <= comCount; i++)
            commands[i] = NULL;

        comCount = 0;
        commands[0] = strtok(inputString, "|");
        while (commands[comCount] != NULL)
        {
            comCount++;
            commands[comCount] = strtok(NULL, "|");
        }

        // Remove leading and trailing spaces from each command
        for (int i = 0; i < comCount; i++)
        {
            if (commands[i][0] == ' ')
                commands[i]++;
            if (commands[i][strlen(commands[i]) - 1] == ' ')
                commands[i][strlen(commands[i]) - 1] = '\0';
        }

        // Count number of pipes and create required file descriptor pairs
        int pipeCount = comCount - 1;
        int fds[2 * pipeCount];

        // Initialize the pipe file descriptors
        for (int i = 0; i < 2 * pipeCount; i += 2)
            if (pipe(fds + i))
            {
                fprintf(stderr, "Unable to create pipe\n");
                return;
            }
        int pid;

        // For each command
        for (int i = 0; i < comCount; i++)
        {
            // Make a new child process for the shell
            pid = fork();
            // In the child process
            if (pid == 0)
            {
                // If not the last command, link the block's write fd to stdout
                if (i < pipeCount)
                    dup2(fds[2 * i + 1], STDOUT);
                // If not the first command, link the previous block's read fd to stdin to read what the previous command wrote
                if (i > 0)
                    dup2(fds[2 * i - 2], STDIN);
                // Close the pipes in the current child (just clean up)
                for (int j = 0; j < 2 * pipeCount; j++)
                    close(fds[j]);
                // Execute the command
                char *command = commands[i];
                execCommand(command);
                // Exit the process on completion
                exit(0);
            }
        }
        // Close all the pipes in the parent
        for (int i = 0; i < 2 * pipeCount; i++)
            close(fds[i]);
        // Reap all the dead child processes
        for (int i = 0; i < comCount; i++)
            wait(NULL);
    }
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
    children[childCount].pid = pid;
    strcpy(children[childCount].pName, pName);
    childCount++;
    return 0;
}

// Set up the signal handler
void installHandlers()
{
    /*-----------------SIGCHLD---------------------*/
    // Create a sigaction instance
    struct sigaction sigchld_action;
    // Initialize its memory space to 0
    memset(&sigchld_action, 0, sizeof(sigchld_action));
    // Set its handler function
    sigchld_action.sa_handler = sigchldHandler;
    // After signal is handled, restart halted syscalls
    sigchld_action.sa_flags = SA_RESTART;
    // No flags to mask
    sigemptyset(&sigchld_action.sa_mask);
    // Run the handler for ever SIGCHLD signal recieved
    sigaction(SIGCHLD, &sigchld_action, NULL);

    /*-----------------SIGINT---------------------*/
    struct sigaction sigint_action;
    memset(&sigint_action, 0, sizeof(sigint_action));
    sigint_action.sa_handler = sigintHandler;
    sigint_action.sa_flags = SA_RESTART;
    sigemptyset(&sigint_action.sa_mask);
    sigaction(SIGINT, &sigint_action, NULL);

    /*-----------------SIGTSTP---------------------*/
    struct sigaction sigtstp_action;
    memset(&sigtstp_action, 0, sizeof(sigtstp_action));
    sigtstp_action.sa_handler = sigtstpHandler;
    sigtstp_action.sa_flags = SA_RESTART;
    sigemptyset(&sigtstp_action.sa_mask);
    sigaction(SIGTSTP, &sigtstp_action, NULL);
}

// Utility function to get the max of two numbers
int max(int a, int b)
{
    return a > b ? a : b;
}

// Utility function to parse the inputted command string, extract individual commands and call the required functions
void parseInputString(char *COMMAND_STRING)
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
        cleanCommand(COMMANDS[i]);
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

// Method to remove a child from the pool, given its pid
void removeChild(pid_t pid)
{
    char found = 0;
    // If a child process that is in the middle of the array is killed, the processes
    // ahead of it are moved back one space to keep them contigeous
    for (int i = 0; i < childCount; i++)
    {
        if (children[i].pid == pid && !found)
            found = 1;
        if (found)
        {
            if (i == childCount - 1)
                children[i].pid = -1;
            else
            {
                children[i].pid = children[i + 1].pid;
                strcpy(children[i].pName, children[i + 1].pName);
            }
        }
    }
    childCount--;
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
                removeChild(children[i].pid);
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

void sigintHandler(int sigNum)
{
    write(STDOUT, "\n", 1);
    write(STDOUT, PS, strlen(PS));
}

void sigtstpHandler(int sigNum)
{
    write(STDOUT, "\n", 1);
    write(STDOUT, PS, strlen(PS));
}