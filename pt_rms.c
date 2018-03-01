#include <stdio.h>
#include "common.h"
#include <assert.h>

//#define DEBUG

struct task *PeriodTransformTasks(struct task *table, int *tablesize)
{
	int i, j;

	/* Sort table with higher criticality tasks at top */
	qsort(table, *tablesize, sizeof(struct task), criticalitySort);

	for (i = *tablesize - 1; i >= 0; i--) {

		int n;
		struct task *rtask = &table[i];
		double min_period = LONG_MAX;

		for (j = i + 1; j < *tablesize; j++) {
			min_period = (table[j].period_ns < min_period) ?
					table[j].period_ns : min_period;
		}

		n = min_period < rtask->period_ns ?
			ceilf(rtask->period_ns / min_period) :
			1;

		rtask->orig_nominal_exectime_ns = rtask->nominal_exectime_ns;
		rtask->orig_period_ns = rtask->period_ns;

		rtask->nominal_exectime_ns =
			rtask->exectime_ns =
				rtask->exectime_ns / n;
		rtask->period_ns = rtask->period_ns / n;
	}

	return table;
}

/* Naive because not all of the cycles are needed to be ran if not overloading */
double getPTTaskNaiveResponseTime(struct task *table, int tablesize, struct task *rtask)
{
	return getResponseTimeRM(table, tablesize, rtask);
}

bool admitAllPTTaskNaive(struct task *table, int tablesize)
{
	int i;

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (getPTTaskNaiveResponseTime(table, tablesize, rtask) < 0)
			return false;
	}

	return true;
}


/* Checks if OPT can schedule task (naively test) */
bool PTIsTaskSchedNaive(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	/* No checks */
	*checkPass = true;

	assert(checkRMTable(table, numEntry));

	table = PeriodTransformTasks(table, &numEntry);
	assert(table);

	assert(checkRMTable(table, numEntry));

	ret = admitAllPTTaskNaive(table, numEntry);

	return ret;
}

double getPTTaskResponseTime(struct task *table, int tablesize, struct task *rtask)
{
	return getResponseTimePT(table, tablesize, rtask);
}

bool admitAllPTTask(struct task *table, int tablesize)
{
	int i;

	qsort(table, tablesize, sizeof(struct task), criticalitySort);

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (getPTTaskResponseTime(table, tablesize, rtask) < 0)
			return false;
	}

	return true;
}


/* Checks if OPT can schedule task (naively test) */
bool PTIsTaskSched(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	/* No checks */
	*checkPass = true;

	assert(checkRMTable(table, numEntry));

	table = PeriodTransformTasks(table, &numEntry);
	assert(table);

	assert(checkRMTable(table, numEntry));

	ret = admitAllPTTask(table, numEntry);

	return ret;
}

#ifdef DEBUG

/* Usage: ./rms.elf <C,Co,T,Criticality> ... */
int main(int argc, char **argv)
{
	int i, numEntry;

	struct task *table = parseArgs(argc, argv, &numEntry);
	assert(table);

	assert(checkRMTable(table, numEntry));

	table = PeriodTransformTasks(table, &numEntry);
	assert(table);

	assert(checkRMTable(table, numEntry));

	/* Sort table with higher criticality tasks at top */
	qsort(table, numEntry, sizeof(struct task), criticalitySort);

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%f,\t C':%f,\t T:%f,\t Crit:%f,\t Scheduability:%f\n",
				rtask->nominal_exectime_ns,
				rtask->exectime_ns,
				rtask->period_ns,
				rtask->criticality,
				getPTTaskResponseTime(table, numEntry, rtask));
	}

	printf("OPT admit all: %d\n", admitAllPTTask(table, numEntry));

	free(table);
	return 0;
}

#endif /* DEBUG */
