#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "logger.h"
#include "common.h"


int main(int argc, char *argv[])
{
	if (argc < 1) {
		fprintf(stderr, "At least 1 arg needed.");
		exit(EXIT_FAILURE);
	}
	int writer_delay_ms = atoi(argv[1]);
	if (writer_delay_ms < 1 && writer_delay_ms > 999) {
		fprintf(stderr, "max write delay should be between 1 and 999");
		exit(EXIT_FAILURE);
	}

	log_open("writer");
	log_append("Program started.");
	chld_ins_sig_hdls();

	sem_t *mutex_wrt = mutex_get("wrt");

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = writer_delay_ms * 1000000;

	// seed pid to rand
	srand(getpid());

	int val;
	while (1) {
		nanosleep(&ts, NULL);

		sem_wait(mutex_wrt);

		val = rand();
		shared_file_write(val);
		char log_msg[100];
		sprintf(log_msg, "Written: %d", val);
		log_append(log_msg);

		sem_post(mutex_wrt);
	}
}
