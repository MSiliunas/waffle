#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>

const char *CMD_HELP = "help";
const char *CMD_TYPE_CIRCLE = "circle";
const char *CMD_INPUT_FILE = "--input-file";
const char *CMD_INPUT_FILE_SHORT = "-i";

char *getInputFilename(char *argv[], int argc);
void *cropToCircle(void *arg);
void *showProgress(void *arg);
void advance_cursor();
void printHelp();
void printHeader();
