// definitons.h -> Header file to store custom definitons
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Limits
#define MAX_ARGS 100          // Maximum number of arguments that can be provided for a single command
#define MAX_CHLD_COUNT 512    // Maximum children processes the shell can spawn
#define MAX_COMMANDS 100      // Maximum semi-colon seperated commands that can be given in one input
#define MAX_COMMAND_LEN 50000 // Maximum length of string inputted to the shell at one go
#define MAX_FILE_NAME 256     // Maximum length of file name
#define MAX_INPUT_LEN 50000   // Maximum lenght of a single semi-colon seperated command
#define MAX_INT_READ_LEN 2048 // Maximum characters to read from /proc/interrupt
#define MAX_STAT_LEN 1024     // Maximum characters to read from the stat file
#define MAX_TOKEN_LEN 200     // Maximum length of a single space seperated token from input
#define STAT_COUNT 52         // Number of space seperated values in a processes stat file
// Streams
#define STDIN 0
#define STDOUT 1
#define STDERR 2
// Misc
#define PERM 0644 // Default permission of file created with redirection

// Structure to child process information
struct pData
{
    pid_t pid;
    char pName[MAX_FILE_NAME + 1];
};

#endif