#ifndef UTILITIES_H
#define UTILITIES_H

void clear();

void cleanCommand(char *COMMAND);

int digitCount(long long x);

void execCommand(char *COMMAND);

int generatePS(char init, char *PS, char *INVOC_LOC);

void handlePipes(char *inputString);

void initChildren();

int insertChild(int pid, char *pName);

void installHandlers();

int max(int a, int b);

void parseInputString(char *COMMAND_STRING);

void perrorHandle(int quit);

void psError(int psState);

void removeChild(pid_t pid);

void shortenPath(char *prefix, char *path);

void sigchldHandler(int sigNum);

void sigintHandler(int sigNum);

void sigtstpHandler(int sigNum);

#endif