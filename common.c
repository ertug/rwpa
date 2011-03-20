#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>



void chld_terminate()
{
	log_close();
	exit(EXIT_SUCCESS);
}

static void chld_sigint_hdl(int sig)
{
	//log_append("Interrupt signal received, ignoring.");
	//chld_terminate();
}

static void chld_sigusr1_hdl(int sig)
{
	log_append("Terminate signal received, terminating...");
	chld_terminate();
}

void chld_ins_sig_hdls()
{
    struct sigaction setmask;
    sigemptyset(&setmask.sa_mask);
    setmask.sa_handler = chld_sigint_hdl;
    setmask.sa_flags = 0;
    sigaction(SIGINT, &setmask, (struct sigaction *) NULL);

    sigemptyset(&setmask.sa_mask);
    setmask.sa_handler = chld_sigusr1_hdl;
    setmask.sa_flags = 0;
    sigaction(SIGUSR1, &setmask, (struct sigaction *) NULL);
}

sem_t *mutex_get(char *sem_name)
{
	sem_t *mutex = sem_open(sem_name, O_CREAT, 0644, 1);
	if (mutex == SEM_FAILED) {
		perror("unable to create semaphore");
		sem_unlink(sem_name);
			exit(EXIT_FAILURE);
	}
	return mutex;
}

void mutex_destroy(char *sem_name)
{
	sem_t *mutex = mutex_get(sem_name);
	sem_close(mutex);
	sem_unlink(sem_name);
}


int shmid;

int *shared_rc_get()
{
	static const int SHM_KEY = 843;

	if (shmid == NULL) {
		shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT|0666);
		if (shmid < 0) {
			perror("shmget");
			exit(EXIT_FAILURE);
		}
	}

	int *rc = shmat(shmid, NULL, 0);
	return rc;
}

void shared_rc_destroy()
{
	shmctl(shmid, IPC_RMID, 0);
}

int shared_file_read()
{
	static FILE *fp;
	if (fp == NULL) {
		fp = fopen("shared.dat", "r");
	}
	int val;
	fseek(fp, 0, SEEK_SET);
	fscanf(fp, "%d", &val);
	return val;
}

void shared_file_write(int val)
{
	static FILE *fp;
	if (fp == NULL) {
		fp = fopen("shared.dat", "w");
	}

	fseek(fp, 0, SEEK_SET);
	char strbuf[100];
	fprintf(fp, "%d", val);
	fflush(fp);
}
