#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

static const char *LOG_FILE_PATH = "event.log";
static const unsigned char LOG_TO_STDOUT = 1;
FILE *fp;
char *pname;

void log_open(char *_pname)
{
	fp = fopen(LOG_FILE_PATH, "a");
	pname = _pname;
}

void log_append(const char *msg)
{
	// datetime
	const char *dt_fmt = "%Y-%m-%d; %H:%M:%S;";
    char dt_str[50];
    struct tm *tmp;
    time_t t = time(NULL);
    tmp = localtime(&t);
    strftime(dt_str, sizeof(dt_str), dt_fmt, tmp);

	// pid
	pid_t my_pid = getpid();

	// FIXME: malloc!
    char strbuf[200];
	sprintf(strbuf, "%s pid=%d; %s; %s \n", dt_str, my_pid, pname, msg);

	if (LOG_TO_STDOUT == 1) printf("%s", strbuf);

	// append to file
	fprintf(fp, "%s", strbuf);

	// XXX: forcing write to disk, performance penalty!
	fflush(fp);
}

void log_close()
{
	fclose(fp);
}
