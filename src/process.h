#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "readcmd.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>


#ifndef PROCESS_H
#define PROCESS_H

void terminate(char *line, proclist* list);

void pipe_process(char*** seq);

void redirect_process(struct cmdline* l);

void create_process(proclist* jobs_list, struct cmdline* l, struct rlimit* time_limit);

int setup_line(struct cmdline** l, char* line, proclist* jobs_list);

int special_calls(char* line,proclist* jobs_list, struct rlimit* time_limit);

uint32_t getlen_cmd(char*** command);

void write_cmd(char** buffer,char*** command);

void write_error(char*** command);

#endif
