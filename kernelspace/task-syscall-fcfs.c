#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/kfifo.h>

#define RET_OK (0)
#define RET_ERR (1)

#define IDLE  (0xFFFFFFFF)
#define FIFO_SIZE (32)

typedef struct {
	pid_t pid;
	int job_time;
} job_t;

static DECLARE_KFIFO(ku_fifo, job_t, FIFO_SIZE);

static pid_t now = IDLE;

static bool ku_is_empty(void) {
	return kfifo_is_empty(&ku_fifo);
}

static void ku_push(job_t new_job) {
	kfifo_put(&ku_fifo, new_job);
}

static pid_t ku_pop(void) {
	job_t temp;
	kfifo_get(&ku_fifo, &temp);
	return temp.pid;
}

static bool ku_is_new_id(pid_t new_pid) {
	return new_pid != now;
}

SYSCALL_DEFINE3(os2024_ku_cpu_fcfs, char*, name, int, job_time, int, priority) {
	job_t new_job = {
		current->pid,
		job_time
	};

	if (now == IDLE) now = new_job.pid;

	if (now == new_job.pid) {
		if (job_time == 0) {
			printk(KERN_INFO "Process finished: %s\n", name);
			if (ku_is_empty()) { now = IDLE; }
			else { now = ku_pop(); }
		}
		else {
			printk(KERN_INFO "Working: %s\n", name);
		}
		return RET_OK;
	}
	else {
		if (ku_is_new_id(new_job.pid)) { ku_push(new_job); }
		printk("Working Denied for %s\n", name);
	}

	return -RET_ERR;
}
