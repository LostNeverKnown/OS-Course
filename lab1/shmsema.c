
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <wait.h>
#define SHMSIZE 128
#define SHM_R 0400
#define SHM_W 0200

const char *semaName1 = "my_sema1";
const char *semaName2 = "my_sema2";

int main(int argc, char **argv)
{
	struct shm_struct {
		int buffer[10];
		int fetch1; //Counts where in buffer the parent process is
		int fetch2; //Counts where in buffer the child process is
	};
	sem_t *sem_id1 = sem_open(semaName1, O_CREAT, O_RDWR, 10); //Open a new semaphore
	sem_t *sem_id2 = sem_open(semaName2, O_CREAT, O_RDWR, 0);
	volatile struct shm_struct *shmp = NULL;
	char *addr = NULL;
	pid_t pid = -1;
	int var1 = 0, var2 = 0, shmid = -1;
	struct shmid_ds *shm_buf;
	int status;

	/* allocate a chunk of shared memory */
	shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | SHM_R | SHM_W);
	shmp = (struct shm_struct *) shmat(shmid, addr, 0);
	shmp->fetch1 = 0;
	shmp->fetch2 = 0;
	pid = fork();
	if (pid != 0) {
		/* here's the parent, acting as producer */
		while (var1 < 100) {
			/* write to shmem */
			sem_wait(sem_id1); //Decreases the value by 1 if it can to run or block parent process
			var1++;
			printf("Sending %d\n", var1); fflush(stdout);
			shmp->buffer[shmp->fetch1] = var1;
			shmp->fetch1 += 1;
			if(shmp->fetch1 == 10)
				shmp->fetch1 = 0;
			usleep(100000+400000*(float)rand()/RAND_MAX);
			sem_post(sem_id2); //Increases value by 1 so child process can start
		}
		shmdt(addr);
		shmctl(shmid, IPC_RMID, shm_buf);
		sem_close(sem_id1);
		sem_close(sem_id2);
		wait(&status);
		sem_unlink(semaName1);
		sem_unlink(semaName2);
	} else {
		/* here's the child, acting as consumer */
		while (var2 < 100) {
			/* read from shmem */
			sem_wait(sem_id2);
			var2 = shmp->buffer[shmp->fetch2];
			shmp->fetch2 += 1;
			if(shmp->fetch2 == 10)
				shmp->fetch2 = 0;
			printf("Received %d\n", var2); fflush(stdout);
			usleep(200000+1800000*(float)rand()/RAND_MAX);
			sem_post(sem_id1);
		}
		shmdt(addr);
		shmctl(shmid, IPC_RMID, shm_buf);
		sem_close(sem_id1);
		sem_close(sem_id2);
	}
}
