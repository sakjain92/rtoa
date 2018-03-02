#include <stdio.h>
#include "common.h"
#include <assert.h>

static bool admitAllCAPATask(struct task *table, int tablesize)
{
	int i;

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (getResponseTimeCAPA(table, tablesize, rtask) < 0)
			return false;
	}

	return true;
}

/* Checks if OPT can schedule task */
bool CAPAIsTaskSched(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	*checkPass = true;

	assert(checkTable(table, numEntry));

	ret = admitAllCAPATask(table, numEntry);

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

	/* Sort table with higher criticality tasks at top */
	qsort(table, (size_t)numEntry, sizeof(struct task), criticalitySort);

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%f,\t C':%f,\t T:%f,\t Crit:%f,\t Scheduability:%f\n",
				rtask->nominal_exectime_ns,
				rtask->exectime_ns,
				rtask->period_ns,
				rtask->criticality,
				getResponseTimeCAPA(table, numEntry, rtask));
	}

	printf("CAPA admit all: %d\n", admitAllCAPATask(table, numEntry));

	free(table);
	return 0;
}

#endif /* DEBUG */
