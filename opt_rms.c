#include <stdio.h>
#include "common.h"
#include <assert.h>


struct task *OverloadPeriodTransformTasks(struct task *table, int *tablesize)
{
	int i, j, k;
	struct task *newtable = NULL;
	int numEntry = 0;
	struct task *overload_task;

	newtable = malloc(sizeof(struct task) * 2 * *tablesize);
	assert(newtable);

	/* Sort table with higher criticality tasks at top */
	qsort(table, *tablesize, sizeof(struct task), criticalitySort);

	for (i = 0, numEntry = *tablesize; i < *tablesize; i++) {

		struct task *rtask = &table[i];
		if (rtask->nominal_exectime_ns != rtask->exectime_ns)
			numEntry++;
	}

	for (i = *tablesize - 1, k = numEntry - 1; i >= 0; i--) {

		int n;
		struct task *rtask = &table[i];
		double min_period = LONG_MAX;

		for (j = k + 1; j < numEntry; j++) {
			min_period = (newtable[j].period_ns < min_period) ?
					newtable[j].period_ns : min_period;
		}

		n = min_period < rtask->period_ns ?
			ceilf(rtask->period_ns / min_period) :
			1;

		overload_task = NULL;
		if (rtask->nominal_exectime_ns != rtask->exectime_ns) {

			copyTask(&newtable[k], rtask);

			newtable[k].nominal_exectime_ns =
				newtable[k].exectime_ns =
				rtask->exectime_ns - rtask->nominal_exectime_ns;

			newtable[k].isOverloadTask = true;
			newtable[k].overload_task = NULL;
			overload_task = &newtable[k];
			k--;
		}

		copyTask(&newtable[k], rtask);
		newtable[k].nominal_exectime_ns =
			newtable[k].exectime_ns =
			rtask->nominal_exectime_ns / n;
		newtable[k].period_ns = rtask->period_ns / n;
		newtable[k].overload_task = overload_task;
		newtable[k].isOverloadTask = false;
		k--;
	}

	assert(k == -1);

	*tablesize = numEntry;
	return newtable;
}


/*
 * TODO: There is another way to schedule tasks.
 * We here are placing a contraint of precendence. We can not have that precendence
 * but in that case all tasks will might face blocking from overload task
 */
double getOPTTaskResponseTime(struct task *table, int tablesize, struct task *rtask)
{
	/*
	 * Need to consider the task (in overload situation) and all higher
	 * criticality task (in non overloaded state)
	 */
	struct task *newtable = NULL;
	int numEntry = 0;
	int i;
	double ret;
	struct task *parent;

	newtable = malloc(sizeof(struct task) * tablesize);
	assert(newtable);
	assert(!rtask->isOverloadTask);

	for (i = 0, numEntry = 0; &table[i] != rtask; i++) {
		if (table[i].isOverloadTask)
			continue;
		copyTask(&newtable[numEntry], &table[i]);
		assert(table[i].criticality > rtask->criticality);
		numEntry++;
	}

	/* Checking if in sorted order according to the criticality */
	for (; i < tablesize; i++) {
		assert(table[i].criticality <= rtask->criticality);
	}

	/* Incase of parent task, we check if the overload task is scheduable */
	copyTask(&newtable[numEntry++], rtask);

	if (rtask->overload_task) {
		parent = &newtable[numEntry - 1];
		copyTask(&newtable[numEntry], rtask->overload_task);
		/*
		 * Increase just a bit the criticality of original.
		 * Make the period of overload task same as parent task
		 */
		parent->criticality += 0.1;
		newtable[numEntry].period_ns = parent->period_ns;
		numEntry++;
	}

	ret = getResponseTimeRM(newtable, numEntry, &newtable[numEntry - 1]);
	free(newtable);

	if (ret > rtask->period_ns)
		ret = -1;

	return ret;
}

bool admitAllOPTTask(struct task *table, int tablesize)
{
	int i;

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (rtask->isOverloadTask)
			continue;
		if (getOPTTaskResponseTime(table, tablesize, rtask) < 0)
			return false;
	}

	return true;
}


/* Does a neccesary check on all tasks */
bool checkOPTTable(struct task *table, int numEntry)
{
	int i;

	for (i = 0; i < numEntry; i++) {
		struct task *rtask = &table[i];
		if (!rtask->overload_task)
			continue;
		if (rtask->overload_task->nominal_exectime_ns + rtask->nominal_exectime_ns >
				rtask->period_ns)
			return false;
	}

	return true;
}

/* Checks if OPT can schedule task */
bool OPTIsTaskSched(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	*checkPass = true;

	assert(checkRMTable(table, numEntry));

	table = OverloadPeriodTransformTasks(table, &numEntry);
	if (table == NULL) {
		printf("Error in conversion\n");
		return -1;
	}

	assert(checkRMTable(table, numEntry));

	if (checkOPTTable(table, numEntry) == false) {
		*checkPass = false;
		return false;
	}

	ret = admitAllOPTTask(table, numEntry);

	return ret;
}

/* Trying all permutations of variations */
bool VOPTIsTaskSched(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	*checkPass = true;

	assert(checkRMTable(table, numEntry));

	table = OverloadPeriodTransformTasks(table, &numEntry);
	if (table == NULL) {
		printf("Error in conversion\n");
		return -1;
	}

	assert(checkRMTable(table, numEntry));

	if (checkOPTTable(table, numEntry) == false) {
		*checkPass = false;
		return false;
	}

	ret = admitAllOPTTask(table, numEntry);

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

	table = OverloadPeriodTransformTasks(table, &numEntry);
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
				rtask->isOverloadTask ? 0 : getOPTTaskResponseTime(table, numEntry, rtask));
	}

	printf("OPT admit all: %d\n", admitAllOPTTask(table, numEntry));

	free(table);
	return 0;
}

#endif /* DEBUG */
