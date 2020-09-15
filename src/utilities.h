#ifndef UTILITIES_H
#define UTILITIES_H

void clear();

void cleanCommand(char *COMMAND, char *exitBool);

void cproc();

int digitCount(long long x);

void execCommand(char *COMMAND, char *exitBool);

int generatePS(char init, char *PS, char *INVOC_LOC);

void initChildren();

int insertChild(int pid, char *pName);

void installHandler();

int max(int a, int b);

void parseInputString(char *COMMAND_STRING, char *exitBool);

void perrorHandle(int quit);

void psError(int psState);

void shortenPath(char *prefix, char *path);

void sigchldHandler(int sigNum);

#endif