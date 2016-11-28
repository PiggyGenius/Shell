#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include "list.h"
#include "process.h"

proclist* create_list(void) 
{
	proclist* list = malloc(sizeof(proclist));
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	return list;
}

void add(proclist* list, pid_t pid, struct timeval start_time, char*** command) 
{
	/* We store the command in one string */
	proc* child = malloc(sizeof(proc));
	child->command = calloc(getlen_cmd(command), 1);
	write_cmd(&(child->command),command);
	
	child->pid = pid;
	child->start_time = start_time;
	child->next = NULL;
	child->running = true;

	if (list->head == NULL) {
		list->head = child;
		list->tail = child;
	} else {
		list->tail->next = child;
		list->tail = child;
	}
	list->size += 1;
}

void del(proclist* list, pid_t pid) 
{
	if (list->head == NULL) {
		fprintf(stderr, "Error : empty list.\n");
		return;
	}

	proc* prec = list->head;
	proc* curr = list->head;
	
	// if we remove the first element
	if (curr->pid == pid) {
		list->head = curr->next;
		free(curr->command);
		free(curr);
	}
	else {
		while (curr != NULL) {
			if (curr->pid != pid) {
				prec = curr;
				curr = curr->next;
			} else {
				prec->next = curr->next;
				// if we remove the last one
				if (curr->next == NULL) { 
					list->tail = prec;
				}
				free(curr->command);
				free(curr);
			}
		}
	}
	list->size -= 1;
}

void disp_jobs(proclist* list) 
{
	proc* child = list->head;
	char* state = NULL;
	for (uint32_t i=1; child != NULL;i++) {
		if (child->running) {
			state = "Running";
			printf("[%u]\t %s \t\t %u: %s\n",i,state,child->pid,child->command);
		} else {
			state = "Done";
			printf("[%u]\t %s \t\t %u: %s\n",i,state,child->pid,child->command);
			del(list,child->pid);
		}
		child = child->next;
	}
}

void change_state(proclist* list,pid_t pid)
{
	proc* child = list->head;
	while(child != NULL){
		if(child->pid == pid){
			child->running = !(child->running);
			break;
		}
		child = child->next;
	}
}

void clean_list(proclist* list){
	proc* child = list->head;
	for(uint32_t i=1;child != NULL;i++){
		if(!(child->running)){
			printf("[%u]\t done \t\t %u: %s\n",i,child->pid,child->command);
			del(list,child->pid);
		}
		child = child->next;
	}
}

void getchild_time(proclist* list,pid_t pid,struct timeval* tv)
{
	proc* child = list->head;
	for(uint32_t i=1;child != NULL;i++){
		if(child->pid == pid)
			*tv = child->start_time;
		child = child->next;
	}
}

void kill_children(proclist* list) 
{
	proc* child;
	while (list->head != NULL) {
		child = list->head;
		list->head = child->next;
		kill(child->pid,SIGKILL);
		free(child->command);
		free(child);
	}
}
