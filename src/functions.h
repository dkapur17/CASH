#include <sys/types.h>
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int cd(char *COMMAND);

int bExec(char *COMMAND);

int echo(char *COMMAND);

int fExec(char *COMMAND);

void printPermissions(mode_t bits);

void ls_printPermissions(mode_t bits);

void ls_printFileDetails(struct dirent *lsDir, struct stat fileStat, int *colLen);

int ls(char *COMMAND);

void nw_interrupt(int sleepDur);

void nw_newborn(int sleepDur);

void nightswatch(char* COMMAND);

int pinfo(char *COMMAND);

int pwd();

#endif