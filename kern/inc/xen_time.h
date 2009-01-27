
#ifndef _XEN_TIME_H_
#define _XEN_TIME_H_

#include <stdint.h>
/*
 * System Time
 * 64 bit value containing the nanoseconds elapsed since boot time.
 * This value is adjusted by frequency drift.
 * NOW() returns the current time.
 * The other macros are for convenience to approximate short intervals
 * of real time into system time 
 */
typedef int64_t s_time_t;
#define NOW()                   ((s_time_t)monotonic_clock())
#define SECONDS(_s)             (((s_time_t)(_s))  * 1000000000UL )
#define TENTHS(_ts)             (((s_time_t)(_ts)) * 100000000UL )
#define HUNDREDTHS(_hs)         (((s_time_t)(_hs)) * 10000000UL )
#define MILLISECS(_ms)          (((s_time_t)(_ms)) * 1000000UL )
#define MICROSECS(_us)          (((s_time_t)(_us)) * 1000UL )
#define Time_Max                ((s_time_t) 0x7fffffffffffffffLL)
#define FOREVER                 Time_Max
#define NSEC_TO_USEC(_nsec)     (_nsec / 1000UL)
#define NSEC_TO_SEC(_nsec)      (_nsec / 1000000000ULL)

/* wall clock time  */
typedef long time_t;
typedef long suseconds_t;
struct timeval {
	time_t		tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* microseconds */
};

struct timespec {
    time_t      ts_sec;
    long        ts_nsec;
};


/* prototypes */
void     xen_init_time(void);
s_time_t get_s_time(void);
s_time_t get_v_time(void);
u64      monotonic_clock(void);
void     gettimeofday(struct timeval *tv);
void     block_domain(s_time_t until);

#endif /* _XEN_TIME_H_ */
