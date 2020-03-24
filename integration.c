#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/sysinfo.h>

#define DX 1e-9

int handleInt(const char *str, int *num);
double integrate(double from, double to, double dx);
void *threadRoutine(void *args);
void *idleThread(void *args);

struct arg {
	double res;
	double from;
	double to;
};

int main(int argc, char *argv[]) {
	
	if (argc == 1) {
		printf("Missing argument\n");
		return 1;
	}
	
	int n = 0;
	if (handleInt(argv[1], &n)) {
		return 1;
	}
	
	int cpus = get_nprocs();
	if (n > cpus) {
		printf("Too much threads\n");
	}
	
        struct arg *args = (struct arg *) calloc(n, sizeof(struct arg));
	
	pthread_t *threads = (pthread_t *) calloc(cpus, sizeof(pthread_t));
	
	args[0].from = 0.0;
	args[n - 1].to = 1.0;
	
	for (int i = 1; i < n; i++) {
		double x = floor(1.0 / n * i / DX) * DX;
		args[i - 1].to = x;
		args[i].from = x;
	}
	
	for (int i = 0; i < n ; i++) {
		if (pthread_create(&(threads[i]), NULL, threadRoutine, (void *)(args + i)) != 0) {
			printf("Failed to create threads\n");
			exit(1);
		}
	}

	for (int i = n; i < cpus; i++) {
		if (pthread_create(&(threads[i]), NULL, idleThread, NULL)) {
			printf("Failed to create threads\n");
			exit(1);
		}
	}
	
	for (int i = 0; i < n; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			printf("Failed to join threads\n");
			return 1;
		}
	}
	
	double sum = 0;
	for (int i = 0; i < n; i++) {
		sum += args[i].res;
	}
	
	printf("%.9g\n", sum);

	free(args);
	free(threads);
}

double f(double x);

void *threadRoutine(void *args) {
	struct arg *arg = (struct arg *) args;
	arg->res = integrate(arg->from, arg->to, DX);

	return NULL;
}

void *idleThread(void *args) {
	for (;;);

	return NULL;
}

double integrate(double from, double to, double dx) {
	double sum = 0;
	double fcur = f(from);
	double fprev = 0;
	for (double x = from; x <= to; x += dx) {
	        fprev = fcur;
		fcur = f(x);
		sum += (fprev + fcur) / 2 * dx;
	}
	return sum;
}

double f(double x) {
	return sin(x);
}

int handleInt(const char *str, int *num) {
	errno = 0;
	char *endp = NULL;
	*num = strtol(str, &endp, 10);
	if (errno == ERANGE) {
		printf("You've entered too big number. Unable to comply\n");
		return 1;
	}
	if (*num < 0) {
		printf("You've entered negative number. Unable to comply\n");
		return 1;
	}
	if (endp == str) {
		printf("You've entered nothing like number. Unable to comply\n");
		return 1;
	}
	if (*num == 0) {
		printf("You've entered 0. So?");
		return 1;
	}
	return 0;
}

