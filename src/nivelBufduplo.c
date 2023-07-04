/* Monitor double buffer, in bufduplo.c */

#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NIVEL_TAMBUF 200
static long nivel_buffer_0[NIVEL_TAMBUF];
static long nivel_buffer_1[NIVEL_TAMBUF];

static int nivel_in_use = 0;
static int nivel_next_insertion = 0;
static int nivel_record = -1;

static pthread_mutex_t nivel_mutual_exclusion = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t nivel_buffer_full = PTHREAD_COND_INITIALIZER;

void nivel_bufduplo_insereLeitura(long leitura) {
	pthread_mutex_lock(&nivel_mutual_exclusion);
	if (nivel_in_use == 0)
		nivel_buffer_0[nivel_next_insertion] = leitura;
	else
		nivel_buffer_1[nivel_next_insertion] = leitura;
	++nivel_next_insertion;
	if (nivel_next_insertion == NIVEL_TAMBUF) {
		nivel_record = nivel_in_use;
		nivel_in_use = (nivel_in_use + 1) % 2;
		nivel_next_insertion = 0;
		pthread_cond_signal(&nivel_buffer_full);
	}
	pthread_mutex_unlock(&nivel_mutual_exclusion);
}

long* nivel_bufduplo_esperaBufferCheio(void) {
	long* buffer;
	pthread_mutex_lock(&nivel_mutual_exclusion);
	while (nivel_record == -1)
		pthread_cond_wait(&nivel_buffer_full, &nivel_mutual_exclusion);
	if (nivel_record == 0)
		buffer = nivel_buffer_0;
	else
		buffer = nivel_buffer_1;
	nivel_record = -1;
	pthread_mutex_unlock(&nivel_mutual_exclusion);
	return buffer;
}

int nivel_tamBuf(void) {
	return NIVEL_TAMBUF;
}
