#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

struct task {

	bool isOverloadTask;

	float exectime_ns;
	float nominal_exectime_ns;
	float exectime_in_rm_ns;
	float period_ns;
	float criticality;

	struct task *overload_task;
};


struct task *parseArgs(int argc, char **argv, int *tablesize);
bool OPTIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool PTIsTaskSchedNaive(struct task *table, int numEntry, bool *checkPass);
float getResponseTimeRM(struct task *table, int tablesize,
						struct task *rtask);
int criticalitySort(const void *a, const void *b);
void copyTask(struct task *dest, struct task *src);
bool checkRMTable(struct task *table, int tablesize);

#endif /* __COMMON_H__ */
