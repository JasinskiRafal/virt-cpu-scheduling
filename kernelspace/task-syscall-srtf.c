#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/klist.h>
#include <linux/slab.h>

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
} work_stack_t;

LIST_HEAD(work_stack);

static bool ku_is_empty(void) {
	return list_empty(&work_stack);
}

static void ku_push(job_t new_job) {
	work_stack_t *tmp = kmalloc(sizeof(*tmp), GFP_KERNEL);
	tmp->job = new_job;
	list_add(&tmp->list, &work_stack);
}

static job_t ku_pop(void) {
	work_stack_t ret = {0};
	work_stack_t *tmp = list_first_entry(&work_stack, work_stack_t, list);
	ret = *tmp;
	list_del(&tmp->list);
	kfree(tmp);
	return ret.job;

}

static bool ku_is_new_id(pid_t new_pid) {
	work_stack_t *pos;
	work_stack_t *tmp;

	if (ku_is_empty()) { return true; }

	list_for_each_entry_safe(pos, tmp, &work_stack, list) {
		if (pos->job.pid == new_pid) {
			return false;
		}
	}

	return true;
}

SYSCALL_DEFINE3(os2024_ku_cpu_srtf, char*, name, int, job_time, int, priority) {
	job_t new_job = {
		current->pid,
		job_time
	};

	// If idle assign the new job to current one
	if (now.pid == IDLE) now = new_job;

	// The same process has spawned a new job
	if (now.pid == new_job.pid) {
		// If the job is completed move to the next one or go IDLE
		if (job_time == 0) {
			printk(KERN_INFO "Process finished: %s\n", name);
			if (ku_is_empty()) { now.pid = IDLE; now.job_time = 0; }
			else { now = ku_pop(); }
		}
		else {
			printk(KERN_INFO "Working %s\n", name);
		}

		// Accept the requested job
		return RET_OK;
	}

	// Handle a job request from a different process
	if (new_job.job_time < now.job_time) {
		// Store the previosuly running job
		if (ku_is_new_id(new_job.pid)) { ku_push(now); }
		// Execute the new job
		printk(KERN_INFO "Switched work to %s\n", name);
		now = new_job;
		return RET_OK;
	}

	// check if this id is already waiting
	if (ku_is_new_id(new_job.pid)) { ku_push(new_job); }
	printk(KERN_INFO "Work denied for %s\n", name);


	return -RET_ERR;
}
