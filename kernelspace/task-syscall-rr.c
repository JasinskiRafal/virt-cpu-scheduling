#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/klist.h>
#include <linux/slab.h>

#define RET_OK (0)
#define RET_ERR (1)

#define IDLE  (0xFFFFFFFF)
#define IDLE_PRIO (0xFFFFFFFF)
#define FIFO_SIZE (32)

typedef struct {
	pid_t pid;
	int job_time;
	int execution_slices;
	char name[4];
} job_t;

static job_t now = {IDLE, 0, IDLE_PRIO, ""};

typedef struct {
	job_t job;
	struct list_head list;
} processes_list_t;

static LIST_HEAD(processes_list);

static bool ku_is_empty(void) {
	return list_empty(&processes_list);
}

static void ku_push(job_t new_job) {
	processes_list_t *tmp = kmalloc(sizeof(*tmp), GFP_KERNEL);
	tmp->job = new_job;
	INIT_LIST_HEAD(&tmp->list);

	printk(KERN_INFO "KU_PUSH: Pushed new job %s\n", tmp->job.name);
	// insert new job in the list
	list_add_tail(&tmp->list, &processes_list);
}

static job_t ku_pop(void) {
	processes_list_t ret = {0};
	processes_list_t *tmp = list_first_entry(&processes_list, processes_list_t, list);
	ret = *tmp;
	printk(KERN_INFO "KU_POP: return %s\n", ret.job.name);
	list_del(&tmp->list);
	kfree(tmp);
	return ret.job;
}

static void ku_remove_job(job_t job_to_remove) {
	processes_list_t *pos;
	processes_list_t *tmp;

	list_for_each_entry_safe(pos, tmp, &processes_list, list) {
		if (pos->job.pid == job_to_remove.pid) {
			list_del(&pos->list);
		}
	}
}

static bool ku_is_new_id(pid_t new_pid) {
	processes_list_t *pos;
	processes_list_t *tmp;

	if (ku_is_empty()) { return true; }

	list_for_each_entry_safe(pos, tmp, &processes_list, list) {
		if (pos->job.pid == new_pid) {
			return false;
		}
	}

	return true;
}

SYSCALL_DEFINE3(os2024_ku_cpu_rr, char*, name, int, job_time, int, priority) {
	job_t new_job = {
		current->pid,
		job_time,
		0
	};
	copy_from_user(new_job.name, name, sizeof(new_job.name));

	// If idle assign the new job to current one
	if (now.pid == IDLE) now = new_job;

	// The same process has spawned a new job
	if (now.pid == new_job.pid) {
		// If the job is completed move to the next one or go IDLE
		if (job_time == 0) {
			printk(KERN_INFO "Process finished: %s\n", name);
			// remove the job from waiting list
			ku_remove_job(now);
			if (ku_is_empty()) {
				now.pid = IDLE;
				now.job_time = 0;
				now.execution_slices = 0;
				now.name[0] = '\0';
			}
			else { now = ku_pop(); }
		}
		else {
			printk(KERN_INFO "Working %s\n", name);
			now.execution_slices++;
		}

		// Accept the requested job
		return RET_OK;
	}

	// Handle a job request from a different process
	if (now.execution_slices >= 10) {
		// reset the execution slices for current job
		now.execution_slices = 0;
		// Store the previosuly running job
		if (ku_is_new_id(new_job.pid)) { ku_push(now); }
		// Execute the new job
		printk(KERN_INFO "Turn over ------> %s\n", now.name);
		now = new_job;
		return RET_OK;
	}

	// check if this id is already waiting
	if (ku_is_new_id(new_job.pid)) { ku_push(new_job); }

	return -RET_ERR;
}
