#include <semaphore.h>

void chld_ins_sig_hdls();
sem_t *mutex_get(char *sem_name);
void mutex_destroy(char *sem_name);
int *shared_rc_get();
void shared_rc_destroy();
int shared_file_read();
void shared_file_write(int val);
