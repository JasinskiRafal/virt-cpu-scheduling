#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/klist.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define RET_OK (0)
#define RET_ERR (1)

#define IDLE  (0xFFFFFFFF)
#define IDLE_PRIO (0xFFFFFFFF)
#define FIFO_SIZE (32)

typedef struct {
	pid_t pid;
	int job_time;
	int priority;
	char name[4];
} job_t;

static job_t now = {IDLE, 0, IDLE_PRIO, {0}};

typedef struct {
	job_t job;
	struct list_head list;
} priority_list_t;

static LIST_HEAD(priority_list);

static bool ku_is_empty(void) {
	return list_empty(&priority_list);
}

static void ku_push(job_t new_job) {
	priority_list_t *cursor;
	priority_list_t *storage;
	priority_list_t *tmp = kmalloc(sizeof(*tmp), GFP_KERNEL);
	tmp->job = new_job;
	INIT_LIST_HEAD(&tmp->list);


	// look for lower priority element
	list_for_each_entry_safe(cursor, storage, &priority_list, list) {
		if (cursor->job.priority < new_job.priority) {
			// if the new job has higher priority insert it before the cursor
			list_add_tail(&tmp->list, &cursor->list);
			return;
		}
	}

	// otherwise insert it at the end of the list
	list_add_tail(&tmp->list, &priority_list);

}

static job_t ku_pop(void) {
	priority_list_t ret = {0};
	priority_list_t *tmp = list_first_entry(&priority_list, priority_list_t, list);
	ret = *tmp;
	list_del(&tmp->list);
	kfree(tmp);
	return ret.job;

}

static bool ku_is_new_id(pid_t new_pid) {
	priority_list_t *pos;
	priority_list_t *tmp;

	if (ku_is_empty()) { return true; }

	list_for_each_entry_safe(pos, tmp, &priority_list, list) {
		if (pos->job.pid == new_pid) {
			return false;
		}
	}

	return true;
}

SYSCALL_DEFINE3(os2024_ku_cpu_prio, char*, name, int, job_time, int, priority) {
	job_t new_job = {
		current->pid,
		job_time,
		priority
	};
	copy_from_user(new_job.name, name, sizeof(new_job.name));

	// If idle assign the new job to current one
	if (now.pid == IDLE) now = new_job;

	// The same process has spawned a new job
	if (now.pid == new_job.pid) {
		// If the job is completed move to the next one or go IDLE
		if (job_time == 0) {
			printk(KERN_INFO "Process finished: %s\n", name);
			if (ku_is_empty()) {
				now.pid = IDLE;
				now.job_time = 0;
				now.priority = IDLE_PRIO;
				now.name[0] = '\0';
			}
			else { now = ku_pop(); }
		}
		else {
			printk(KERN_INFO "Working %s\n", name);
		}

		// Accept the requested job
		return RET_OK;
	}

	// Handle a job request from a different process
	if (new_job.priority > now.priority) {
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
