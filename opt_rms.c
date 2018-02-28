#include <stdio.h>
#include "common.h"
#include <assert.h>

//#define DEBUG


struct task *OverloadPeriodTransformTasks(struct task *table, int *tablesize)
{
	int i, j;
	struct task *newtable = NULL;
	int numEntry = 0;
	struct task *overload_task;

	newtable = malloc(sizeof(struct task) * 2 * *tablesize);
	assert(newtable);

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
	return newtable;
}


/*
 * TODO: There is another way to schedule tasks.
 * We here are placing a contraint of precendence. We can not have that precendence
 * but in that case all tasks will might face blocking from overload task
 */
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

	if (rtask->overload_task) {
		copyTask(&newtable[numEntry], rtask->overload_task);
		/*
		 * Increase just a bit the criticality of original.
		 * Make the period of overload task same as parent task
		 */
		newtable[numEntry - 1].criticality += 0.1;
		newtable[numEntry].period_ns = newtable[numEntry - 1].period_ns;
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
