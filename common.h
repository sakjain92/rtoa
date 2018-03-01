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

	double orig_nominal_exectime_ns;
	double orig_period_ns;

	double exectime_ns;
	double nominal_exectime_ns;
	double exectime_in_rm_ns;
	double period_ns;
	double criticality;

	struct task *overload_task;
};


double getResponseTimeCAPA(struct task *table, int tablesize, struct task *rtask);
struct task *parseArgs(int argc, char **argv, int *tablesize);

bool OPTIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool VOPTIsTaskSched(struct task *table, int numEntry, bool *checkPass);

bool PTIsTaskSchedNaive(struct task *table, int numEntry, bool *checkPass);
double getResponseTimeRM(struct task *table, int tablesize,
						struct task *rtask);
int criticalitySort(const void *a, const void *b);
void copyTask(struct task *dest, struct task *src);
bool checkRMTable(struct task *table, int tablesize);

bool CAPAIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool PTIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool ZSRMSIsTaskSched(struct task *table, int numEntry, bool *checkPass);

double getResponseTimePT(struct task *table, int tablesize,
				struct task *rtask);

bool RMIsTaskSched(struct task *table, int numEntry, bool *checkPass);
#endif /* __COMMON_H__ */
