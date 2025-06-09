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

static job_t now = {IDLE, 0};

typedef struct {
	job_t job;
	struct list_head list;
} job_list_t;

static LIST_HEAD(job_list);

static bool ku_is_empty(void) {
	return list_empty(&job_list);
}

static void ku_push(job_t new_job) {
	job_list_t *tmp = kmalloc(sizeof(*tmp), GFP_KERNEL);
	tmp->job = new_job;
	list_add(&tmp->list, &job_list);
}

static job_t ku_pop(void) {
	job_list_t ret = {0};
	job_list_t *tmp = list_first_entry(&job_list, job_list_t, list);
	ret = *tmp;
	list_del(&tmp->list);
	kfree(tmp);
	return ret.job;
}


static bool ku_is_new_id(pid_t new_pid) {
	job_list_t *pos;
	job_list_t *tmp;

	if (ku_is_empty()) { return true; }

	list_for_each_entry_safe(pos, tmp, &job_list, list) {
		if (pos->job.pid == new_pid) {
			return false;
		}
	}

	return true;
}


SYSCALL_DEFINE3(os2024_ku_cpu_fcfs, char*, name, int, job_time, int, priority) {
	job_t new_job = {
		current->pid,
		job_time
	};

	if (now.pid == IDLE) now = new_job;

	if (now.pid == new_job.pid) {
		if (job_time == 0) {
			printk(KERN_INFO "Process finished: %s\n", name);
			if (ku_is_empty()) { now.pid = IDLE; now.job_time = 0; }
			else { now = ku_pop(); }
		}
		else {
			printk(KERN_INFO "Working: %s\n", name);
		}
		return RET_OK;
	}

	if (ku_is_new_id(new_job.pid)) { ku_push(new_job); }
	printk("Working Denied for %s\n", name);

	return -RET_ERR;
}
