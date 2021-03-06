#include <sys/types.h>
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int cash_setenv(char *args[]);

int cash_unsetenv(char *args[]);

int cash_env();

int cd(char *args[]);

int bExec(char *args[]);

int bg(char *args[]);

void echo(char *args[]);

int fExec(char *args[]);

int fg(char *args[]);

int jobs();

int kjob(char *args[]);

void printPermissions(mode_t bits);

void ls_printPermissions(mode_t bits);

void ls_printFileDetails(struct dirent *lsDir, struct stat fileStat, int *colLen);

int ls(char *args[], char wRedir);

void nw_interrupt(int sleepDur);

void nw_newborn(int sleepDur);

void nightswatch(char *args[]);

int overkill(char *args[]);

int pinfo(char *args[]);

void pwd();

#endif