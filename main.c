#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "logger.h"
#include "common.h"

static const char *CONFIG_FILE_PATH = "main.cfg";
int num_readers = 0;
int num_writers = 0;
int reader_delay_ms = 0;
int writer_delay_ms = 0;


int *cpid_arr_get()
{
	static int *cpid_arr;
	if (cpid_arr == NULL) {
		int child_count = num_readers + num_writers + 1;
		cpid_arr = (int *) malloc(child_count * sizeof(int));
		if (cpid_arr == NULL) {
			fprintf(stderr, "can't malloc, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
		int i;
		for (i = 0; i < child_count; i++) cpid_arr[i] = NULL;
	}
	return cpid_arr;
}

void cpid_arr_append(pid_t cpid)
{	
	int *cpid_arr_ptr = cpid_arr_get();
	int i = 0;
	int *ptr;
	do {
		//printf("%d, cpid[%d]=%d\n", cpid, i, cpid_arr_ptr[i]);
		ptr = &cpid_arr_ptr[i];
	} while (cpid_arr_ptr[i++] != NULL);
	*ptr = cpid;
}

int *cpid_arr_release()
{
	int *cpid_arr = cpid_arr_get();
	free(cpid_arr);
	cpid_arr = NULL;
}

void terminate_children()
{
	int *cpid_arr_ptr = cpid_arr_get();
	int i = 0;
	while (cpid_arr_ptr[i] != NULL) {
		pid_t cpid = cpid_arr_ptr[i];
		
		char log_msg[100];
		if (kill(cpid, SIGUSR1) == 0) {
			sprintf(log_msg, "Interrupt signal sent to child, waiting child to die... cpid=%d", cpid);
			log_append(log_msg);

			int status;
			waitpid(cpid, &status, 0);
			int code = WEXITSTATUS(status);

			sprintf(log_msg, "Child died. cpid=%d, code=%d", cpid, code);
			log_append(log_msg);
		} else {
			sprintf(log_msg, "Interrupt signal couldn't sent to child. cpid=%d", cpid);
			log_append(log_msg);
		}
		i++;
	}
	cpid_arr_release();
}

void terminate()
{
	terminate_children();
	mutex_destroy("rc");
	mutex_destroy("wrt");
	shared_rc_destroy();
	log_close();
	exit(EXIT_SUCCESS);
}

static void sigint_hdl(int sig)
{
	log_append("Interrupt signal received, terminating...");
	terminate();
}

static void sigchld_hdl(int sig)
{
	int status;
	pid_t cpid = waitpid(-1, &status, WNOHANG);
	if (cpid > 0) {
		if (WIFEXITED(status)) {
			int code = WEXITSTATUS(status);
			char log_msg[100];
			sprintf(log_msg, "A child exited. cpid=%d, code=%d", cpid, code);
			log_append(log_msg);
		} else {
			char log_msg[100];
			sprintf(log_msg, "A child is terminated abnormally. cpid=%d", cpid);
			log_append(log_msg);
		}
	}
}

void ins_sig_hdls()
{
    struct sigaction setmask;
    sigemptyset(&setmask.sa_mask);
    setmask.sa_handler = sigint_hdl;
    setmask.sa_flags = 0;
    sigaction(SIGINT, &setmask, (struct sigaction *) NULL);

    sigemptyset(&setmask.sa_mask);
    setmask.sa_handler = sigchld_hdl;
    setmask.sa_flags = 0;
    sigaction(SIGCHLD, &setmask, (struct sigaction *) NULL);
}

void read_config()
{
	FILE *fp = fopen(CONFIG_FILE_PATH, "r");
	fscanf(fp, "%d, %d, %d, %d", &num_readers, &num_writers, &reader_delay_ms, &writer_delay_ms);
}

void fork_children(char *pname, int num, int delay)
{
	int i;
	for (i = 0; i < num; i++) {
		pid_t cpid = fork();
		if (cpid == 0) {
			char strbuf[5];
			sprintf(strbuf, "%d", delay);
			execl(pname, pname, strbuf, (char *) NULL);
		} else if (cpid > 0) {
			cpid_arr_append(cpid);
			char log_msg[100];
			sprintf(log_msg, "Started a child. cpid=%d", cpid);
			log_append(log_msg);
		} else {   
			fprintf(stderr, "can't fork, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
	}
}

void init_process_pool()
{
	int *rc = shared_rc_get();
	*rc = 0;
	fork_children("reader", num_readers, reader_delay_ms);
	fork_children("writer", num_writers, writer_delay_ms);
}

int main(int argc, char *argv[])
{
	log_open("main");
	log_append("Program started.");

	ins_sig_hdls();
	log_append("Signal handler installed.");

	read_config();
	char log_msg[100];
	const char *log_fmt = "Config file read. num_readers=%d, num_writers=%d, reader_delay_ms=%d, writer_delay_ms=%d";
	sprintf(log_msg, log_fmt, num_readers, num_writers, reader_delay_ms, writer_delay_ms);
	log_append(log_msg);

	init_process_pool();
	log_append("Child processes started.");

	//int *rc = shared_rc_get();
	while(1) {
		//printf("rc:%d", *rc);
		sleep(1);
	}
}
