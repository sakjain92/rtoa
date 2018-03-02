#include <stdio.h>
#include "common.h"
#include <assert.h>

static struct task *OverloadPeriodTransformTasks(struct task *table, int *tablesize)
{
	int i, j;

	/* Sort table with higher criticality tasks at top */
	qsort(table, (size_t)*tablesize, sizeof(struct task), criticalitySort);

	/* Look at tasks in increasing order of criticality */
	for (i = *tablesize - 1; i >= 0; i--) {

		int n;
		struct task *rtask = &table[i];
		double min_period = (double)LONG_MAX;

		/* Shouldn't be period transformed already */
		assert(table[i].n == 1);
		assert(table[i].overload_task == NULL &&
				table[i].is_overload_task == false);

		for (j = i + 1; j < *tablesize; j++) {
			double normPeriod = normTaskPeriod(&table[j]);
			min_period = (normPeriod < min_period) ?
					normPeriod : min_period;
		}

		rtask->n = n = (int)(min_period < rtask->period_ns ?
			ceill(rtask->period_ns / min_period) : 1);

		if ((n != 1) && (rtask->nominal_exectime_ns != rtask->exectime_ns)) {
			struct task *ov_task;

			ov_task = rtask->overload_task = malloc(sizeof(struct task));
			assert(ov_task);

			copyTask(ov_task, rtask);

			ov_task->n = 1;
			ov_task->nominal_exectime_ns =
				ov_task->exectime_ns =
				rtask->exectime_ns - rtask->nominal_exectime_ns;
			ov_task->is_overload_task = true;
			ov_task->overload_task = NULL;

			rtask->exectime_ns = rtask->nominal_exectime_ns;
		}
	}

	return table;
}

static bool admitAllOPTTask(struct task *table, int tablesize)
{
	int i;

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (getResponseTimePT(table, tablesize, rtask) < 0)
			return false;
	}

	return true;
}


/* Does a neccesary check on all tasks */
static bool checkOPTTable(struct task *table, int numEntry)
{
	int i;

	/* Check if scheduable in last period of PT with overload */
	for (i = 0; i < numEntry; i++) {
		struct task *rtask = &table[i];
		if (rtask->overload_task == NULL)
			continue;
		if (normTaskOComp(rtask->overload_task) + normTaskOComp(rtask) >
				normTaskPeriod(rtask))
			return false;
	}

	return true;
}

static void OPTTableCleanup(struct task *table, int numEntry)
{
	int i;

	for (i = 0; i < numEntry; i++) {
		if (table[i].overload_task != NULL) {
			free(table[i].overload_task);
			table[i].overload_task = NULL;
		}
	}
}
/* Checks if OPT can schedule task */
bool OPTIsTaskSched(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	*checkPass = true;

	assert(checkTable(table, numEntry));

	table = OverloadPeriodTransformTasks(table, &numEntry);

	assert(checkTable(table, numEntry));

	if (checkOPTTable(table, numEntry) == false) {
		*checkPass = false;
		ret = false;
		goto err;
	}

	ret = admitAllOPTTask(table, numEntry);

err:
	OPTTableCleanup(table, numEntry);
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

	table = OverloadPeriodTransformTasks(table, &numEntry);
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
				getOPTTaskResponseTime(table, numEntry, rtask));

		if (rtask->overload_task != NULL) {

			assert(rtask->overload_task->n == 1);

			printf("C:%f,\t C':%f,\t T:%f,\t Crit:%f,\t Scheduability:--\n",
					normTaskNComp(rtask->overload_task),
					normTaskOComp(rtask->overload_task),
					normTaskPeriod(rtask->overload_task),
					rtask->overload_task->criticality;
		}
	}

	printf("OPT admit all: %d\n", admitAllOPTTask(table, numEntry));

	printf("Check: %s\n", checkOPTTable(table, numEntry) == true ? "PASS" : "FAIL");

	OPTTableCleanup(table, numEntry);

	free(table);
	return 0;
}

#endif /* DEBUG */
