/*
 * async job
 *
 * Author: Andy <andy@iwalk.me>
 *
 */

#ifndef __ASYNC_JOB_H__
#define __ASYNC_JOB_H__

typedef struct async_job_s async_job;

/*
 * create a new job
 *
 * 'arg' is the argument of 'func', used by yourself.
 * 'ret' is a pointer to save the return value of func, set NULL to discard it.
 *
 * Warning: If the caller of async_job_new will return, and call async_job_start
 *  in other functions, 'arg' and 'ret' must not in stack, should in the heap.
 *
 */
async_job *async_job_new(int (*func)(void *), void *arg, int *ret);

/*
 * run the job, start or resume
 * job done return 1, job paused return 0;
 */
int async_job_start(async_job *job);

/*
 * pause the job, called in the job func
 */
void async_job_pause(void);

/*
 * free the job
 */
void async_job_free(async_job *job);


#endif /* __ASYNC_JOB_H__ */

