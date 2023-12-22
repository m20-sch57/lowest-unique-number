#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

int main(void) {
	char a[50];
	int i, j, k, N, *ans;
	pid_t pid;
	unsigned int seed;

	pid = getpid();
	fprintf(stderr, "%i: Started!\n", pid);
	N = 0;
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
			break;
		}
		if ('0' > *a || '9' < *a) {
			fprintf(stderr, "Bad symbol in N: %c\n", *a);
			return 1;
		}
		N = N*10 + *a - '0';
	}
	k = 0;
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
		if (*a == '\n') {
			break;
		}
		if ('0' > *a || '9' < *a) {
			fprintf(stderr, "Bad symbol in k: %c\n", *a);
			return 1;
		}
		k = k*10 + *a - '0';
	}
	fprintf(stderr, "%i: Number of players: %i, position: %i\n", pid, N, k);
	ans = malloc(sizeof(int)*N);
	if (ans == NULL) {
		perror("malloc");
		return 1;
	}
	getrandom(&seed, sizeof(int), 0);
	srand(seed);
	fprintf(stderr, "%i: Random seed: %u\n", pid, seed);
	for (i = 0; 1; ++i) {
		k = rand();
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
