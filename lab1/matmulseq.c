/***************************************************************************
 *
 * Sequential version of Matrix-Matrix multiplication
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 1024

static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

struct arg{
    int id;
};

static void
init_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++) {
	        /* Simple initialization, which enables us to easy check
	         * the correct answer. Each element in c will have the same
	         * value as SIZE after the matmul operation.
	         */
	        a[i][j] = 1.0;
	        b[i][j] = 1.0;
        }
}

void*
matmul_seq(void* param)
{
    struct arg* args = (struct arg*) param;
    int i,j;
    for(i = 0; i < SIZE; i++){
	c[args->id][i] = 0.0;
	for(j = 0; j < SIZE; j++){
		c[args->id][i] = c[args->id][i] + a[args->id][j] * b[j][i];
	}
    }
}

static void
print_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++)
	        printf(" %7.2f", c[i][j]);
	    printf("\n");
    }
}

int
main(int argc, char **argv)
{
    pthread_t* children;
    struct arg* args;
    children = malloc(SIZE*sizeof(pthread_t));
    args = malloc(SIZE*sizeof(struct arg));
    init_matrix();
    for(int i = 0; i < SIZE; i++){
	args[i].id = i;
	pthread_create(&(children[i]), NULL, matmul_seq, (void*)&args[i]);
    }
    //print_matrix();
    for(int i = 0; i < SIZE; i++)
	pthread_join(children[i], NULL);
    free(children);
    free(args);
}
