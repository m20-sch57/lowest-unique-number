#define _GNU_SOURCE

#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WAIT_INIT	5000
#define WAIT_SIGKILL	100
#define WAIT_SIGTERM	1000
#define WAIT_MSEC	100
#define BUFSIZE		1024
#define NUM_TURNS	1000
#define MAXX		1000000
#define MEMLIMIT	4294967296l
/* #define MEMLIMIT	2147483648l*/

#define ARG_N_POS	5
char *USAGE = "usage: %s <config_log> <machine_log> <human_log> <error_log> <N> <st_1> ... <st_N>\n";
char *FIFO_NAME_TEMPL = "fifo%i.%s";

int MACHINE_LOG;
int EXIT_CODE;

volatile sig_atomic_t got_SIGCHLD = 0;


/*struct sigaction sa;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sa.sa_handler = sa_sigchld;
sigaction(SIGINT, &sa, 0);*/
void sa_sigchld(int __attribute__((unused)) sig) {
	got_SIGCHLD = 1;
}

int main(int argc, char **argv) {
	int N, i, j, k, round_num, st_num, buf_size;
	char **in_fifos, **out_fifos;
	struct pollfd *pfds;
	struct rlimit *lim_as;
	struct timespec *tmo_p;
	struct timeval *pfinish, *pnow;
	struct sigaction *psa;
	sigset_t sigmask;
	int *in_fds, *out_fds;
	char buf[BUFSIZE];
	char *args[2];
	char *not_alive;
	int *retstatuses, *ans, *answered;
	pid_t *pids, pid;

	EXIT_CODE = EXIT_SUCCESS;
	args[1] = NULL;
	if (argc < ARG_N_POS+1) {
		fprintf(stderr, USAGE, *argv);
		return EXIT_FAILURE;
	}
	N = atoi(*(argv+ARG_N_POS));
	if (N < 2) {
		fprintf(stderr, USAGE, *argv);
		fprintf(stderr, "\tN must be at least 2 (got `%i`)\n", N);
		return EXIT_FAILURE;
	}
	if (argc-(ARG_N_POS+1) != N) {
		fprintf(stderr, USAGE, *argv);
		fprintf(stderr, "\tNumber of strategies must be N=%i (%i found)\n", N, argc-(ARG_N_POS+1));
		return EXIT_FAILURE;
	}
	if (strcmp(*(argv+1), "-") != 0) {
		k = open(*(argv+1), O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (k < 0) {
			fprintf(stderr, "'%s' exists, can't create it!\n", *(argv+1));
			perror("open");
			return EXIT_FAILURE;
		}
	} else {
		k = 1;
	}
	dprintf(k, "# N TURNS MAXX WAIT_MSEC MEMLIMIT\n# Strategies executables\n%i %i %i %i %li\n", N, NUM_TURNS, MAXX, WAIT_MSEC, MEMLIMIT);
	for (i = 0; i < N; ++i) {
		dprintf(k, (i == N-1) ? "%s\n" : "%s ", *(argv+(ARG_N_POS+1)+i));
	}
	if (k != 1) {
		fsync(k);
		close(k);
	}
	if (strcmp(*(argv+2), "-") != 0) {
		MACHINE_LOG = open(*(argv+2), O_WRONLY | O_CREAT | O_EXCL,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (MACHINE_LOG < 0) {
			fprintf(stderr, "'%s' exists, can't create it!\n", *(argv+2));
			perror("open");
			return EXIT_FAILURE;
		}
	} else {
		MACHINE_LOG = 1;
	}
	if (strcmp(*(argv+3), "-") != 0) {
		k = open(*(argv+3), O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (k < 0) {
			fprintf(stderr, "'%s' exists, can't create it!\n", *(argv+3));
			perror("open");
			return EXIT_FAILURE;
		}
		dup2(k, 1);
		close(k);
	}
	if (strcmp(*(argv+4), "-") != 0) {
		k = open(*(argv+4), O_WRONLY | O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (k < 0) {
			fprintf(stderr, "'%s' exists, can't create it!\n", *(argv+4));
			perror("open");
			return EXIT_FAILURE;
		}
		dup2(k, 2);
		close(k);
	}
	in_fifos = malloc(sizeof(char *)*N);
	if (in_fifos == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	out_fifos = malloc(sizeof(char *)*N);
	if (out_fifos == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	in_fds = malloc(sizeof(int)*N);
	if (in_fds == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	out_fds = malloc(sizeof(int)*N);
	if (out_fds == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	pfds = malloc(sizeof(struct pollfd)*N);
	if (pfds == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	tmo_p = malloc(sizeof(struct timespec));
	if (tmo_p == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	pfinish = malloc(sizeof(struct timeval));
	if (pfinish == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	pnow = malloc(sizeof(struct timeval));
	if (pnow == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	lim_as = malloc(sizeof(struct rlimit));
	if (lim_as == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	psa = malloc(sizeof(struct sigaction));
	if (psa == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	memset(psa, 0, sizeof(struct sigaction));
	not_alive = malloc(sizeof(char)*N);
	if (not_alive == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	memset(not_alive, 0, sizeof(char)*N);
	pids = malloc(sizeof(pid_t)*N);
	if (pids == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	memset(pids, 0, sizeof(pid_t)*N);
	retstatuses = malloc(sizeof(int)*N);
	if (retstatuses == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	ans = malloc(sizeof(int)*N);
	if (ans == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	memset(ans, 0, sizeof(int)*N);
	answered = malloc(sizeof(int)*N);
	if (answered == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	memset(answered, 0, sizeof(int)*N);
	for (i = 0; i < N; ++i) {
		k = snprintf(NULL, 0, FIFO_NAME_TEMPL, i, "in");
		if (k < 0) {
			perror("snprintf");
			return EXIT_FAILURE;
		}
		in_fifos[i] = malloc(sizeof(char)*(k+1));
		if (in_fifos[i] == NULL) {
			perror("malloc");
			return EXIT_FAILURE;
		}
		k = snprintf(in_fifos[i], k+1, FIFO_NAME_TEMPL, i, "in");
		if (k < 0) {
			perror("snprintf");
			return EXIT_FAILURE;
		}
		k = snprintf(NULL, 0, FIFO_NAME_TEMPL, i, "out");
		if (k < 0) {
			perror("snprintf");
			return EXIT_FAILURE;
		}
		out_fifos[i] = malloc(sizeof(char)*(k+1));
		if (out_fifos[i] == NULL) {
			perror("malloc");
			return EXIT_FAILURE;
		}
		k = snprintf(out_fifos[i], k+1, FIFO_NAME_TEMPL, i, "out");
		if (k < 0) {
			perror("snprintf");
			return EXIT_FAILURE;
		}
	}
	for (i = 0; i < N; ++i) {
		k = mkfifo(in_fifos[i], S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (k != 0) {
			perror("mkfifo");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		k = mkfifo(out_fifos[i], S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (k != 0) {
			perror("mkfifo");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		in_fds[i] = -1;
		out_fds[i] = -1;
		pids[i] = -1;
		not_alive[i] = -1;
	}
	sigemptyset(&sigmask);
	psa->sa_flags = 0;
	psa->sa_handler = sa_sigchld;
	psa->sa_mask = sigmask;
	if (sigaction(SIGCHLD, psa, NULL) == -1) {
		perror("sigaction");
		EXIT_CODE = EXIT_FAILURE;
		goto kill_all;
	}
	sigaddset(&sigmask, SIGCHLD);
	if (sigprocmask(SIG_SETMASK, &sigmask, NULL)) {
		perror("sigprocmask");
		EXIT_CODE = EXIT_FAILURE;
		goto kill_all;
	}
	sigemptyset(&sigmask);
	for (i = 0; i < N; ++i) {
		pid = fork();
		if (pid < 0) {
			perror("fork");
			EXIT_CODE = EXIT_FAILURE;
			goto kill_all;
		}
		if (pid == 0) {
			/* child */
			if (MACHINE_LOG != 1) {
				close(MACHINE_LOG);
			}
			for (j = 0; j < i; ++j) {
				if (in_fds[i] >= 0) {
					close(in_fds[j]);
				}
				if (out_fds[i]) {
					close(out_fds[j]);
				}
			}
			fprintf(stderr, "Starting to reopen 0 and 1\n");
			k = open(in_fifos[i], O_WRONLY);
			if (k == -1) {
				perror("open");
				return EXIT_FAILURE;
			}
			dup2(k, 1);
			close(k);
			k = open(out_fifos[i], O_RDONLY);
			if (k == -1) {
				perror("open");
				return EXIT_FAILURE;
			}
			dup2(k, 0);
			close(k);
			k = getrlimit(RLIMIT_AS, lim_as);
			if (k < 0) {
				perror("getrlimit");
				return EXIT_FAILURE;
			}
			if (lim_as->rlim_max == RLIM_INFINITY || lim_as->rlim_max > MEMLIMIT) {
				lim_as->rlim_cur = MEMLIMIT;
				lim_as->rlim_max = MEMLIMIT;
				k = setrlimit(RLIMIT_AS, lim_as);
				if (k < 0) {
					perror("setrlimit");
					return EXIT_FAILURE;
				}
			}
			args[0] = *(argv+(ARG_N_POS+1)+i);
			fprintf(stderr, "Calling execv `%s` (pid %i)\n", args[0], getpid());
			if (execv(*(argv+(ARG_N_POS+1)+i), args) < 0) {
				perror("execv");
				return EXIT_FAILURE;
			}
		} else {
			/* parent */
			fprintf(stderr, "Forked %i\n", pid);
			pids[i] = pid;
			not_alive[i] = 0;
			in_fds[i] = open(in_fifos[i], O_RDONLY);
			if (in_fds[i] == -1) {
				perror("open");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			out_fds[i] = open(out_fifos[i], O_WRONLY);
			if (out_fds[i] == -1) {
				perror("open");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			/* tell strategy number of players, it's position
			 * and number of turns */
			buf_size = snprintf(buf, BUFSIZE, "%i %i %i\n", N, i, NUM_TURNS);
			if (buf_size < 0) {
				perror("snprintf");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			if (buf_size >= BUFSIZE) {
				fprintf(stdout, "Too many strategies, 'N k TURNS' is longer, than %i symbols\n",
					BUFSIZE);
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			j = 0;
			do {
				k = write(out_fds[i], buf, buf_size - j);
				if (k < 0) {
					perror("write");
					EXIT_CODE = EXIT_FAILURE;
					goto kill_all;
				}
				j += k;
			} while (j < buf_size);
			/* wait and get first guess from it */
			ans[i] = 0;
			while (1) {
				k = read(in_fds[i], buf, BUFSIZE);
				if (k < 0) {
					perror("read");
					EXIT_CODE = EXIT_FAILURE;
					goto kill_all;
				}
				if (k == 0) {
					fprintf(stdout, "Process %i closed fd, aborting\n", pid);
					EXIT_CODE = EXIT_FAILURE;
					goto kill_all;
				}
				for (j = 0; j < k; ++j) {
					if ('0' <= buf[j] && buf[j] <= '9') {
						ans[i] = 10*ans[i]+buf[j]-'0';
						if (ans[i] > MAXX) {
							fprintf(stdout, "Strategy %i answered invalid number (at least %i, greater than %i), aborting\n", i, ans[i], MAXX);
							EXIT_CODE = EXIT_FAILURE;
							goto kill_all;
						}
					} else {
						if (buf[j] == '\n') {
							break;
						}
						fprintf(stdout, "Invalid char (%c) found in answer of %i strategy, aborting\n", buf[j], i);
						EXIT_CODE = EXIT_FAILURE;
						goto kill_all;
					}
				}
				if (j < k && (buf[j] > '9' || buf[j] < '0')) {
					break;
				}
			}
			fprintf(stderr, "Strategy %i: first ans is %i\n", i, ans[i]);
			(pfds+i)->fd = -1 * in_fds[i];
			(pfds+i)->events = POLLIN;
		}
	}
	j = 0;
	for (i = 0; i < N; ++i) {
		k = snprintf(buf + j, BUFSIZE - j, "%i", ans[i]);
		if (k < 0) {
			perror("snprintf");
			EXIT_CODE = EXIT_FAILURE;
			goto kill_all;
		}
		if (k + 1 >= BUFSIZE - j) {
			fprintf(stdout, "Ans string too long, aborting\n");
			EXIT_CODE = EXIT_FAILURE;
			goto kill_all;
		}
		j += k + 1;
		buf[j-1] = (i == N-1) ? '\n' : ' ';
	}
	buf_size = j;
	j = 0;
	do {
		k = write(MACHINE_LOG, buf, buf_size);
		if (k < -1) {
			perror("write");
			EXIT_CODE = EXIT_FAILURE;
			goto kill_all;
		}
		j += k;
	} while (j < buf_size);
	for (round_num = 1; round_num < NUM_TURNS; ++round_num) {
		/* doing one round */
		memset(answered, 0, sizeof(int)*N);
		memset(ans, 0, sizeof(int)*N);
		for (st_num = 0; st_num < N; ++st_num) {
			j = 0;
			do {
				k = write(out_fds[st_num], buf, buf_size);
				if (k < -1) {
					perror("write");
					EXIT_CODE = EXIT_FAILURE;
					goto kill_all;
				}
				j += k;
			} while (j < buf_size);
			(pfds+st_num)->fd = in_fds[st_num];
#ifndef NPAR
		}
#endif
		if (gettimeofday(pfinish, NULL) == -1) {
			perror("gettimeofday");
			EXIT_CODE = EXIT_FAILURE;
			goto kill_all;
		}
		pfinish->tv_usec += WAIT_MSEC % 1000 * 1000;
		pfinish->tv_sec += WAIT_MSEC / 1000 + pfinish->tv_usec / 1000000;
		pfinish->tv_usec %= 1000000;
		while (1) {
#ifndef NPAR
			k = 1;
			for (i = 0; i < N; ++i) {
				if (!answered[i]) {
					k = 0;
					break;
				}
			}
#else
			k = answered[st_num];
#endif
			if (k) {
				/* fprintf(stderr, "All finished before time was up\n"); */
				break;
			}
			if (gettimeofday(pnow, NULL) == -1) {
				perror("gettimeofday");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			if (pnow->tv_sec > pfinish->tv_sec ||
				(pnow->tv_sec == pfinish->tv_sec &&
				pnow->tv_usec >= pfinish->tv_usec)) {
				break;
			}
			if (pfinish->tv_usec < pnow->tv_usec) {
				tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec - 1;
				tmo_p->tv_nsec = 1000000000 +
					(pfinish->tv_usec - pnow->tv_usec) * 1000;
			} else {
				tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec;
				tmo_p->tv_nsec = (pfinish->tv_usec - pnow->tv_usec) * 1000;
			}
			k = ppoll(pfds, N, tmo_p, &sigmask);
			if (k == -1 && errno != EINTR) {
				fprintf(stderr, "ppoll failed\n");
				perror("ppoll");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			for (i = 0; i < N; ++i) {
				if ((pfds+i)->fd > 0 && (pfds+i)->revents != 0) {
					if ((pfds+i)->revents & POLLIN) {
						k = read(in_fds[i], buf, BUFSIZE);
						if (k == -1) {
							perror("read");
							EXIT_CODE = EXIT_FAILURE;
							goto kill_all;
						}
						for (j = 0; j < k; ++j) {
							if ('0' > buf[j] || buf[j] > '9') {
								if (buf[j] == '\n') {
									(pfds+i)->fd = -1 * in_fds[i];
									answered[i] = 1;
									break;
								}
								fprintf(stdout, "Invalid char (%c) found in answer of %i strategy, aborting\n", buf[j], i);
								EXIT_CODE = EXIT_FAILURE;
								goto kill_all;
							} else {
								ans[i] = ans[i]*10 + (int)(buf[j] - '0');
							}
						}
					} else {
						fprintf(stdout, "Err on polling of %i strategy fd\n", i);
						EXIT_CODE = EXIT_FAILURE;
						goto kill_all;
					}
				}
			}
			if (got_SIGCHLD) {
				got_SIGCHLD = 0;
				fprintf(stdout, "Got SIGCHLD\n");
				/*
				pid = wait(&k);
				if (pid == -1) {
					perror("wait");
					EXIT_CODE = EXIT_FAILURE;
					goto kill_all;
				}
				fprintf(stderr, "waited for %i\n", pid);
				for (i = 0; i < N; ++i) {
					if (pid == pids[i]) {
						not_alive[i] = 1;
						retstatuses[i] = k;
					}
				}
				*/
				for (i = 0; i < N; ++i) {
					if (not_alive[i]) {
						continue;
					}
					j = waitpid(pids[i], &k, WNOHANG);
					if (j < 0) {
						perror("waitpid");
					}
					if (j == 0) {
						continue;
					}
					fprintf(stdout, "waited for strategy %i (%i)\n", i, j);
					not_alive[i] = 1;
					retstatuses[i] = k;
				}
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
		}
#ifdef NPAR
		}
#endif
		for (i = 0; i < N; ++i) {
			fprintf(stdout, "%i-th strategy %s: %i\n", i, answered[i] ?
			"finished" : "not finished", ans[i]);
		}
		j = 0;
		for (i = 0; i < N; ++i) {
			k = snprintf(buf + j, BUFSIZE - j, "%i", ans[i]);
			if (k < 0) {
				perror("snprintf");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			if (k + 1 >= BUFSIZE - j) {
				fprintf(stdout, "Ans string too long, aborting\n");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			j += k + 1;
			buf[j-1] = (i == N-1) ? '\n' : ' ';
		}
		buf_size = j;
		j = 0;
		do {
			k = write(MACHINE_LOG, buf, buf_size);
			if (k < 0) {
				perror("write");
				EXIT_CODE = EXIT_FAILURE;
				goto kill_all;
			}
			j += k;
		} while (j < buf_size);
	} /* one round routine finished */
kill_all:
	if (MACHINE_LOG != 1) {
		if (fsync(MACHINE_LOG) < 0) {
			perror("fsync");
		}
		close(MACHINE_LOG);
	}
	for (i = 0; i < N; ++i) {
		if (in_fds[i] >= 0) {
			close(in_fds[i]);
		}
		if (out_fds[i] >= 0) {
			close(out_fds[i]);
		}
	}
	if (gettimeofday(pfinish, NULL) == -1) {
		perror("gettimeofday");
		EXIT_CODE = EXIT_FAILURE;
		goto cleanup;
	}
	pfinish->tv_usec += WAIT_SIGTERM % 1000 * 1000;
	pfinish->tv_sec += WAIT_SIGTERM / 1000 + pfinish->tv_usec / 1000000;
	pfinish->tv_usec %= 1000000;
	while (1) {
		k = 0;
		for (i = 0; i < N; ++i) {
			if (!not_alive[i]) {
				k = 1;
				break;
			}
		}
		if (k == 0) {
			break;
		}
		if (gettimeofday(pnow, NULL) == -1) {
			perror("gettimeofday");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		if (pnow->tv_sec > pfinish->tv_sec ||
			(pnow->tv_sec == pfinish->tv_sec &&
			pnow->tv_usec >= pfinish->tv_usec)) {
			break;
		}
		if (pfinish->tv_usec < pnow->tv_usec) {
			tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec - 1;
			tmo_p->tv_nsec = 1000000000 +
				(pfinish->tv_usec - pnow->tv_usec) * 1000;
		} else {
			tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec;
			tmo_p->tv_nsec = (pfinish->tv_usec - pnow->tv_usec) * 1000;
		}
		k = ppoll(pfds, 0, tmo_p, &sigmask);
		if (k == -1 && errno != EINTR) {
			fprintf(stderr, "ppoll failed\n");
			perror("ppoll");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		if (got_SIGCHLD) {
			got_SIGCHLD = 0;
			for (i = 0; i < N; ++i) {
				if (not_alive[i]) {
					continue;
				}
				j = waitpid(pids[i], &k, WNOHANG);
				if (j < 0) {
					perror("waitpid");
					EXIT_CODE = EXIT_FAILURE;
					goto cleanup;
				}
				if (j == 0) {
					continue;
				}
				fprintf(stdout, "waited for strategy %i (%i)\n", i, j);
				not_alive[i] = 1;
				retstatuses[i] = k;
			}
		}
	}
	for (i = 0; i < N; ++i) {
		if (not_alive[i]) {
			continue;
		}
		if (pids[i] > 0) {
			fprintf(stdout, "Sending SIGTERM to strategy %i (%i)\n", i, pids[i]);
			kill(pids[i], SIGTERM);
			continue;
		}
		if (pids[i] <= 0) {
			fprintf(stdout, "Bad pid (%i) of %i strategy\n", pids[i], i);
		}
	}
	if (gettimeofday(pfinish, NULL) == -1) {
		perror("gettimeofday");
		EXIT_CODE = EXIT_FAILURE;
		goto cleanup;
	}
	pfinish->tv_usec += WAIT_SIGTERM % 1000 * 1000;
	pfinish->tv_sec += WAIT_SIGTERM / 1000 + pfinish->tv_usec / 1000000;
	pfinish->tv_usec %= 1000000;
	while (1) {
		k = 0;
		for (i = 0; i < N; ++i) {
			if (!not_alive[i]) {
				k = 1;
				break;
			}
		}
		if (k == 0) {
			break;
		}
		if (gettimeofday(pnow, NULL) == -1) {
			perror("gettimeofday");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		if (pnow->tv_sec > pfinish->tv_sec ||
			(pnow->tv_sec == pfinish->tv_sec &&
			pnow->tv_usec >= pfinish->tv_usec)) {
			break;
		}
		if (pfinish->tv_usec < pnow->tv_usec) {
			tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec - 1;
			tmo_p->tv_nsec = 1000000000 +
				(pfinish->tv_usec - pnow->tv_usec) * 1000;
		} else {
			tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec;
			tmo_p->tv_nsec = (pfinish->tv_usec - pnow->tv_usec) * 1000;
		}
		k = ppoll(pfds, 0, tmo_p, &sigmask);
		if (k == -1 && errno != EINTR) {
			fprintf(stderr, "ppoll failed\n");
			perror("ppoll");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		if (got_SIGCHLD) {
			got_SIGCHLD = 0;
			for (i = 0; i < N; ++i) {
				if (not_alive[i]) {
					continue;
				}
				j = waitpid(pids[i], &k, WNOHANG);
				if (j < 0) {
					perror("waitpid");
					EXIT_CODE = EXIT_FAILURE;
					goto cleanup;
				}
				if (j == 0) {
					continue;
				}
				fprintf(stdout, "waited for strategy %i (%i)\n", i, j);
				not_alive[i] = 1;
				retstatuses[i] = k;
			}
		}
	}
	for (i = 0; i < N; ++i) {
		if (pids[i] > 0 && !not_alive[i]) {
			fprintf(stdout, "Sending SIGKILL to strategy %i (%i)\n", i, pids[i]);
			kill(pids[i], SIGKILL);
		}
	}
	if (gettimeofday(pfinish, NULL) == -1) {
		perror("gettimeofday");
		goto cleanup;
	}
	pfinish->tv_usec += WAIT_SIGKILL % 1000 * 1000;
	pfinish->tv_sec += WAIT_SIGKILL / 1000 + pfinish->tv_usec / 1000000;
	pfinish->tv_usec %= 1000000;
	while (1) {
		k = 0;
		for (i = 0; i < N; ++i) {
			if (!not_alive[i]) {
				k = 1;
				break;
			}
		}
		if (k == 0) {
			break;
		}
		if (gettimeofday(pnow, NULL) == -1) {
			perror("gettimeofday");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		if (pnow->tv_sec > pfinish->tv_sec ||
			(pnow->tv_sec == pfinish->tv_sec &&
			pnow->tv_usec >= pfinish->tv_usec)) {
			break;
		}
		if (pfinish->tv_usec < pnow->tv_usec) {
			tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec - 1;
			tmo_p->tv_nsec = 1000000000 +
				(pfinish->tv_usec - pnow->tv_usec) * 1000;
		} else {
			tmo_p->tv_sec = pfinish->tv_sec - pnow->tv_sec;
			tmo_p->tv_nsec = (pfinish->tv_usec - pnow->tv_usec) * 1000;
		}
		k = ppoll(pfds, 0, tmo_p, &sigmask);
		if (k == -1 && errno != EINTR) {
			fprintf(stderr, "ppoll failed\n");
			perror("ppoll");
			EXIT_CODE = EXIT_FAILURE;
			goto cleanup;
		}
		if (got_SIGCHLD) {
			got_SIGCHLD = 0;
			for (i = 0; i < N; ++i) {
				if (not_alive[i]) {
					continue;
				}
				j = waitpid(pids[i], &k, WNOHANG);
				if (j < 0) {
					perror("waitpid");
					EXIT_CODE = EXIT_FAILURE;
					goto cleanup;
				}
				if (j == 0) {
					continue;
				}
				fprintf(stdout, "waited for strategy %i (%i)\n", i, j);
				not_alive[i] = 1;
				retstatuses[i] = k;
			}
		}
	}
cleanup:
	for (i = 0; i < N; ++i) {
		if (not_alive[i] == -1) {
			fprintf(stdout, "%i strategy was not started\n", i);
			continue;
		}
		if (not_alive[i]) {
			if (WIFEXITED(retstatuses[i])) {
				fprintf(stdout, "%i strategy (%i) exited with %i\n", i, pids[i], WEXITSTATUS(retstatuses[i]));
			}
			if (WIFSIGNALED(retstatuses[i])) {
				fprintf(stdout, "%i strategy (%i) killed by %i\n", i, pids[i], WTERMSIG(retstatuses[i]));
			}
		}
	}
	for (i = 0; i < N; ++i) {
		k = unlink(in_fifos[i]);
		if (k != 0) {
			perror("unlink");
			return EXIT_FAILURE;
		}
		k = unlink(out_fifos[i]);
		if (k != 0) {
			perror("unlink");
			return EXIT_FAILURE;
		}
	}
	for (i = 0; i < N; ++i) {
		free(in_fifos[i]);
		free(out_fifos[i]);
	}
	free(in_fifos);
	free(out_fifos);
	free(in_fds);
	free(out_fds);
	free(pfds);
	free(tmo_p);
	free(pfinish);
	free(pnow);
	free(psa);
	free(lim_as);
	free(not_alive);
	free(pids);
	free(retstatuses);
	free(ans);
	free(answered);
	return EXIT_CODE;
}
