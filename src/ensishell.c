/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <fcntl.h>
#include "variante.h"
#include "readcmd.h"
#include "list.h"
#include "process.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Linked list of background process */
proclist* jobs_list;

struct rlimit time_limit = {0, 5};


#if USE_GUILE == 1
#include <libguile.h>
#include "scheme.h"
/* Executes the scheme commands */
int question6_executer(char *line)
{
	struct cmdline* l;
	if(special_calls(line,jobs_list, &time_limit))
		return 1;
	if(setup_line(&l, line, jobs_list) == 0)
		return 0;
	if (l->seq[0] != NULL) {
		create_process(jobs_list, l, &time_limit);
		clean_list(jobs_list);
	}
	return 1;
}
SCM executer_wrapper(SCM x)
{
	return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


/* Our handler will deal with multiple processes running in background */
void childhandler(int s)
{
	struct timeval start_time;
	struct timeval end_time;
	double elapsed_time;
	pid_t pid = 0;
	while ((pid = waitpid(-1,NULL,WNOHANG))>0){
		/* For Terminaison Asynchrone we would do that, but we prefer not printing a message, it is weird, so we commented it */
		/*printf("%d is done !!!\n",pid);*/

		gettimeofday(&end_time,NULL);
		getchild_time(jobs_list,pid,&start_time);

		/* We print the elpased time in ms for now */
		elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000;
		elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000;

		printf("%d is done, running for: %fms\n>",pid,elapsed_time);
		fflush(stdout);
		change_state(jobs_list,pid);
	}
}



int main()
{
	printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

	/* Creating jobs list */
	jobs_list = create_list();

	#if USE_GUILE == 1
	scm_init_guile();
	/* Register "executer" function in scheme */
	scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
	#endif

	/* Linking the signal handlers */
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = childhandler;
	sigaction(SIGCHLD,&sa,NULL);

	char *line = NULL;
	char *prompt = ">";
	struct cmdline *l;

	while (1) {
		line = NULL;
		/* One memory leak per command seems unavoidable, internal memory */
		line = readline(prompt);

		#if USE_GNU_READLINE == 1
		add_history(line);
		#endif

		if (special_calls(line, jobs_list, &time_limit))
			continue;

		#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			setup_scheme(line);
			continue;
		}
		#endif

		if(setup_line(&l, line, jobs_list) == 0)
			continue;
		if (l->seq[0] != NULL) {
			create_process(jobs_list, l, &time_limit);
			clean_list(jobs_list);
		}
	}
}
