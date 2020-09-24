// functions.c -> Contains all command related implementation logic

// Custom Header Files
#include "definitions.h"
#include "functions.h"
#include "utilities.h"

// Variables Declared elsewhere
extern char OSTRING[];
extern char INVOC_LOC[];
extern char PREV_LOC[];
extern char SHELL_NAME[];
extern struct pData children[];
extern int childCount;
extern char fgP;

char DIR_PATH[PATH_MAX + 1 - MAX_FILE_NAME];
char FILE_PATH[PATH_MAX + 1];

// Method to implement background process execution
int bExec(char *args[])
{
    // If the location of the executable is relative to ~, make the path absolute
    if (args[0][0] == '~')
    {
        char execPath[PATH_MAX + 1];
        sprintf(execPath, "%s%s", INVOC_LOC, args[0] + 1);
        args[0] = execPath;
    }
    for (int i = 0; args[i] != NULL; i++)
        if (!strcmp(args[i], "&"))
        {
            args[i] = NULL;
            break;
        }

    // Fork the process, if error, return
    pid_t pid = fork();
    if (pid < 0)
        return -1;

    // In the child process
    if (pid == 0)
    {
        // If there is space in the child pool
        if (childCount < MAX_CHLD_COUNT)
        {
            // Change the process group of the child to send it to the background
            setpgid(0, 0);
            // If the command given is invalid, print an error and exit with code 16
            if (execvp(args[0], args))
            {
                fprintf(stderr, "%s Error: Command not Found: %s\n", SHELL_NAME, args[0]);
                exit(0);
            }
        }
        // If there is no more place in the pool for a new process, exit with code 17
        else
        {
            fprintf(stderr, "Child pool is full. New process cannot be added\n");
            exit(0);
        }
    }
    // In the parent process, insert the child process into the pool and wait for 1 second before resuming (for aesthetics)
    else
    {
        insertChild(pid, args[0]);
    }

    return 0;
}

int bg(char *args[])
{
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;
    if (argCount != 2)
    {
        fprintf(stderr, "Usage: fg <job number>\n");
        return 0;
    }
    int jobIndexLen = strlen(args[1]);
    for (int i = 0; i < jobIndexLen; i++)
        if (!isdigit(args[1][i]))
        {
            fprintf(stderr, "<job number> must be an integer\n");
            return 0;
        }
    int jobIndex = atoi(args[1]);
    if (jobIndex > childCount)
    {
        fprintf(stderr, "No such process\n");
        return 0;
    }

    return kill(children[jobIndex - 1].pid, SIGCONT);
}

// Method to implement setenv
int cash_setenv(char *args[])
{
    // Count number of arguments
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;

    // If ill formatted command, return usage message
    if (argCount == 1 || argCount > 3)
    {
        fprintf(stderr, "Usage: setenv var [value]\n");
        return 0;
    }

    // Set the variable and return
    return setenv(args[1], args[2] != NULL ? args[2] : "", 1);
}

// Method to implement unsetenv
int cash_unsetenv(char *args[])
{
    // Count number of arguments
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;

    // If ill formatted command, return usage message
    if (argCount != 2)
    {
        fprintf(stderr, "Usage: unsetenv var\n");
        return 0;
    }

    // Unset the variable and return
    return unsetenv(args[1]);
}

// Method to print all environment variables
int cash_env()
{
    for (char **env = __environ; *env; env++)
        printf("%s\n", *env);
    return 0;
}

// Method to implement cd
int cd(char *args[])
{
    char *LOC = args[1];
    char code;
    char temp[PATH_MAX + 1];
    strcpy(temp, PREV_LOC);
    getcwd(PREV_LOC, PATH_MAX);
    if (LOC == NULL || !strcmp(LOC, ">") || !strcmp(LOC, ">>") || !strcmp(LOC, "<") || !strcmp(LOC, "~"))
        code = chdir(INVOC_LOC);
    else if(!strcmp(LOC, "-"))
    {
        strcpy(OSTRING, temp);
        shortenPath(INVOC_LOC, OSTRING);
        printf("%s\n", OSTRING);
        code = chdir(temp);
    }
    else if (LOC[0] == '~')
    {
        code = chdir(INVOC_LOC);
        LOC += 2;
        code = chdir(LOC);
    }
    else
        code = chdir(LOC);
    if(code) // Some invalid location
        strcpy(PREV_LOC, temp);
    return code;
}

// Method to implement echo
void echo(char *args[])
{
    for (int i = 1; args[i] != NULL; i++)
    {
        if (i != 1)
            printf(" ");
        printf("%s", args[i]);
    }
    printf("\n");
    return;
}

// Method to implement foreground process execution
int fExec(char *args[])
{ // If path relative to INVOC_LOC is give, handle it
    if (args[0][0] == '~')
    {
        char execPath[PATH_MAX + 1];
        sprintf(execPath, "%s%s", INVOC_LOC, args[0] + 1);
        args[0] = execPath;
    }
    // Fork the process and handle error
    pid_t pid = fork();
    fgP = 1;
    if (pid < 0)
        return -1;

    // In the child fork, execute the command
    if (pid == 0)
    {
        if (execvp(args[0], args))
        {
            fprintf(stderr, "%s Error: Command not Found: %s\n", SHELL_NAME, args[0]);
            exit(1);
        }
    }
    // In the parent process, await completion of child
    else
    {
        int status;
        if(waitpid(pid, &status, WUNTRACED) > 0)
            if(WIFSTOPPED(status))
                insertChild(pid, args[0]);
        fgP = 0;
    }
    return 0;
}

int fg(char *args[])
{
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;
    if (argCount != 2)
    {
        fprintf(stderr, "Usage: fg <job number>\n");
        return 0;
    }
    int jobIndexLen = strlen(args[1]);
    for (int i = 0; i < jobIndexLen; i++)
        if (!isdigit(args[1][i]))
        {
            fprintf(stderr, "<job number> must be an integer\n");
            return 0;
        }
    int jobIndex = atoi(args[1]);
    if (jobIndex > childCount)
    {
        fprintf(stderr, "No such process\n");
        return 0;
    }

    pid_t pid = children[jobIndex - 1].pid;
    char pName[MAX_FILE_NAME + 1];
    strcpy(pName, children[jobIndex - 1].pName);
    removeChild(pid);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    fgP = 1;
    if(tcsetpgrp(STDIN, getpgid(pid)))
    {
        fprintf(stderr, "Could not give terminal control to job %d\n", jobIndex);
        perrorHandle(0);
        return -1;
    }
    kill(pid, SIGCONT);
    int status;
    if(waitpid(pid, &status, WUNTRACED) > 0)
        if(WIFSTOPPED(status))
            insertChild(pid, pName);
    fgP = 0;
    if(tcsetpgrp(STDIN, getpgid(0)))
    {
        fprintf(stderr, "Could not return terminal controll to the shell. Exitting the shell\n");
        perrorHandle(1);
    }
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    return 0;
}

int jobs()
{
    char filePath[20];
    char statRead[100];
    FILE *procStat;
    for (int i = 0; i < childCount; i++)
    {
        // Reading run status
        sprintf(filePath, "/proc/%d/stat", (int)children[i].pid);
        procStat = fopen(filePath, "r");
        if (procStat == NULL)
        {
            errno = ESRCH;
            return -1;
        }
        fread(statRead, 100, 1, procStat);
        fclose(procStat);
        char *temp = strtok(statRead, " ");
        temp = strtok(NULL, " ");
        temp = strtok(NULL, " ");
        char runStat = temp[0];
        // Printing the output
        printf("[%d] %s %s [%d]\n", i + 1, runStat == 'S' ? "Running" : "Stopped", children[i].pName, (int)children[i].pid);
    }
    return 0;
}

// Method to implement kjob
int kjob(char *args[])
{
    // Count arguments
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;

    // If not proper argument structure, return usage message
    if (argCount != 3)
    {
        fprintf(stderr, "Usage: kjob <job number> <signal number>\n");
        return 0;
    }

    // If 1st argument is not an integer
    int jobIndexLen = strlen(args[1]);
    for (int i = 0; i < jobIndexLen; i++)
        if (!isdigit(args[1][i]))
        {
            fprintf(stderr, "<job number> must be an integer\n");
            return 0;
        }
    // If 2nd argument is not an integer
    int sigNumLen = strlen(args[2]);
    for (int i = 0; i < sigNumLen; i++)
        if (!isdigit(args[2][i]))
        {
            fprintf(stderr, "<signal number> must be an integer\n");
            return 0;
        }

    int jobIndex = atoi(args[1]);
    int sigNum = atoi(args[2]);

    // If job number doesn't belong to any child in the pool
    if (jobIndex > childCount)
    {
        fprintf(stderr, "No such process\n");
        return 0;
    }

    // Fallback condition (kind of redundant though)
    pid_t pid = children[jobIndex - 1].pid;
    if (pid < 1)
    {
        fprintf(stderr, "No such process\n");
        return 0;
    }

    // If all is fine, kill the process
    else
        return kill(pid, sigNum);
}

// Utility Function for ls. Used to print the permissions string for the -l flag
void ls_printPermissions(mode_t bits)
{
    char permString[] = " ---------";
    if (S_ISDIR(bits))
        permString[0] = 'd';
    else if (S_ISLNK(bits))
        permString[0] = 'l';
    else if (S_ISBLK(bits))
        permString[0] = 'b';
    else if (S_ISCHR(bits))
        permString[0] = 'c';
    else if (S_ISSOCK(bits))
        permString[0] = 's';
    else if (S_ISFIFO(bits))
        permString[0] = 'f';
    else if (S_ISREG(bits))
        permString[0] = '-';
    else
        permString[0] = '?';

    if (bits & S_IRUSR)
        permString[1] = 'r';
    if (bits & S_IWUSR)
        permString[2] = 'w';
    if (bits & S_IXUSR)
        permString[3] = 'x';
    if (bits & S_IRGRP)
        permString[4] = 'r';
    if (bits & S_IWGRP)
        permString[5] = 'w';
    if (bits & S_IXGRP)
        permString[6] = 'x';
    if (bits & S_IROTH)
        permString[7] = 'r';
    if (bits & S_IWOTH)
        permString[8] = 'w';
    if (bits & S_IXOTH)
        permString[9] = 'x';

    printf("%s ", permString);
}

// Utility function for ls. Used to print the details for the -l flag
void ls_printFileDetails(struct dirent *lsDir, struct stat fileStat, int *colLen)
{
    int currLen = 0;

    // Permissions
    ls_printPermissions(fileStat.st_mode);

    // Number of symbolic links, right aligned
    currLen = digitCount(fileStat.st_nlink);
    for (int i = currLen; i < colLen[1]; i++)
        printf(" ");
    printf("%lu ", fileStat.st_nlink);

    // Owner, left aligned
    printf("%s ", getpwuid(fileStat.st_uid)->pw_name);
    currLen = strlen(getpwuid(fileStat.st_uid)->pw_name);
    for (int i = currLen; i < colLen[2]; i++)
        printf(" ");

    // Group, left aligned
    printf("%s ", getgrgid(fileStat.st_gid)->gr_name);
    currLen = strlen(getgrgid(fileStat.st_gid)->gr_name);
    for (int i = currLen; i < colLen[3]; i++)
        printf(" ");

    // File size, right align
    currLen = digitCount(fileStat.st_size);
    for (int i = currLen; i < colLen[4]; i++)
        printf(" ");
    printf("%lu ", fileStat.st_size);

    // Time, auto-formatted
    strcpy(OSTRING, asctime(localtime(&(fileStat.st_mtime))));
    OSTRING[strlen(OSTRING) - 1] = '\0';
    printf("%s ", OSTRING);

    // Filename, auto-formatted
    printf("%s\n", lsDir->d_name);
}

// Method to implement ls
int ls(char *args[], char wRedir)
{
    // Counting the number of arguments in the strings
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;

    // Calculating the provided flags by iterating over the assigned pointers
    char aFlag = 0;
    char lFlag = 0;
    int flagCount = 0;
    for (int i = 0; i < argCount; i++)
        if (args[i][0] == '-')
        {
            flagCount++;
            if (strchr(args[i], 'a') != NULL)
                aFlag = 1;
            if (strchr(args[i], 'l') != NULL)
                lFlag = 1;
        }

    // Counting the numbers of directories provided. If none are provided, then store "." in one of the pointers
    int dirCount = argCount - flagCount - 1;
    if (dirCount == 0)
        strcpy(args[0], ".");

    // For every argument
    for (int i = 0; i < argCount; i++)
    {
        // If it is neither "ls", nor a flag, it must be a directory
        if (strcmp(args[i], "ls") && args[i][0] != '-')
        {
            // Array to store max characters in a column
            int colLen[7] = {0};

            // Open the directory
            DIR *lsDirStream;
            if (!strcmp(args[i], "~"))
                strcpy(DIR_PATH, INVOC_LOC);

            // Turn paths relative to ~ into absolute paths
            else if (args[i][0] == '~')
            {
                args[i] += 1;
                strcpy(DIR_PATH, INVOC_LOC);
                strcat(DIR_PATH, args[i]);
            }
            else
                strcpy(DIR_PATH, args[i]);

            lsDirStream = opendir(DIR_PATH);
            if (lsDirStream == NULL)
                return -1;
            if (dirCount > 1)
                printf("%s:\n", DIR_PATH);

            //Padding and total size calculation logic
            int total = 0;
            struct dirent *lsDirRecon = readdir(lsDirStream);
            while (lsDirRecon != NULL)
            {
                if (lFlag)
                {
                    sprintf(FILE_PATH, "%s/%s", DIR_PATH, lsDirRecon->d_name);
                    struct stat fileStat;
                    if (lstat(FILE_PATH, &fileStat))
                        return -1;
                    char hiddenFlag = (lsDirRecon->d_name[0] == '.');
                    if ((aFlag && hiddenFlag) || (!hiddenFlag))
                    {
                        colLen[0] = 10;
                        colLen[1] = max(colLen[1], digitCount((long long)fileStat.st_nlink));
                        colLen[2] = max(colLen[2], strlen(getpwuid(fileStat.st_uid)->pw_name));
                        colLen[3] = max(colLen[3], strlen(getgrgid(fileStat.st_gid)->gr_name));
                        colLen[4] = max(colLen[4], digitCount((long long)fileStat.st_size));
                        colLen[5] = -1;
                        colLen[6] = max(colLen[6], strlen(lsDirRecon->d_name));
                        total += (fileStat.st_blocks * 512 + 1023) / 1024;
                    }
                }
                lsDirRecon = readdir(lsDirStream);
            }
            closedir(lsDirStream);

            // Actual printing logic
            lsDirStream = opendir(DIR_PATH);
            if (lsDirStream == NULL)
                return -1;
            if (lFlag)
                printf("total %d\n", total);

            // Iteratively read the files from it till no files are left
            struct dirent *lsDirPrint = readdir(lsDirStream);
            while (lsDirPrint != NULL)
            {
                // For each file, if -l is not give
                if (!lFlag)
                {
                    // But -a is given, print all files
                    char hiddenFlag = (lsDirPrint->d_name[0] == '.');
                    if ((hiddenFlag && aFlag) || (!hiddenFlag))
                        printf("%s\t", lsDirPrint->d_name);
                }
                // For each file if -l is given
                else
                {
                    // Get path for each file
                    sprintf(FILE_PATH, "%s/%s", DIR_PATH, lsDirPrint->d_name);
                    struct stat fileStat;
                    if (lstat(FILE_PATH, &fileStat))
                        return -1;
                    // If -a is given, print details of all the files
                    char hiddenFlag = (lsDirPrint->d_name[0] == '.');
                    if ((hiddenFlag && aFlag) || (!hiddenFlag))
                        ls_printFileDetails(lsDirPrint, fileStat, colLen);
                }
                // Get the next file in the current directory
                lsDirPrint = readdir(lsDirStream);
            }
            // Erase the extra new line that gets printed with -l
            if (lFlag)
                printf("\x1B[A\b");
            // Add a sperarating newline if multiple directories are specified
            printf("%s\n", dirCount > 1 ? "\n" : "");
            // Close the stream
            closedir(lsDirStream);
        }
    }
    // If multiple directories, erase the trailing newline
    if (dirCount > 1 && !wRedir)
        printf("\x1B[A\b");
    return 0;
}

// Method to handle nightswatch interrupt
void nw_interrupt(int sleepDur)
{
    // Open /proc/interrupts and print its first line, handle errors
    char interruptInfo[MAX_STAT_LEN + 1];
    int intFd = open("/proc/interrupts", O_RDONLY);
    if (intFd < 0)
    {
        fprintf(stderr, "%s Error: Unable to read interrupt file.\n", SHELL_NAME);
        return;
    }
    if (read(intFd, interruptInfo, MAX_STAT_LEN - 1) < 1)
    {
        fprintf(stderr, "%s Error: Unable to read interrupt file\n", SHELL_NAME);
        return;
    }
    char *line = NULL;
    line = strtok(interruptInfo, "\n");
    printf("%s\n", line + 4);

    // Make stdio non-blocking and non-buffered
    struct termios oldConfig, newConfig;
    tcgetattr(STDIN, &oldConfig);
    int val = fcntl(STDIN, F_SETFL, O_NONBLOCK);
    newConfig = oldConfig;
    newConfig.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN, TCSANOW, &newConfig);

    // Keep printing the 3rd line from /proc/interrupts
    // Fetch a character from stdin every second and check if it is 'q' or 'Q'
    char ip = ' ';
    int lineCount = 0;
    int len = 0;
    while (ip != 'Q' && ip != 'q')
    {
        lseek(intFd, 0, SEEK_SET);
        if (read(intFd, interruptInfo, MAX_STAT_LEN - 1) < 1)
        {
            fprintf(stderr, "%s Error: Unable to read interrupt file\n", SHELL_NAME);
            return;
        }
        lineCount = 0;
        line = strtok(interruptInfo, "\n");
        for (int i = 0; i < 2; i++)
            line = strtok(NULL, "\n");
        len = strlen(line);
        for (int i = 0; i < len; i++)
            if (isalpha(line[i]))
            {
                line[i] = '\0';
                break;
            }
        printf("%s\n", line + 4);
        for (int i = 0; i < sleepDur; i++)
        {
            sleep(1);
            ip = getchar();
            if (ip == 'Q' || ip == 'q')
                break;
        }
    }
    close(intFd);

    // Make stdin buffered again
    tcsetattr(STDIN, TCSANOW, &oldConfig);
    // Clean out the buffer
    ungetc('\n', stdin);
    scanf("%s", OSTRING);
    // Make stdin blocking again
    val &= ~O_NONBLOCK;
    fcntl(STDIN, F_SETFL, val);
}

// Method to handle nightswatch newborn (not implemented)
void nw_newborn(int sleepDur)
{
    // Open /proc/loadavg and print its first line, handle errors
    char newbornInfo[MAX_STAT_LEN + 1];
    int nbFd = open("/proc/loadavg", O_RDONLY);
    if (nbFd < 0)
    {
        fprintf(stderr, "%s Error: Unable to read loadaverage file.\n", SHELL_NAME);
        return;
    }
    // Make stdio non-blocking and non-buffered
    struct termios oldConfig, newConfig;
    tcgetattr(STDIN, &oldConfig);
    int val = fcntl(STDIN, F_SETFL, O_NONBLOCK);
    newConfig = oldConfig;
    newConfig.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN, TCSANOW, &newConfig);

    // Keep printing the last space seperated value from the string
    // Fetch a character from stdin every second and check if it is 'q' or 'Q'
    char ip = ' ';
    int lineCount = 0;
    int len = 0;
    while (ip != 'Q' && ip != 'q')
    {
        lseek(nbFd, 0, SEEK_SET);
        if (read(nbFd, newbornInfo, MAX_STAT_LEN - 1) < 1)
        {
            fprintf(stderr, "%s Error: Unable to read from /proc/loadavg\n", SHELL_NAME);
            return;
        }
        len = strlen(newbornInfo);
        char *pid;
        for (int i = len - 1; i >= 0; i--)
            if (newbornInfo[i] == ' ')
            {
                pid = newbornInfo + i + 1;
                break;
            }
        printf("%s", pid);
        for (int i = 0; i < sleepDur; i++)
        {
            sleep(1);
            ip = getchar();
            if (ip == 'Q' || ip == 'q')
                break;
        }
    }
    close(nbFd);

    // Make stdin buffered again
    tcsetattr(STDIN, TCSANOW, &oldConfig);
    // Clean out the buffer
    ungetc('\n', stdin);
    scanf("%s", OSTRING);
    // Make stdin blocking again
    val &= ~O_NONBLOCK;
    fcntl(STDIN, F_SETFL, val);
}

// Method to parse the nightswatch command and perform desired operation
void nightswatch(char *args[])
{
    // Get number of arguments
    int argCount = 0;
    while (args[argCount] != NULL)
        argCount++;

    // If not in proper format, prompt and return
    if (argCount != 2 && argCount != 4)
    {
        fprintf(stderr, "Usage: nightswatch [option] <command>\n");
        return;
    }

    // Default sleep duration 2 seconds
    int sleepDur = 2;
    if (argCount > 2)
    {
        // When flag is found get the given duration value
        int flagLoc = 0;
        for (int i = 0; i < 4; i++)
            if (!strcmp(args[i], "-n"))
            {
                flagLoc = i + 1;
                break;
            }
        sleepDur = atoi(args[flagLoc]);
        // Test again for invalid format
        if (sleepDur == 0)
        {
            fprintf(stderr, "%s Error: Invalid interval\n", SHELL_NAME);
            return;
        }
        // Make args[1] point to the file name
        if (flagLoc != 3)
            args[1] = args[3];
    }

    // Perform command check
    if (!strcmp(args[1], "interrupt"))
        nw_interrupt(sleepDur);
    else if (!strcmp(args[1], "newborn"))
        nw_newborn(sleepDur);
    else
        fprintf(stderr, "%s Error: Unknown Command: %s\n", SHELL_NAME, args[1]);
}

int overkill(char *args[])
{
    int killStat = 0;
    while (childCount)
    {
        if(children[0].pid == -1)
            return 1;
        if (kill(children[0].pid, SIGKILL))
            return 1;
    }

    return 0;
}

// Method to implement pinfo
int pinfo(char *args[])
{
    // Get required process id
    pid_t pid = 0;
    if (args[1] != NULL)
        pid = (pid_t)atoi(args[1]);
    if (pid == 0)
        pid = getpid();
    // Open respective process file and read its contents
    char filePath[20];
    sprintf(filePath, "/proc/%d/stat", (int)pid);
    char fileStatStr[MAX_STAT_LEN + 1] = {'\0'};

    FILE *procStat;
    procStat = fopen(filePath, "r");
    if (procStat == NULL)
    {
        errno = ESRCH;
        return -1;
    }
    fread(fileStatStr, MAX_STAT_LEN, 1, procStat);
    fclose(procStat);

    // A pointer for each space seperated attribute in the stat file
    char *attrs[STAT_COUNT] = {NULL};
    attrs[0] = strtok(fileStatStr, " ");
    for (int i = 1; attrs[i - 1] != NULL; i++)
        attrs[i] = strtok(NULL, " ");

    // Print required information from stat file
    printf("pid -- %d\n", (int)pid);
    printf("Process Status -- %s\n", attrs[2]);
    printf("memory -- %s\n", attrs[22]);

    // Read the exe link to get executable path
    sprintf(filePath, "/proc/%d/exe", (int)pid);
    char execPath[PATH_MAX + 1];
    memset(execPath, 0, PATH_MAX);
    int readStat = readlink(filePath, execPath, PATH_MAX);

    // If it is a zombie process, exe doesn't exist, prompt
    if (readStat != -1)
        shortenPath(INVOC_LOC, execPath);
    printf("Executable Path -- %s\n", readStat == -1 ? "Doesn't exist" : execPath);
    return 0;
}

// Method to implement pwd
void pwd()
{
    char PWD[PATH_MAX + 1];
    if (getcwd(PWD, PATH_MAX) == NULL)
    {
        fprintf(stderr, "Unable to get current directory.\n");
        perrorHandle(0);
        return;
    }
    printf("%s\n", PWD);
}