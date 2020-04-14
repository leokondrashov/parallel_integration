#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#define DX 1e-9

int max(int a, int b) {
	if (a > b)
		return a;
	return b;
}

int handleInt(const char *str, int *num);
void parseCPUmask(const char *buf, cpu_set_t *set);
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
	
	int cpus_conf = get_nprocs_conf();
	char *buf = calloc(3 * cpus_conf, sizeof(char));
	cpu_set_t *mask = (cpu_set_t *) calloc(1, sizeof(cpu_set_t));

	int fd = open("/sys/devices/system/cpu/online", O_RDONLY);
	read(fd, buf, 3 * cpus_conf);
	close(fd);
	
	parseCPUmask(buf, mask);
	free(buf);
	
	int cpus = CPU_COUNT(mask);
	
        struct arg *args = (struct arg *) calloc(n, sizeof(struct arg));
	
	pthread_t *threads = (pthread_t *) calloc(max(cpus, n), sizeof(pthread_t));
	
	args[0].from = 0.0;
	args[n - 1].to = 1.0;
	
	for (int i = 1; i < n; i++) {
		double x = floor(1.0 / n * i / DX) * DX;
//		printf("%.10g\n", x);
		args[i - 1].to = x;
		args[i].from = x;
	}

	int cpu = 0;
	cpu_set_t *set = calloc(1, sizeof(cpu_set_t));
	
	for (int i = 0; i < n; i++) {
		while (!CPU_ISSET(cpu, mask)) {
			cpu = (cpu + 1) % cpus_conf;
		}
		
		if (pthread_create(&(threads[i]), NULL, threadRoutine, (void *)(args + i)) != 0) {
			printf("Failed to create threads\n");
			exit(1);
		}
		
		CPU_ZERO(set);
		CPU_SET(cpu, set);
		pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), set);		
		cpu++;
	}

	for (int i = n; i < cpus; i++) {
		if (pthread_create(&(threads[i]), NULL, idleThread, NULL)) {
			printf("Failed to create threads\n");
			return 1;
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

	free(set);
	free(mask);
	free(args);
	free(threads);
}

double f(double x);

void *threadRoutine(void *args) {
	assert(args);
	
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

void parseCPUmask(const char *buf, cpu_set_t *set) {
	assert(buf);
	assert(set);

	CPU_ZERO(set);
	
	const char *i = buf;
	int cpu = 0;
	char c = '\0';
	int mov = 0;
	
	while (*i != '\0') {
		sscanf(i, "%d%c%n", &cpu, &c, &mov);
		i += mov;
		
		if (c == ',') {
			CPU_SET(cpu, set);
			continue;
		}

		if (c == '\0') {
		        break;
		}

		int cpu_end = 0;
		sscanf(i, "%d%c%n", &cpu_end, &c, &mov);
		i += mov;
		
		for (; cpu <= cpu_end; cpu++) {
			CPU_SET(cpu, set);
		}
	}
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
