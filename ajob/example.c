/*
 * async job
 *
 * Author: Andy <andy@iwalk.me>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "async_job.h"

struct job_context {
    int state;
    char *data;
};

int
job_func2(void *arg)
{
    int i;
    struct job_context *ctx = arg;

    printf("%s:%d job start with: %s\n", __func__, __LINE__, ctx->data);

    /* pause 3 times */
    for (i = 0; i < 3; i++) {
        ctx->state++;
        printf("%s:%d job runing, state %d\n", __func__, __LINE__, ctx->state);

        /* pause, jump to the caller of async_job_start */
        async_job_pause();
    }

    printf("%s:%d job done !\n", __func__, __LINE__);
    return i;
}

int job_func1(void *arg)
{
    int ret = 0;
    async_job *job;
    struct job_context *ctx = arg;

    printf("%s:%d job start with: %s\n", __func__, __LINE__, ctx->data);

    ctx->data = "Hello world too !";

    /* new */
    job = async_job_new(job_func2, ctx, &ret);
    if (!job) {
        return -1;
    }

    /* start or resume the job until job done */
    while(!async_job_start(job)) {

        /* do something here
         * ...
         */

        ctx->state++;
        printf("%s:%d job running, state %d\n", __func__, __LINE__, ctx->state);

        /* pause, jump to the caller of async_job_start */
        async_job_pause();
    }

    /* free */
    async_job_free(job);

    printf("%s:%d job done ret %d\n", __func__, __LINE__, ret);

    return 0;
}

int main(int argc, char **argv)
{
    async_job *job;
    struct job_context ctx;

    ctx.state = 0;
    ctx.data = "Hello world !";

    /* new */
    job = async_job_new(job_func1, &ctx, NULL);
    if (!job) {
        return -1;
    }

    /* start or resume the job until job done */
    while(!async_job_start(job)) {

        /* do something here
         * ...
         */

        ctx.state++;
        printf("%s:%d job paused, state %d\n", __func__, __LINE__, ctx.state);
    }

    /* free */
    async_job_free(job);

    printf("%s:%d job example finish !\n", __func__, __LINE__);

    return 0;
}

