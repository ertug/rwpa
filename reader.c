#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "logger.h"
#include "common.h"


int main(int argc, char *argv[])
{
	if (argc < 1) {
		fprintf(stderr, "At least 1 arg needed.");
		exit(EXIT_FAILURE);
	}
	int reader_wait_ms = atoi(argv[1]);
	if (reader_wait_ms < 1 && reader_wait_ms > 999) {
		fprintf(stderr, "max read delay should be between 1 and 999");
		exit(EXIT_FAILURE);
	}

	log_open("reader");
	log_append("Program started.");
	chld_ins_sig_hdls();

	sem_t *mutex = mutex_get("rc");
	sem_t *mutex_wrt = mutex_get("wrt");
	int *rc = shared_rc_get();

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = reader_wait_ms * 1000000;

	while (1) {
		nanosleep(&ts, NULL);

		sem_wait(mutex);
		*rc = *rc + 1;
		if (*rc == 1) sem_wait(mutex_wrt);
		sem_post(mutex);
		
		int val = shared_file_read();
		char log_msg[100];
		sprintf(log_msg, "Read: %d", val);
		log_append(log_msg);

		sem_wait(mutex);
		*rc = *rc - 1;
		if (*rc == 0) sem_post(mutex_wrt);
		//printf("rc:%d\n", *rc);
		sem_post(mutex);
	}
}
