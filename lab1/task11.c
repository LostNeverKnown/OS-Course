#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int chopsticks[] = {0,0,0,0,0};
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct threadArg{
	int id;
	int num;
	char* name;
	pthread_mutex_t* lock;
};

void* child(void* params){
	struct threadArg *args = (struct threadArg*) params;
	int left = args->id;
	int right = (args->id+args->num-1)%args->num;
	//srand(time(NULL));
	int random = 1+rand()%4;
	printf("%s is thinking.\n", args->name);
	sleep(random);

	printf("%s: Takes left chopstick.\n", args->name);
	pthread_mutex_lock(&args->lock[left]);//locks mutex
	while(chopsticks[right] == 1 || chopsticks[left] == 1){
		printf("%s: Realeses left chopstick.\n", args->name);
		pthread_cond_wait(&cond, &args->lock[left]);//if one of right or left isnt available then release mutex and sleep until signal comes
	}
	chopsticks[left] = 1;//left stick is taken
	printf("%s: Thinking -> got left.\n", args->name);
	//srand(time(NULL));
	random = 2+rand()%6;
	sleep(random);

	printf("%s: Takes right chopstick.\n", args->name);
	pthread_mutex_lock(&args->lock[right]);//locks mutex
	while(chopsticks[right] == 1){
		printf("%s: Realeses right and left chopsticks.\n", args->name);
		pthread_cond_wait(&cond, &args->lock[right]);//if right is taken then release both and wait for signal
		//pthread_cond_wait(&cond, &args->lock[left]);
	}
	chopsticks[right] = 1;//right stick is taken
	printf("%s: Got left -> got right.\n", args->name);
	printf("%s: Eats with %d and %d\n", args->name, left, right);
	//srand(time(NULL));
	random = 5+rand()%5;
	sleep(random);

	printf("%s: Is done eating.\n", args->name);
	chopsticks[left] = 0;
	chopsticks[right] = 0;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&args->lock[left]);
	pthread_mutex_unlock(&args->lock[right]);
}

int main(int argc, char** argv){
	pthread_mutex_t* locks;
	pthread_t* prof;
	int prof_num = 5;
	char* names[] = {"Tanenbaum","Bos","Lamport","Stallings","Silberschatz"};
	struct threadArg* args;
	locks = malloc(prof_num*sizeof(pthread_mutex_t));
	prof = malloc(prof_num*sizeof(pthread_t));
	args = malloc(prof_num*sizeof(struct threadArg));
	for(int i = 0; i < prof_num; i++)
		pthread_mutex_init(&locks[i], NULL);
	for(int i = 0; i < prof_num; i++){
		args[i].id = i;
		args[i].num = prof_num;
		args[i].name = names[i];
		args[i].lock = locks;
		pthread_create(&(prof[i]), NULL, child, (void*)&args[i]);
	}
	for(int i = 0; i < prof_num; i++)
		pthread_join(prof[i], NULL);
	for(int i = 0; i < prof_num; i++)
		pthread_mutex_destroy(&locks[i]);

	free(prof);
	free(args);
	free(locks);
	pthread_cond_destroy(&cond);
	return 0;
}
