#include "common.h"
#include <stdio.h>

int isHigherPrio(struct task *rtask, struct task *other)
{
	if (rtask->period_ns > other->period_ns)
		return true;
	if (rtask->period_ns == other->period_ns) {
		assert(rtask->criticality != other->criticality);
		if (rtask->criticality < other->criticality)
			return true;
	}

	return false;
}

int isHigherCrit(struct task *rtask, struct task *other)
{
	assert(!((rtask->period_ns == other->period_ns) &&
			(rtask->criticality == other->criticality)));

	if (rtask->criticality < other->criticality) {
		return true;
	} else if (rtask->criticality > other->criticality) {
		return false;
	} else if (rtask->period_ns > other->period_ns) {
		return true;
	}

	return false;
}


int getNextInSet(struct task *table, int *sidx, int tablesize,
		struct task *rtask, int (*inSet)(struct task *r, struct task *o))
{
	int selectedIdx=-1;

	if (*sidx < 0)
		return -1;

	if (inSet != NULL) {
		while ((*sidx < tablesize) &&
			(rtask ==  &table[*sidx] ||
			 !inSet(rtask, &table[*sidx]))){
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
			errno = 0;
			num = strtol(buf, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				goto err;
			}

			if (errno == EINVAL)
				goto err;


			switch(field) {

			case 1:
				table[i].nominal_exectime_ns = (float)num;
				break;
			case 2:
				table[i].exectime_ns = (float)num;
				break;
			case 3:
				table[i].period_ns = (float)num;
				break;
			case 4:
				table[i].criticality = (float)num;
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

float getResponseTimeRM(struct task *table, int tablesize,
						struct task *rtask)
{
	int firsttime=1;
	float resp=0L;
	float prevResp=0L;
	int numArrivals=0l;
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
			numArrivals = ceilf(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * table[selectedIdx].nominal_exectime_ns;
    		}
	}

	if (resp > rtask->period_ns)
		return -1;

  	return resp;
}

float getResponseTimeCAPA(struct task *table, int tablesize,
				struct task *rtask)
{
	int firsttime=1;
	float resp=0L;
	float prevResp=0L;
	int numArrivals=0l;
	int idx=0;
	int selectedIdx=-1;

	resp = rtask->exectime_ns;

	while (firsttime || (resp > prevResp && resp <= rtask->period_ns)){

		firsttime=0;
		prevResp = resp;
		resp = rtask->exectime_ns;

    		// get interference from Higher Criticality
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherCrit)) >=0) {
			numArrivals = ceilf(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * table[selectedIdx].nominal_exectime_ns;
    		}
	}

	if (resp > rtask->period_ns)
		return -1;

  	return resp;
}

float getResponseTimePT(struct task *table, int tablesize,
				struct task *rtask)
{
	int firsttime=1;
	float resp=0L;
	float prevResp=0L;
	int numArrivals=0l;
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
			/* Higher priority wouldn't overload but might run at higher period */
			float n = table[selectedIdx].orig_period_ns / table[selectedIdx].period_ns;

			int fullPeriods = floorf(prevResp / table[selectedIdx].period_ns);
			resp += fullPeriods * (table[selectedIdx].orig_nominal_exectime_ns / n);

			int remainderTime = prevResp - fullPeriods * table[selectedIdx].period_ns;

			/* XXX: Is this higher than neccesary? */
			numArrivals = ceilf(remainderTime / table[selectedIdx].period_ns);
			int add = numArrivals * table[selectedIdx].nominal_exectime_ns;
			resp += add > (table[selectedIdx].orig_nominal_exectime_ns / n)?
				(table[selectedIdx].orig_nominal_exectime_ns / n): add;
		}
	}

	if (resp > rtask->period_ns)
		return -1;

  	return resp;
}


/*
 * Used by qsort to sort by criticality in decreasing order
 * Higher criticality is represented by higher number
 */
int criticalitySort(const void *a, const void *b)
{
	struct task *rtask = (struct task *)a;
	struct task *o = (struct task *)b;

	float diff = o->criticality - rtask->criticality;
	if (diff < 0)
		return -1;
	else if (diff > 0)
		return 1;

	return 0;
}

/*
 * Copies entry from one task to another
 */
void copyTask(struct task *dest, struct task *src)
{
	memcpy(dest, src, sizeof(struct task));
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

		if (rtask->exectime_ns < rtask->nominal_exectime_ns)
			return false;

		if (rtask->period_ns < rtask->exectime_ns)
			return false;
	}

	return true;
}

