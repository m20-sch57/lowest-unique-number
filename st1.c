#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

int main(void) {
	char a[50];
	int i, j, k, N, *ans;
	unsigned int seed;
	fprintf(stderr, "Started!\n");
	/* scanf("%s%s", a, a+25); */
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
		if (*a == '\n') {
			break;
		}
		N = N*10 + *a - '0';
	}
	fprintf(stderr, "Number of players: %i\n", N);
	ans = malloc(sizeof(int)*N);
	if (ans == NULL) {
		perror("malloc");
		return 1;
	}
	getrandom(&seed, sizeof(int), 0);
	srand(seed);
	fprintf(stderr, "Random seed: %u\n", seed);
	for (i = 0; 1; ++i) {
		k = rand();
		printf("%i\n", k);
		fflush(stdout);
		fprintf(stderr, "%i\n", k);
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
		fprintf(stderr, "%i: %i\n", k, (int)a[0]);
	}
	fprintf(stderr, "Done! %s %s\n", a, a+25);
	return 0;
}
