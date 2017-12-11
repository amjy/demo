/*
 * async job
 *
 * Author: Andy <andy@iwalk.me>
 *
 */

#undef _FORTIFY_SOURCE

#include <ucontext.h>
#include <setjmp.h>

#include <stdlib.h>
#include <string.h>

#include "async_job.h"

#define ajmalloc malloc
#define ajfree   free

#define STACK_SIZE       32768

typedef struct {
    ucontext_t fibre;
    jmp_buf env;
    int env_init;
}async_fibre;

struct async_job_s {
    async_fibre fibrectx;
    async_fibre dispatcher;
    int done;
    int (*func)(void *);
    void *arg;
    int *ret;
};

static __thread async_job *__current_job = NULL;

static inline async_job **
async_current_job(void)
{
    return &__current_job;
}

#define current (*async_current_job())

static inline void
async_fibre_swapcontext(async_fibre *o, async_fibre *n)
{
    o->env_init = 1;

    if (!_setjmp(o->env)) {
        if (n->env_init) {
            _longjmp(n->env, 1);

        } else {
            setcontext(&n->fibre);        
        }
    }
}

static void
async_start_func(void)
{
    async_job *job = current;
    
    if (job->ret) {
        *job->ret = job->func(job->arg);

    } else {
        job->func(job->arg);    
    }

    job->done = 1;

    async_fibre_swapcontext(&job->fibrectx, &job->dispatcher);
}

static inline int
async_fibre_makecontext(async_fibre *fibre)
{
    fibre->env_init = 0;

    if (getcontext(&fibre->fibre) == 0) {
        fibre->fibre.uc_stack.ss_sp = ajmalloc(STACK_SIZE);
        if (fibre->fibre.uc_stack.ss_sp != NULL) {
            fibre->fibre.uc_stack.ss_size = STACK_SIZE;
            fibre->fibre.uc_link = NULL;
            makecontext(&fibre->fibre, (void *)async_start_func, 0, NULL);
            return 0;
        }
    } else {
        fibre->fibre.uc_stack.ss_sp = NULL;
    }

    return -1;
}

static inline void
async_fibre_free(async_fibre *fibre)
{
    ajfree(fibre->fibre.uc_stack.ss_sp);
    fibre->fibre.uc_stack.ss_sp = NULL;
}

async_job *
async_job_new(int (*func)(void *), void *arg, int *ret)
{
    async_job *job;

    if (!func) {
        return NULL;
    }

    job = ajmalloc(sizeof(async_job));
    if (!job) {
        return NULL;
    }

    memset(job, 0, sizeof(async_job));

    job->func = func;
    job->arg = arg;
    job->ret = ret;

    if (async_fibre_makecontext(&job->fibrectx)) {
        ajfree(job);
        job = NULL;
    }

    return job;
}

int
async_job_start(async_job *job)
{
    async_job *tjob = current;

    if (!job->done) {
        current = job;
        async_fibre_swapcontext(&job->dispatcher, &job->fibrectx);
        current = tjob;
    }

    return job->done;
}

void
async_job_pause(void)
{
    async_job *job = current;

    /*
     * just not in the job func
     */
    if (!job) {
        return;
    }

    async_fibre_swapcontext(&job->fibrectx, &job->dispatcher);
}

void
async_job_free(async_job *job)
{
    async_fibre_free(&job->fibrectx);
    ajfree(job);
}

