#include "common.h"
#include <stdio.h>

static bool isHigherPrioHigherCrit(struct task *rtask, struct task *o) {
	return ((rtask->period_ns >= o->period_ns) &&
			(rtask->criticality < o->criticality));
}

static bool isLowerPrioHigherCrit(struct task *rtask, struct task *o) {
	return ((rtask->period_ns < o->period_ns) &&
		  (rtask->criticality < o->criticality));
}

static bool isHigherPrioSameCrit(struct task *rtask, struct task *o) {
	return ((rtask->period_ns >= o->period_ns) &&
		  (rtask->criticality == o->criticality));
}

static bool isHigherPrioLowerCrit(struct task *rtask, struct task *o) {
	return ((rtask->period_ns >= o->period_ns) && (rtask->criticality > o->criticality));
}

static int getNextInSet(struct task *table, int *cidx, int tablesize,
		struct task *rtask,
		bool (*inSet)(struct task *rtask, struct task *o)) {

	int selectedIdx=-1;

	if (*cidx < 0)
		return -1;

	while ((*cidx < tablesize) &&
		(rtask ==  &table[*cidx] ||
		 !inSet(rtask, &table[*cidx]))) {
		*cidx +=1;
	}

	if (*cidx <tablesize){
		selectedIdx = *cidx;
		*cidx+=1;
	}

	return selectedIdx;
}

static float getExecTimeHigherPrioHigherCrit(struct task *rtask) {
	return rtask->nominal_exectime_ns;
}

static float getExecTimeHigherPrioSameCrit(struct task *rtask) {
	return rtask->exectime_ns;
}

static float getExecTimeLowerPrioHigherCrit(struct task *rtask) {
	return (rtask->nominal_exectime_ns - rtask->exectime_in_rm_ns);
}

static float getResponseTimeCritNs(struct task *table, int tablesize, struct task *rtask) {

	int firsttime=1;
	float resp=0L;
	float prevResp=0L;
	size_t numArrivals=0l;
	int idx=0;
	int selectedIdx=-1;

	resp = rtask->exectime_ns - rtask->exectime_in_rm_ns;

	while (firsttime || (resp > prevResp && resp <= rtask->period_ns)){

		firsttime=0;
		prevResp = resp;

		resp = rtask->exectime_ns - rtask->exectime_in_rm_ns;

		// get interference from Higher Priority Higher Criticality taskset
		idx=0;

		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioHigherCrit)) >=0){

			numArrivals = ceilf(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * getExecTimeHigherPrioHigherCrit(&table[selectedIdx]);
		}

		// get interference from Lower Priority Higher Criticality taskset
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isLowerPrioHigherCrit)) >=0){

			numArrivals = ceilf(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * getExecTimeLowerPrioHigherCrit(&table[selectedIdx]);
		}

		// get interference from Higher Priority Same Criticality taskset
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioSameCrit)) >=0){

			numArrivals = ceilf(prevResp / table[selectedIdx].period_ns);
      			resp += numArrivals * getExecTimeHigherPrioSameCrit(&table[selectedIdx]);
		}
  	}
  	return resp;
}

static float getRMInterference(struct task *table, int tablesize, struct task *rtask, float Z) {

	int idx;
	int selectedIdx;
	size_t numArrivals;
	float interf=0L;

  	idx=0;
	while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioHigherCrit)) >=0){
		numArrivals = ceilf(Z / table[selectedIdx].period_ns);
		interf += numArrivals * table[selectedIdx].nominal_exectime_ns;
	}

	idx=0;
	while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioLowerCrit)) >=0){
		numArrivals = ceilf(Z / table[selectedIdx].period_ns);
		interf += numArrivals * table[selectedIdx].exectime_ns;
	}

	idx=0;
	while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioSameCrit)) >=0){
		numArrivals = ceilf(Z / table[selectedIdx].period_ns);
		interf += numArrivals * table[selectedIdx].exectime_ns;
	}

	return interf;
}

bool getZSRMSTaskResponseTime(struct task *table, int tablesize, struct task *rtask, float *calcZ, float *calcResp)
{
	float resp=0L;
	float Z=0L;
	float prevZ = 0L;
	float interf=0L;
	float interf1=0L;
	float slack=0L;

	rtask->exectime_in_rm_ns = 0L;
	Z = 1L;
	prevZ = 0L;

	while (rtask->period_ns >= resp && Z > prevZ){

		prevZ = Z;
		resp = getResponseTimeCritNs(table,tablesize,rtask);

		if (rtask->period_ns >= resp){

			Z = rtask->period_ns - resp;
			interf = getRMInterference(table, tablesize, rtask,Z);

			if (Z > (interf + rtask->exectime_in_rm_ns)){

				slack = Z - interf - rtask->exectime_in_rm_ns;

			} else if (Z == (interf + rtask->exectime_in_rm_ns)){

				// XXX: Is this needed???
				// Got stuck in limit
				// see if there is some slack if we push Z a bit beyond interference window
				if ( Z + (resp - rtask->exectime_ns + rtask->exectime_in_rm_ns) <= rtask->period_ns) {

					interf1 = getRMInterference(table,tablesize,
									rtask,
									Z + (resp - rtask->exectime_ns + rtask->exectime_in_rm_ns));

					if (interf1 <= resp - (rtask->exectime_ns - rtask->exectime_in_rm_ns)){
						// some slack upened up
						Z = Z + (resp - rtask->exectime_ns + rtask->exectime_in_rm_ns) ;
						interf = interf1;
						slack = Z - interf - rtask->exectime_in_rm_ns;
					}
				}
			} else {
				slack = 0L;
			}

			if (slack + rtask->exectime_in_rm_ns < rtask->exectime_ns) {
				rtask->exectime_in_rm_ns += slack;
			} else {
				rtask->exectime_in_rm_ns = rtask->exectime_ns;
				// set Z to period?
				Z = rtask->period_ns;
			}
		}
	}

	*calcZ = Z;
	*calcResp = Z + resp;

	return (rtask->period_ns >= resp);
}

bool admitAllZSRMSTask(struct task *table, int tablesize)
{
	int i;
	float Z, resp;

	/* Sort table with higher criticality tasks at top */
	qsort(table, tablesize, sizeof(struct task), criticalitySort);

	for (i = 0; i < tablesize; i++) {

		struct task *rtask = &table[i];
		if (getZSRMSTaskResponseTime(table, tablesize, rtask, &Z, &resp) == false)
			return false;
	}

	return true;
}

/* Checks if ZSRMS can schedule task */
bool ZSRMSIsTaskSched(struct task *table, int numEntry, bool *checkPass)
{
	bool ret;

	*checkPass = true;

	assert(checkRMTable(table, numEntry));

	ret = admitAllZSRMSTask(table, numEntry);

	return ret;
}


#ifdef DEBUG

/* Usage: ./rms.elf <C,Co,T,Criticality> ... */
int main(int argc, char **argv)
{
	int i, numEntry;
	float Z, resp;
	bool isSched;

	struct task *table = parseArgs(argc, argv, &numEntry);
	assert(table);

	assert(checkRMTable(table, numEntry));

	/* Sort table with higher criticality tasks at top */
	qsort(table, numEntry, sizeof(struct task), criticalitySort);

	for (i = 0; i < numEntry; i++) {

		struct task *rtask = &table[i];
		isSched = getZSRMSTaskResponseTime(table, numEntry, rtask, &Z, &resp);
		printf("C:%f,\t C':%f,\t T:%f,\t Crit:%f,\t Z: %f, Scheduability:%f\n",
				rtask->nominal_exectime_ns,
				rtask->exectime_ns,
				rtask->period_ns,
				rtask->criticality,
				isSched == true ? Z : -1,
				isSched == true ? resp : -1);
	}

	printf("ZSRMS admit all: %d\n", admitAllZSRMSTask(table, numEntry));

	free(table);
	return 0;
}

#endif /* DEBUG */
