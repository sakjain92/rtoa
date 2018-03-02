#include <stdio.h>
#include "common.h"
#include <assert.h>

static struct task *PeriodTransformTasks(struct task *table, int *tablesize)
{
	int i, j;

	/* Sort table with higher criticality tasks at top */
	qsort(table, (size_t)*tablesize, sizeof(struct task), criticalitySort);

	/* Look at tasks in increasing order of criticality */
	for (i = *tablesize - 1; i >= 0; i--) {

		struct task *rtask = &table[i];
		double min_period = (double)LONG_MAX;

		/* Shouldn't be period transformed already */
		assert(table[i].n == 1);

		for (j = i + 1; j < *tablesize; j++) {
			double normPeriod = normTaskPeriod(&table[j]);
			min_period = (normPeriod < min_period) ?
					normPeriod : min_period;
		}

		rtask->n = (int)(min_period < rtask->period_ns ?
			ceill(rtask->period_ns / min_period) : 1);
	}

	return table;
}

/* Naive because not all of the cycles are needed to be ran if not overloading */
static double getPTTaskNaiveResponseTime(struct task *table, int tablesize, struct task *rtask)
{
	return getResponseTimeRM(table, tablesize, rtask);
}

static bool admitAllPTTaskNaive(struct task *table, int tablesize)
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

	assert(checkTable(table, numEntry));

	table = PeriodTransformTasks(table, &numEntry);
	assert(table);

	assert(checkTable(table, numEntry));

	ret = admitAllPTTaskNaive(table, numEntry);

	return ret;
}

static double getPTTaskResponseTime(struct task *table, int tablesize, struct task *rtask)
{
	return getResponseTimePT(table, tablesize, rtask);
}

static bool admitAllPTTask(struct task *table, int tablesize)
{
	int i;

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

	assert(checkTable(table, numEntry));

	table = PeriodTransformTasks(table, &numEntry);
	assert(table);

	assert(checkTable(table, numEntry));

	ret = admitAllPTTask(table, numEntry);

	return ret;
}

#ifdef DEBUG

/* Usage: ./rms.elf <C,Co,T,Criticality> ... */
int main(int argc, char **argv)
{
	int i, numEntry;

	struct task *table = parseArgs(argc, argv, &numEntry);
	if (table == NULL) {
		printf("Error in parsing args\n");
		return -1;
	}

	assert(checkTable(table, numEntry));

	table = PeriodTransformTasks(table, &numEntry);
	assert(table);

	assert(checkTable(table, numEntry));

	/* Sort table with higher criticality tasks at top */
	qsort(table, numEntry, sizeof(struct task), criticalitySort);

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%f,\t C':%f,\t T:%f,\t Crit:%f,\t Scheduability:%f\n",
				normTaskNComp(rtask),
				normTaskOComp(rtask),
				normTaskPeriod(rtask),
				rtask->criticality,
				getPTTaskResponseTime(table, numEntry, rtask));
	}

	printf("OPT admit all: %d\n", admitAllPTTask(table, numEntry));

	free(table);
	return 0;
}

#endif /* DEBUG */
