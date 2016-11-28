#include "process.h"
#include <sys/time.h>
#include <sys/resource.h>

uint32_t getlen_cmd(char*** command)
{
	uint32_t length = 0;
	for(uint32_t i=0; command[i] != NULL;i++){
		for (uint32_t j=0; command[i][j] != NULL; j++)
			length += strlen(command[i][j])+1;
		length += 1;
	}
	return length+1;
}

/*TODO Allocating two more bytes than needed, maybe change it */
void write_cmd(char** buffer,char*** command)
{
	for(uint32_t i=0; command[i] != NULL; i++) {
		for (uint32_t j=0; command[i][j] != NULL; j++) {
			strcat(*buffer, command[i][j]);
			strcat(*buffer, " ");
		}
		strcat(*buffer, "|");
	}
	*(*buffer + strlen(*buffer)-2) = '\0';
}

void terminate(char *line, proclist* list)
{
	#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
	#endif

	if (line)
		free(line);
	/* We have to kill all our children before leaving */
	disp_jobs(list);
	kill_children(list);
	exit(0);
}

void write_error(char*** command)
{
	char* error = calloc(getlen_cmd(command),1);
	write_cmd(&error,command);
	fprintf(stderr,"ensishell: %s: comand not found\n",error);
	free(error);
}


void set_time_limit(struct rlimit* time_limit, int rlim) {
	printf("Set new time limit to %d\n", rlim);
	time_limit->rlim_cur = rlim;	
	time_limit->rlim_max = rlim+5;	
}


int special_calls(char* line,proclist* jobs_list, struct rlimit* time_limit)
{
	if (line == NULL || ! strncmp(line, "exit", 4))
		terminate(line, jobs_list);
	else if ((! strncmp(line, "jobs", 4)) && strlen(line) == 4) {
		disp_jobs(jobs_list);
		return 1;
	} else if (! strncmp(line, "ulimit", 6) && line[6] == ' ') {
		set_time_limit(time_limit, atoi(line+7));
		return 1;
	}
	return 0;
}

void pipe_process(char*** seq)
{
	int pipe_tab[2];
	pipe(pipe_tab);
	int res = fork();
	if (res == -1) {
		fprintf(stderr, "Error when trying to fork.\n");
		exit(0);
	}
	// The son becomes the 'after pipe'
	if (res != 0) {
		dup2(pipe_tab[0], 0);
		close(pipe_tab[0]);
		close(pipe_tab[1]);
		if (seq[2] == NULL) {
			execvp(*(seq[1]), seq[1]);
		} else {
			pipe_process(&(seq[1]));
		}
	// The grand son becomes the 'before pipe'
	} else {
		dup2(pipe_tab[1], 1);
		close(pipe_tab[0]);
		close(pipe_tab[1]);
		execvp(*(seq[0]), seq[0]);
	}
}

void redirect_process(struct cmdline* l)
{
	if (l->in) { // input redirection
		int in_fd = open(l->in, O_RDONLY); 
		dup2(in_fd, 0);
		close(in_fd);
	}
	if (l->out) { // output redirection
		int out_fd = open(l->out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(out_fd, 1);
		close(out_fd);
	}
}


void create_process(proclist* jobs_list, struct cmdline* l, struct rlimit* time_limit)
{
	uint32_t child_pid = fork();
	struct timeval start_time;

	if (child_pid < 0) {
		fprintf(stderr,"Error when trying to fork.\n");
		exit(0);
	}

	// FATHER PROCESS
	if (child_pid != 0) {
		if (!l->bg)
			waitpid(child_pid,NULL, 0);
		else {
			gettimeofday(&start_time,NULL);
			add(jobs_list, child_pid, start_time, l->seq);
			printf("[%u] %d\n",jobs_list->size,child_pid);
		}
	} 

	// CHILD PROCESS
	else {
		// Set time limit if defined
		if (time_limit->rlim_cur != 0) {
			// WARNING : setrlimit sets a limit for the CPU time, not the wall time
			// so a sleep command won't seem affected by it as it doesn't spend a lot of time in the CPU
			setrlimit(RLIMIT_CPU, time_limit);
		}
		// Redirect if needed
		redirect_process(l);
		// No pipe needed
		if (l->seq[1] == NULL) {
			if (execvp(**(l->seq), *(l->seq)) == -1){
				write_error(l->seq);
				exit(0);
			}
		}
		else
			pipe_process(l->seq);
	}
}

int setup_line(struct cmdline** l, char* line, proclist* jobs_list)
{
	*l = parsecmd(&line);
	/* If input stream closed, normal termination */
	if (!(*l))
		terminate(0,jobs_list);
	if ((*l)->err) {
		/* Syntax error, read another command */
		printf("error: %s\n", (*l)->err);
		return 0;
	}
	return 1;
}
