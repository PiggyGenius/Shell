#include "scheme.h" 

void setup_scheme(char* line)
{
	char catchligne[strlen(line) + 256];
	sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
	scm_eval_string(scm_from_locale_string(catchligne));
	free(line);
}

