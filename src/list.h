#ifndef list_h
#define list_h

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

typedef struct child {
	pid_t pid;
	struct timeval start_time;
	char* command;
	bool running;
	struct child* next;
} proc;

typedef struct {
	proc* head;
	proc* tail;
	uint32_t size;
} proclist;

proclist* create_list(void);
void getchild_time(proclist* list, pid_t pid,struct timeval* tv);
void add(proclist* list, pid_t pid,struct timeval start_time, char*** command);
void del(proclist* list, pid_t pid);
void disp_jobs(proclist* list);
void kill_children(proclist* list);
void change_state(proclist* list, pid_t pid);
void clean_list(proclist* list);
#endif
