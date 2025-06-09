#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define OK_RET (0)
#define ERROR_RET (-1)

int main(int argc, char ** argv) {
	int syscall_number = 0;
	int job_time = 0;
	int delay_time = 0;
	int priority = 0;
	char name[4] = {0};
	int wait = 0;

	if (argc < 5) {
		printf("ERROR: Insuficient Arguments.\n");
		return ERROR_RET;
	}

	syscall_number = atoi(argv[1]);
	job_time = atoi(argv[2]);
	delay_time = atoi(argv[3]);
	strcpy(name, argv[4]);
	if (argc == 6) {
		priority = atoi(argv[5]);
	}

	sleep(delay_time);
	printf("\nProcess %s: I will use CPU for %ds.\n", name, job_time);
	job_time *= 10;


	while(job_time) {
		if (OK_RET == syscall(syscall_number, name, job_time, priority)) {
			job_time--;
		}
		else {
			wait++;
		}
		usleep(100000); // delay 100 ms
	}

	syscall(syscall_number, name, 0, priority);
	printf("Process %s finished, wait time is: %d\n", name, (wait / 10));

	return OK_RET;
}
