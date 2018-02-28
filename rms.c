#include <stdio.h>
#include "common.h"
#include <assert.h>

//#define DEBUG

bool checkRMTable(struct task *table, int tablesize)
{
	int i;
	if (tablesize <= 0)
		return false;

	for (i = 0; i < tablesize; i++) {
		struct task *rtask = &table[i];

		if (rtask->nominal_exectime_ns <= 0)
			return false;

		if (rtask->exectime_ns < rtask->nominal_exectime_ns)
			return false;

		if (rtask->period_ns < rtask->exectime_ns)
			return false;
	}

	return true;
}

struct task *OverloadPeriodTransformTasks(struct task *table, int *tablesize)
{
	int i, j;
	struct task *newtable = NULL;
	int numEntry = 0;
	struct task *overload_task;

	newtable = malloc(sizeof(struct task) * 2 * *tablesize);
	if (newtable == NULL)
		goto err;

	/* Sort table with higher criticality tasks at top */
	qsort(table, *tablesize, sizeof(struct task), criticalitySort);

	for (i = 0, numEntry = 0; i < *tablesize; i++) {

		int n;
		struct task *rtask = &table[i];
		float min_period = LONG_MAX;

		for (j = i + 1; j < *tablesize; j++) {
			min_period = (table[j].period_ns < min_period) ?
					table[j].period_ns : min_period;
		}

		n = min_period < rtask->period_ns ?
			ceilf(rtask->period_ns / min_period) :
			1;

		if (n != 1) {
			overload_task = NULL;
			if (rtask->nominal_exectime_ns != rtask->exectime_ns) {

				copyTask(&newtable[numEntry], rtask);

				newtable[numEntry].nominal_exectime_ns =
					newtable[numEntry].exectime_ns =
					rtask->exectime_ns - rtask->nominal_exectime_ns;

				newtable[numEntry].isOverloadTask = true;
				overload_task = &newtable[numEntry];
				numEntry++;
			}

			copyTask(&newtable[numEntry], rtask);
			newtable[numEntry].nominal_exectime_ns =
				newtable[numEntry].exectime_ns =
				rtask->nominal_exectime_ns / n;
			newtable[numEntry].period_ns = rtask->period_ns / n;
			newtable[numEntry].overload_task = overload_task;
			numEntry++;

		} else {
			copyTask(&newtable[numEntry], rtask);
			numEntry++;
		}
	}

	*tablesize = numEntry;
	free(table);
	return newtable;
err:
	if (table)
		free(table);
	if (newtable)
		free(newtable);
	return NULL;
}



float getOPTTaskResponseTime(struct task *table, int tablesize, struct task *rtask)
{
	/*
	 * Need to consider the task (in overload situation) and all higher
	 * criticality task (in non overloaded state)
	 */
	struct task *newtable = NULL;
	int numEntry = 0;
	int i;
	float ret;

	newtable = malloc(sizeof(struct task) * tablesize);
	assert(newtable);
	assert(!rtask->isOverloadTask);

	for (i = 0, numEntry = 0; &table[i] != rtask; i++) {
		if (table[i].isOverloadTask)
			continue;
		copyTask(&newtable[numEntry], &table[i]);
		numEntry++;
	}

	/* Incase of parent task, we check if the overload task is scheduable */
	copyTask(&newtable[numEntry++], rtask);

	if (rtask->overload_task)
		copyTask(&newtable[numEntry++], rtask->overload_task);

	ret = getResponseTimeRM(newtable, numEntry, &newtable[numEntry - 1]);
	free(newtable);

	if (ret > rtask->period_ns)
		ret = -1;

	return ret;
}

bool admitAllOPTTask(struct task *table, int tablesize)
{
	int i;

	/* Sort table with higher criticality tasks at top */
	qsort(table, tablesize, sizeof(struct task), criticalitySort);

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (rtask->isOverloadTask)
			continue;
		if (getOPTTaskResponseTime(table, tablesize, rtask) < 0)
			return false;
	}

	return true;
}

#ifdef DEBUG

struct task table[] = {
	{
		.period_ns = 100L,
		.exectime_ns = 40L,
		.nominal_exectime_ns = 40L,
		.exectime_in_rm_ns = 0L,
		.criticality = 1
	},

	{
		.period_ns = 150L,
		.exectime_ns = 40L,
		.nominal_exectime_ns = 40L,
		.exectime_in_rm_ns = 0L,
		.criticality = 1
	},

	{
		.period_ns = 350L,
		.exectime_ns = 200L,
		.nominal_exectime_ns = 200L,
		.exectime_in_rm_ns = 0L,
		.criticality = 1
	},
};

int main()
{
	int i;
	int numEntry = sizeof(table)/sizeof(table[0]);
	assert(checkRMTable(table, numEntry));

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%f,\t T:%f,\t ResponseTime:%f\n",
				rtask->nominal_exectime_ns,
				rtask->period_ns,
				getResponseTimeRM(table, numEntry, rtask));
	}

	return 0;
}

#endif /* DEBUG */


/* Usage: ./rms.elf <C,Co,T,Criticality> ... */
int main(int argc, char **argv)
{
	int i, numEntry;

	struct task *table = parseArgs(argc, argv, &numEntry);
	if (table == NULL) {
		printf("Error in parsing args\n");
		return -1;
	}

	assert(checkRMTable(table, numEntry));

	table = OverloadPeriodTransformTasks(table, &numEntry);
	if (table == NULL) {
		printf("Error in conversion\n");
		return -1;
	}

	assert(checkRMTable(table, numEntry));

	/* Sort table with higher criticality tasks at top */
	qsort(table, numEntry, sizeof(struct task), criticalitySort);

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%f,\t C':%f,\t T:%f,\t Crit:%zd,\t Scheduability:%f\n",
				rtask->nominal_exectime_ns,
				rtask->exectime_ns,
				rtask->period_ns,
				rtask->criticality,
				rtask->isOverloadTask ? 0 : getOPTTaskResponseTime(table, numEntry, rtask));
	}

	printf("OPT admit all: %d\n", admitAllOPTTask(table, numEntry));

	free(table);
	return 0;
}
