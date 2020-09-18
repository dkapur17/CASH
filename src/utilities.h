#ifndef UTILITIES_H
#define UTILITIES_H

int clear(char wRedir, char *outfile);

void cleanCommand(char *COMMAND);

void cproc();

int digitCount(long long x);

void execCommand(char *COMMAND);

int generatePS(char init, char *PS, char *INVOC_LOC);

void handlePipes(char *inputString);

void initChildren();

int insertChild(int pid, char *pName);

void installHandler();

int max(int a, int b);

void parseInputString(char *COMMAND_STRING);

void perrorHandle(int quit);

void psError(int psState);

void shortenPath(char *prefix, char *path);

void sigchldHandler(int sigNum);

#endif