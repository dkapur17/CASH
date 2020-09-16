#include <sys/types.h>
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int cd(char *COMMAND);

int bExec(char *COMMAND);

int echo(char *args[], char wRedir, char *outfile);

int fExec(char *COMMAND);

void printPermissions(mode_t bits);

void ls_printPermissions(mode_t bits);

void ls_printFileDetails(struct dirent *lsDir, struct stat fileStat, int *colLen);

int ls(char *args[], char wRedir, char *outfile);

void nw_interrupt(int sleepDur);

void nw_newborn(int sleepDur);

void nightswatch(char *args[]);

int pinfo(char *args[], char wRedir, char *outfile);

int pwd(char wRedir, char *outfile);

#endif