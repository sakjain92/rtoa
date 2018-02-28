#include <stdio.h>
#include "common.h"
#include <time.h>
#include <stdlib.h>

#define NUM_TASK 2

#define MAX_PERIOD	100

struct expFunc {
	char *name;
	bool (*func)(struct task *table, int tablesize, bool *checkPass);
	bool printOnFail;
	bool printOnCheckFail;
	bool result;
	bool checkPass;
};

struct expFunc funcList[] =
{
	{
		.name = "OPTSched",
		.func = OPTIsTaskSched,
		.printOnFail = true,
		.printOnCheckFail = false,
	},
	{
		.name = "PTSchedNavie",
		.func = PTIsTaskSchedNaive,
		.printOnFail = false,
		.printOnCheckFail = false,
	}
};

void printTaskset(struct task *table, int numEntry)
{
	int i;
	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		printf("C:%f,\t C':%f,\t T:%f,\t Crit:%f\n",
			rtask->nominal_exectime_ns,
			rtask->exectime_ns,
			rtask->period_ns,
			rtask->criticality);
	}

}

void compare(struct task *table, int tablesize, float *criticality)
{
	int i;
	struct task *newtable = malloc(sizeof(struct task) * tablesize);
	assert(newtable);
	bool pass = false;
	int fsize = sizeof(funcList)/ sizeof(funcList[0]);
	bool printReq = false;

	for (i = 0; i < tablesize; i++) {
		table[i].criticality = criticality[i];
	}

	for (i = 0; i < fsize; i++) {
		struct expFunc *ef = &funcList[i];

		memcpy(newtable, table, sizeof(struct task) * tablesize);

		ef->result = ef->func(newtable, tablesize, &ef->checkPass);

		if (ef->result == true) {
			pass = true;
		}

		if (ef->result == false && ef->printOnFail == true) {
			if (!(ef->printOnCheckFail == false && ef->checkPass == false)) {
				printReq = true;

			}
		}
	}

	/* If atleast someone passed but a function that req printing on fail didn't pass */
	if (pass && printReq) {
		printf("-------------------------------------------------------\n");
		printTaskset(table, tablesize);

		for (i = 0; i < fsize; i++) {
			struct expFunc *ef = &funcList[i];
			if (ef->result == false && ef->printOnFail == true) {
				if (!(ef->printOnCheckFail == false && ef->checkPass == false))
					printf("%s failed to sched taskset\n", ef->name);
			}
		}
	}

}

void swap(float *a, float *b)
{
	float temp = *a;
	*a = *b;
	*b = temp;
}

void permute(float *array, int i, int length,
		void (*func)(struct task *table, int tablesize, float *criticality),
		struct task *table) {

	if (length == i){
		func(table, length, array);
		return;
	}

	int j = i;
	for (j = i; j < length; j++) {
		swap(array+i,array+j);
		permute(array,i + 1, length, func, table);
		swap(array+i,array+j);
	}
	return;
}

int main()
{
	float criticality[NUM_TASK];
	int i;

	srand(time(NULL));

	struct task table[NUM_TASK];

	for (i = 0; i < NUM_TASK; i++) {
		criticality[i] = i+1;
	}

	while (1) {

		memset(table, 0, sizeof(struct task) * NUM_TASK);

		for (i = 0; i < NUM_TASK; i++) {
			struct task *rtask = &table[i];
			int period, exec;
			rtask->period_ns = period = (rand() % MAX_PERIOD) + 1;
			rtask->exectime_ns = exec = (rand() % period) + 1;
			rtask->nominal_exectime_ns = (float)(rand() % exec) + 1;
		}

		permute(criticality, 0, NUM_TASK, compare, table);
	}

	return 0;
}
