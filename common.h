#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

struct task {

	size_t exectime_ns;
	size_t nominal_exectime_ns;
	size_t exectime_in_rm_ns;
	size_t period_ns;
	size_t criticality;
};

int isHigherPrio(struct task *rtask, struct task *other)
{
  return (rtask->period_ns >= other->period_ns);
}


int getNextInSet(struct task *table, int *sidx, int tablesize,
		struct task *rtask, int (*inSet)(struct task *r, struct task *o))
{
	int selectedIdx=-1;

	if (*sidx < 0)
		return -1;

	if (inSet != NULL) {
		while ((*sidx < tablesize) &&
			(!inSet(rtask, &table[*sidx]) ||
	  		rtask ==  &table[*sidx])){
			*sidx +=1;
		}
	} else {
		while ((*sidx < tablesize) &&
	  		rtask ==  &table[*sidx]){
			*sidx +=1;
		}
	}

	if (*sidx < tablesize){
		selectedIdx = *sidx;
		*sidx+=1;
	}

	return selectedIdx;
}

bool checkRMTable(struct task *table, int tablesize)
{
	int i;
	if (tablesize <= 0)
		return false;

	for (i = 0; i < tablesize; i++) {
		struct task *rtask = &table[i];

		if (rtask->nominal_exectime_ns <= 0)
			return false;

		if (rtask->exectime_ns != rtask->nominal_exectime_ns)
			return false;

		if (rtask->period_ns < rtask->exectime_ns)
			return false;
	}

	return true;
}

ssize_t getResponseTimeRM(struct task *table, int tablesize,
						struct task *rtask)
{
	int firsttime=1;
	size_t resp=0L;
	size_t prevResp=0L;
	size_t numArrivals=0l;
	int idx=0;
	int selectedIdx=-1;

	resp = rtask->exectime_ns;

	while (firsttime || (resp > prevResp && resp <= rtask->period_ns)){

		firsttime=0;
		prevResp = resp;
		resp = rtask->exectime_ns;

    		// get interference from Higher Priority
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrio)) >=0) {
			numArrivals = (prevResp +  table[selectedIdx].period_ns  - 1) / table[selectedIdx].period_ns;
			resp += numArrivals * table[selectedIdx].nominal_exectime_ns;
    		}
	}

	if (resp > rtask->period_ns)
		return -1;

  	return resp;
}

bool admitRMTask(struct task *table, int tablesize, struct task *rtask)
{
	if (getResponseTimeRM(table, tablesize, rtask) < 0)
		return false;

	return true;
}

struct task *parseArgs(int argc, char **argv, int *tablesize)
{
	struct task *table;
	int numEntry = argc - 1;
	const char delims[] = ",";
	int i, field;
	ssize_t num;
	char buf[100];

	if (numEntry < 1)
		return NULL;

	table = malloc(sizeof(struct task) * numEntry);
	if (table == NULL)
		return NULL;

	bzero(table, sizeof(struct task) * numEntry);

	/* Order is C,Co,T,Criticality */
	argv++;
	for (i = 0; i < numEntry; i++) {
		const char *s = argv[i];
		field = 0;
		do {
			size_t field_len = strcspn(s, delims);
			snprintf(buf, sizeof(buf), "%.*s", (int)field_len, s);
			s += field_len;
			field++;
			num = strtol(buf, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				goto err;
			}

			switch(field) {

			case 1:
				table[i].nominal_exectime_ns = num;
				break;
			case 2:
				table[i].exectime_ns = num;
				break;
			case 3:
				table[i].period_ns = num;
				break;
			case 4:
				table[i].criticality = num;
				break;
			default:
				goto err;
			}

		} while (*s++);

		if (field != 4)
			goto err;
	}

	*tablesize = numEntry;
	return table;
err:
	if (table)
		free(table);
	return NULL;
}
#endif /* __COMMON_H__ */
