#include <stdio.h>
#include "common.h"
#include <assert.h>

//#define DEBUG

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
		printf("C:%zu,\t T:%zu,\t ResponseTime:%zd\n",
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

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%zu,\t T:%zu,\t ResponseTime:%zd\n",
				rtask->nominal_exectime_ns,
				rtask->period_ns,
				getResponseTimeRM(table, numEntry, rtask));
	}

	return 1;
}
