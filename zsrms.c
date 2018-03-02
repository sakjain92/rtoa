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

static double getExecTimeHigherPrioHigherCrit(struct task *rtask) {
	return rtask->nominal_exectime_ns;
}

static double getExecTimeHigherPrioSameCrit(struct task *rtask) {
	return rtask->exectime_ns;
}

static double getExecTimeLowerPrioHigherCrit(struct task *rtask) {
	return (rtask->nominal_exectime_ns - rtask->exectime_in_rm_ns);
}

static double getResponseTimeCritNs(struct task *table, int tablesize, struct task *rtask) {

	int firsttime=1;
	double resp=0L;
	double prevResp=0L;
	int numArrivals=0l;
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

			numArrivals = (int)ceill(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * getExecTimeHigherPrioHigherCrit(&table[selectedIdx]);
		}

		// get interference from Lower Priority Higher Criticality taskset
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isLowerPrioHigherCrit)) >=0){

			numArrivals = (int)ceill(prevResp / table[selectedIdx].period_ns);
			resp += numArrivals * getExecTimeLowerPrioHigherCrit(&table[selectedIdx]);
		}

		// get interference from Higher Priority Same Criticality taskset
		idx=0;
		while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioSameCrit)) >=0){

			numArrivals = (int)ceill(prevResp / table[selectedIdx].period_ns);
      			resp += numArrivals * getExecTimeHigherPrioSameCrit(&table[selectedIdx]);
		}
  	}
  	return resp;
}

/* TODO: This is overly pessimistic */
static double getRMInterference(struct task *table, int tablesize, struct task *rtask, double Z) {

	int idx;
	int selectedIdx;
	int numArrivals;
	double interf=0L;

  	idx=0;
	while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioHigherCrit)) >=0){
		numArrivals = (int)ceill(Z / table[selectedIdx].period_ns);
		interf += numArrivals * table[selectedIdx].nominal_exectime_ns;
	}

	idx=0;
	while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioLowerCrit)) >=0){
		numArrivals = (int)ceill(Z / table[selectedIdx].period_ns);
		interf += numArrivals * table[selectedIdx].exectime_ns;
	}

	idx=0;
	while((selectedIdx = getNextInSet(table, &idx, tablesize, rtask, isHigherPrioSameCrit)) >=0){
		numArrivals = (int)ceill(Z / table[selectedIdx].period_ns);
		interf += numArrivals * table[selectedIdx].exectime_ns;
	}

	return interf;
}

static bool getZSRMSTaskResponseTime(struct task *table, int tablesize, struct task *rtask, double *calcZ, double *calcResp)
{
	double resp=0L;
	double Z=0L;
	double prevZ = 0L;
	double interf=0L;
	double interf1=0L;
	double slack=0L;

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

static bool admitAllZSRMSTask(struct task *table, int tablesize)
{
	int i;
	double Z, resp;

	/* Sort table with higher criticality tasks at top */
	qsort(table, (size_t)tablesize, sizeof(struct task), criticalitySort);

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

	assert(checkTable(table, numEntry));

	ret = admitAllZSRMSTask(table, numEntry);

	return ret;
}


#ifdef DEBUG

/* Usage: ./rms.elf <C,Co,T,Criticality> ... */
int main(int argc, char **argv)
{
	int i, numEntry;
	double Z, resp;
	bool isSched;

	struct task *table = parseArgs(argc, argv, &numEntry);
	if (table == NULL) {
		printf("Error in parsing args\n");
		return -1;
	}

	assert(checkTable(table, numEntry));

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
