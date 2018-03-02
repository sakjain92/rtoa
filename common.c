#include "common.h"
#include <stdio.h>

/*
 * TODO: Double can have rounding errors which can be significant as we use
 * floor() and ceil() functions
 */

/*
 * TODO: Check the tie breaking in RMS if same periods
 */

double normTaskPeriod(const struct task *rtask)
{
	/* Not period transforming the overload task */
	assert(rtask->is_overload_task == false || rtask->n == 1);
	return ((double)rtask->period_ns) / rtask->n;
}

double normTaskNComp(const struct task *rtask)
{
	assert(rtask->is_overload_task == false || rtask->n == 1);
	return ((double)rtask->nominal_exectime_ns) / rtask->n;
}

double normTaskOComp(const struct task *rtask)
{
	assert(rtask->is_overload_task == false || rtask->n == 1);
	return ((double)rtask->exectime_ns) / rtask->n;
}

/*
 * Is other task higher prior than ref task
 * Currently we can't have two tasks with same criticality and same priority
 * Because we don't know how to break ties in that case
 */
static bool isHigherPrio(struct task *rtask, struct task *other)
{
	double rtask_period = normTaskPeriod(rtask);
	double other_period = normTaskPeriod(other);

	assert(!((rtask_period == other_period) &&
				(rtask->criticality == other->criticality)));

	if (rtask_period > other_period)
		return true;

	if (rtask_period == other_period) {
		if (rtask->criticality < other->criticality)
			return true;
	}

	return false;
}

/* Is other task higher criticality than ref task */
static bool isHigherCrit(struct task *rtask, struct task *other)
{
	double rtask_period = normTaskPeriod(rtask);
	double other_period = normTaskPeriod(other);

	assert(!((rtask_period == other_period) &&
			(rtask->criticality == other->criticality)));

	if (rtask->criticality < other->criticality) {
		return true;
	} else if (rtask->criticality > other->criticality) {
		return false;
	} else if (rtask_period > other_period) {
		return true;
	}

	return false;
}


int getNextInSet(struct task *table, int *sidx, int tablesize,
		struct task *rtask, bool (*inSet)(struct task *r, struct task *o))
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

void initializeTask(struct task *rtask)
{
	rtask->n = 1;
	rtask->nominal_exectime_ns =
		rtask->exectime_ns = rtask->period_ns = rtask->criticality = -1;
	rtask->overload_task = NULL;
	rtask->is_overload_task = false;
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

	table = malloc(sizeof(struct task) * (size_t)numEntry);
	if (table == NULL)
		return NULL;

	bzero(table, sizeof(struct task) * (size_t)numEntry);

	/* Order is C,Co,T,Criticality */
	argv++;
	for (i = 0; i < numEntry; i++) {

		const char *s = argv[i];

		initializeTask(&table[i]);

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
				table[i].nominal_exectime_ns = (double)num;
				break;
			case 2:
				table[i].exectime_ns = (double)num;
				break;
			case 3:
				table[i].period_ns = (double)num;
				break;
			case 4:
				table[i].criticality = (double)num;
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

double getResponseTimeRM(struct task *table, int tablesize,
						struct task *rtask)
{
	int firsttime=1;
	double resp=0L;
	double prevResp=0L;
	int numArrivals=0l;
	int idx=0;
	int selectedIdx=-1;

	/* Use this function for simple cases only */
	assert(rtask->overload_task == NULL);
	assert(rtask->n == 1);

	resp = rtask->exectime_ns;

	while (firsttime || (resp > prevResp && resp <= rtask->period_ns)){

		firsttime=0;
		prevResp = resp;
		resp = rtask->exectime_ns;

    		// get interference from Higher Priority
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrio)) >=0) {
			numArrivals = (int)ceill(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * table[selectedIdx].nominal_exectime_ns;
    		}
	}

	if (resp > rtask->period_ns)
		return -1;

  	return resp;
}

double getResponseTimeCAPA(struct task *table, int tablesize,
				struct task *rtask)
{
	int firsttime=1;
	double resp=0L;
	double prevResp=0L;
	int numArrivals=0l;
	int idx=0;
	int selectedIdx=-1;

	/* Use this function for simple cases only */
	assert(rtask->overload_task == NULL);
	assert(rtask->n == 1);

	resp = rtask->exectime_ns;

	while (firsttime || (resp > prevResp && resp <= rtask->period_ns)){

		firsttime=0;
		prevResp = resp;
		resp = rtask->exectime_ns;

    		// get interference from Higher Criticality
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherCrit)) >=0) {
			numArrivals = (int)ceill(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * table[selectedIdx].nominal_exectime_ns;
    		}
	}

	if (resp > rtask->period_ns)
		return -1;

  	return resp;
}

/*
 * TODO: There is another way to schedule tasks in case of OPT.
 * We here are placing a contraint of precendence. We can not have that precendence
 * but in that case all tasks will might face blocking from overload task
 */
double getResponseTimePT(struct task *table, int tablesize,
				struct task *rtask)
{
	int firsttime=1;
	double resp=0L;
	double prevResp=0L;
	int numArrivals=0l;
	int idx=0;
	int selectedIdx=-1;

	resp = normTaskOComp(rtask) + ((rtask->overload_task == NULL) ? 0 : normTaskOComp(rtask->overload_task));

	while (firsttime || (resp > prevResp && resp <= rtask->period_ns)){

		firsttime=0;
		prevResp = resp;

		resp = normTaskOComp(rtask) + ((rtask->overload_task == NULL) ? 0 : normTaskOComp(rtask->overload_task));

    		// get interference from Higher Priority
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrio)) >=0) {

			int fullPeriods;
			double remainderTime;
			double last_period_exec;
			double orig_period = table[selectedIdx].period_ns;
			double orig_nominal_exectime = table[selectedIdx].nominal_exectime_ns;
			int n = table[selectedIdx].n;

			/* Higher priority wouldn't overload but might run at higher period */
			fullPeriods = (int)floorl(prevResp / orig_period);
			resp += fullPeriods * orig_nominal_exectime;

			remainderTime = prevResp - fullPeriods * orig_period;

			/* How many PT task comes in the remaining time? */
			numArrivals = (int)ceill((remainderTime * n) / orig_period);
			last_period_exec = numArrivals * normTaskOComp(&table[selectedIdx]);
			resp += last_period_exec > orig_nominal_exectime ?
				orig_nominal_exectime : last_period_exec;
		}
	}

	if (resp > normTaskPeriod(rtask))
		return -1;

	return resp;
}


/*
 * Used by qsort to sort by criticality in decreasing order
 * Higher criticality is represented by higher number
 */
int criticalitySort(const void *a, const void *b)
{
	const struct task *rtask = (const struct task *)a;
	const struct task *o = (const struct task *)b;

	double diff = o->criticality - rtask->criticality;
	if (diff < 0)
		return -1;
	else if (diff > 0)
		return 1;

	return 0;
}

/*
 * Used by qsort to sort by priority in increasing order
 * Higher priority is denoted by lower period
 */
int incPrioritySort(const void *a, const void *b)
{
	const struct task *rtask = (const struct task *)a;
	const struct task *o = (const struct task *)b;

	if (normTaskPeriod(o) > normTaskPeriod(rtask)) {
		return 1;
	} else if (normTaskPeriod(o) == normTaskPeriod(rtask)) {
		return 0;
	}

	return -1;
}

/*
 * Copies entry from one task to another
 */
void copyTask(struct task *dest, struct task *src)
{
	memcpy(dest, src, sizeof(struct task));
}

/* Checks properties of task */
static bool checkProp(struct task *rtask)
{
	bool ret;

	if (rtask->n < 1)
		return false;

	if (rtask->nominal_exectime_ns <= 0)
		return false;

	if (rtask->exectime_ns < rtask->nominal_exectime_ns)
		return false;

	if (rtask->period_ns < rtask->exectime_ns)
		return false;

	if (rtask->is_overload_task == true && rtask->overload_task != NULL)
		return false;

	if (rtask->criticality <= 0)
		return false;

	if (rtask->overload_task != NULL) {
		ret = checkProp(rtask->overload_task);
		if (ret == false)
			return false;
	}

	return true;
}

/* Checks basic property of the entries in the table */
bool checkTable(struct task *table, int tablesize)
{
	int i;
	bool ret;

	if (tablesize <= 0)
		return false;

	for (i = 0; i < tablesize; i++) {
		struct task *rtask = &table[i];

		ret = checkProp(rtask);
		if (ret == false)
			return false;
	}

	return true;
}

