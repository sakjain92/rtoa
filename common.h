#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

/* Defines a task */
struct task {

	/* Used in PT/OPT/VOPT - By how much is the period transformed */
	int n;

	/* Overload execution time */
	double exectime_ns;
	double nominal_exectime_ns;
	double exectime_in_rm_ns;
	double period_ns;
	double criticality;

	/*
	 *Used in OPT - Signifies the task that runs only when overload
	 * Because in OPT a task is split into two parts:
	 * Recurring and overload task
	 */
	struct task *overload_task;

	bool is_overload_task;
};

double normTaskPeriod(struct task *rtask);
double normTaskNComp(struct task *rtask);
double normTaskOComp(struct task *rtask);


int getNextInSet(struct task *table, int *sidx, int tablesize,
		struct task *rtask, bool (*inSet)(struct task *r, struct task *o));
struct task *parseArgs(int argc, char **argv, int *tablesize);
int criticalitySort(const void *a, const void *b);
void copyTask(struct task *dest, struct task *src);
bool checkTable(struct task *table, int tablesize);
void initializeTask(struct task *rtask);


double getResponseTimeRM(struct task *table, int tablesize,
						struct task *rtask);
double getResponseTimeCAPA(struct task *table, int tablesize, struct task *rtask);
double getResponseTimePT(struct task *table, int tablesize,
				struct task *rtask);


bool RMIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool PTIsTaskSchedNaive(struct task *table, int numEntry, bool *checkPass);
bool CAPAIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool PTIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool ZSRMSIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool OPTIsTaskSched(struct task *table, int numEntry, bool *checkPass);
bool VOPTIsTaskSched(struct task *table, int numEntry, bool *checkPass);
#endif /* __COMMON_H__ */
