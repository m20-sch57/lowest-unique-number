#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

#define BUF_LEN		4
#define MAXX 		1000000

int main(void) {
	char a[50];
	int i, j, k, N, *ans, buf[BUF_LEN];
	pid_t pid;
	unsigned int seed;

	pid = getpid();
	fprintf(stderr, "%i: Started!\n", pid);
	N = 0;
	*(buf+0) = 0;
	j = 0;
	while (1) {
		i = read(0, a, 1);
		if (i < 0) {
			perror("read");
			return 1;
		}
		if (i == 0) {
			fprintf(stderr, "EOF\n");
			return 1;
		}
		if (*a == ' ') {
			buf[++j] = 0;
			if (j >= BUF_LEN) {
				goto wrong_number_of_ints;
			}
			continue;
		}
		if (*a == '\n') {
			j++;
			break;
		}
		if ('0' > *a || '9' < *a) {
			fprintf(stderr, "Bad symbol in N: %c\n", *a);
			return 1;
		}
		buf[j] = buf[j]*10 + *a - '0';
	}
	if (j != 3) {
wrong_number_of_ints:
		fprintf(stderr, "%i: expected 3 space separated nonnegative integers, got at least %i:\n", pid, j);
		for (i = 0; i < j && i < BUF_LEN; ++i) {
			fprintf(stderr, (i==j-1||i==BUF_LEN-1) ? "%i\n" : "%i ", buf[i]);
		}
		return 1;
	}
	N = buf[0];
	k = buf[1];
	fprintf(stderr, "%i: Number of players: %i, position: %i, number of turns: %i\n", pid, N, k, buf[2]);
	ans = malloc(sizeof(int)*N);
	if (ans == NULL) {
		perror("malloc");
		return 1;
	}
	getrandom(&seed, sizeof(int), 0);
	srand(seed);
	fprintf(stderr, "%i: Random seed: %u\n", pid, seed);
	for (i = 0; 1; ++i) {
		k = rand() % MAXX;
		printf("%i\n", k);
		fflush(stdout);
		fprintf(stderr, "%i: %i\n", pid, k);
		k = 0;
		j = 0;
		memset(ans, 0, sizeof(int)*N);
		while (1) {
			k = read(0, a, 1);
			if (k < 0) {
				perror("read");
				return 1;
			}
			if (k == 0) {
				k = -1;
				break;
			}
			if (*a == '\n') {
				k = 0;
				break;
			}
			if (*a == ' ') {
				j++;
				continue;
			}
			ans[j] = 10*ans[j] + *a - '0';
		}
		if (k < 0) {
			break;
		}
		/* fprintf(stderr, "%i: ", pid);
		for (j = 0; j < N; ++j) {
			fprintf(stderr, "%i%s", ans[j], (j==N-1) ? "\n" : " ");
		} */
	}
	fprintf(stderr, "%i: Done!\n", pid);
	return 0;
}
