







///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// JOHN YAN - 2014/05/08 - PROFILING
#if 0	// Profiling is normally disabled in production.

// MySQL cached or not!!!
// mysqlnd client side always cache

//CYXE:CYVR
//(pts/0) user@localhost:tmp $ head -30  zzi.1399575414.txt
//!!!: 0 0
//TOTAL FUNCTION: 334 < 1024
//.............................................
//80beb00       2678    40489835908	mysql_query
//8091290       8034    40431627003	php_mysqlnd_conn_data_query_pub
//80ac410       5356    31764040436	php_mysqlnd_res_read_result_metadata_pub
//80b7d50     217122    27315130052	php_mysqlnd_rset_field_read
//807f2d0      15837    10327297105	branch
//80ab190       5354    9699994212	php_mysqlnd_res_free_result_pub

/////////////////////////////// PROFILING
/////////////////////////////// PROFILING
/////////////////////////////// PROFILING
// $ gcc -g instrument.c -finstrument-functions -Wall -ldl
/////////////////////////////// PROFILING





// John Yan - profiling
// cc t.c -finstrument-functions
// ./a.out
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>


// hash
struct cyg_data {
//	struct cyg_data *next;	// first time
	void *fn;
	unsigned long long time;	// enter: microseconds
	unsigned int count;		// enter:
	unsigned long long time_sum;	// exit:
};

#define CYG_ARR_NUM		0xFFFFFF+1
static struct cyg_data cyg_result[CYG_ARR_NUM];

// hash
#define HASHSTEP(x)		((x) & 0xFFFFFF)

//#define CURRENT_TIME(x)	((x).tv_sec*1000000 + (x).tv_usec)



// instrument
//static struct timeval cyg_tv;
static int cyg_i, cyg_full = 0, cyg_missing = 0;

// http://www.mcs.anl.gov/~kazutomo/rdtsc.html
typedef unsigned long long ticks;

static inline
__attribute__((always_inline))
__attribute__((no_instrument_function))
ticks getticks(void)
{
#if __x86_64__
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((ticks)lo) | (((ticks)hi)<<32);
#else
	ticks ret;
	__asm__ __volatile__ ("rdtsc" : "=A" (ret));
	return ret;
#endif
}

static ticks cyg_init, cyg_fini;

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

void
__attribute__((no_instrument_function))
__cyg_profile_func_enter(void *this_fn, void *call_site) {
	// JOHN YAN - 2014/06/24 - DEBUG!
	cyg_i = HASHSTEP((int)this_fn);

	if (likely(cyg_result[cyg_i].fn == this_fn)) {
		// not the first time
		cyg_result[cyg_i].time = getticks();
//		cyg_result[cyg_i].fn = this_fn;
		cyg_result[cyg_i].count++;
		return;
	} else if (cyg_result[cyg_i].fn == NULL) {
		// first time
		cyg_result[cyg_i].time = getticks();
		cyg_result[cyg_i].fn = this_fn;
		cyg_result[cyg_i].count = 1;
		cyg_result[cyg_i].time_sum = 0;
		return;
	}
	// FULL !
	cyg_full++;
} /* __cyg_profile_func_enter */

void
__attribute__((no_instrument_function))
__cyg_profile_func_exit(void *this_fn, void *call_site) {
	// JOHN YAN - 2014/06/24 - RETURN
	cyg_i = HASHSTEP((int)this_fn);

	if (likely(cyg_result[cyg_i].fn == this_fn)) {
		// found
		cyg_result[cyg_i].time_sum += getticks() - cyg_result[cyg_i].time;
		return;
	}
	// ??
	cyg_missing++;
} /* __cyg_profile_func_exit */



// SORT THE RESULT
#define SORT_MAX_N		1024
static struct cyg_data cyg_result2[SORT_MAX_N];

//
//#define _GNU_SOURCE         /* See feature_test_macros(7) */
//#include <dlfcn.h>
typedef struct {
    const char *dli_fname;  /* Pathname of shared object that
                               contains address */
    void       *dli_fbase;  /* Address at which shared object
                               is loaded */
    const char *dli_sname;  /* Name of nearest symbol with address
                               lower than addr */
    void       *dli_saddr;  /* Exact address of symbol named
                               in dli_sname */
} Dl_info;
int dladdr(void *addr, Dl_info *info);
//
static void
//__attribute__((destructor))
__attribute__((no_instrument_function))
__cyg_profile_func_fini(void)
{
	char fname[128];
	FILE *fp;
	int i = 0, j = 0, changed = 1;
	struct cyg_data tmp;
	Dl_info info;
	cyg_fini = getticks();

	for (cyg_i = 0; cyg_i < CYG_ARR_NUM; cyg_i++) {
		if (cyg_result[cyg_i].count == 0)
			continue;
		cyg_result2[j] = cyg_result[cyg_i];
//		cyg_result2[j].fn = (void *)(0x8000000 | cyg_i);
		j++;
	}
	// no instrument???
	if (j == 0)
		return;
	sprintf(fname, "/tmp/zzi.%d.txt", (int)time(NULL));
	fp = fopen(fname, "w");
	if (fp == NULL)
		return;
	// summary
	fprintf(fp, "!!!: %d %d (MUST BE 0 0)\n", cyg_full, cyg_missing);
	fprintf(fp, "PROCESS #:%10d   %12llu\n", getpid(), cyg_fini-cyg_init);
	fprintf(fp, "TOTAL FUNCTIONS: %d < %d\n", j, SORT_MAX_N);
	// sort
	while (changed) {
		changed = 0;
		for (i = 0; i< SORT_MAX_N-1; i++) {
			if (cyg_result2[i].time_sum < cyg_result2[i+1].time_sum) {
				tmp = cyg_result2[i+1];
				cyg_result2[i+1] = cyg_result2[i];
				cyg_result2[i] = tmp;
				changed = 1;
			}
		}
	}
	for (i = 0; i< SORT_MAX_N; i++) {
		if (cyg_result2[i].fn == NULL)
			continue;
		fprintf(fp, "%p %10d   %12llu", cyg_result2[i].fn,
				cyg_result2[i].count, cyg_result2[i].time_sum);
		if (dladdr(cyg_result2[i].fn, &info))
			fprintf(fp, "\t%s\n", info.dli_sname);
		else
			fprintf(fp, "\n");
	}

	fclose(fp);
}
//// initialization
static void
__attribute__((constructor))
__attribute__((no_instrument_function))
__cyg_profile_func_init(void)
{
//	return;
//	for (cyg_i = 0; cyg_i < CYG_ARR_NUM; cyg_i++) {
//		cyg_result[cyg_i].count = 0;
//	}
//	memset(cyg_result2, 0, sizeof(cyg_result2));
	// flush the instrument data
	atexit(__cyg_profile_func_fini);
	cyg_init = getticks();
}

#endif






