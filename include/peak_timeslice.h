#ifndef PEAK_TIMESLICE_H
#define PEAK_TIMESLICE_H

#include <time.h>

typedef struct {
	uint64_t normal;
	uint64_t msec;
	uint64_t sec;
	/* hrm, Ubuntu doesn't build with the name "unix"
	 * and complains in a very cryptic way... */
	time_t epoch;
	struct tm tm;
} timeslice_t;

#define TIMESLICE_ADVANCE(clock, ts_unix, ts_ms) do {			\
	(clock)->msec = (ts_ms) - (clock)->normal + 1000;		\
	(clock)->sec = (clock)->msec / 1000;				\
	if (unlikely((ts_unix) != (clock)->epoch)) {			\
		(clock)->epoch = (ts_unix);				\
		localtime_r(&(clock)->epoch, &(clock)->tm);		\
	}								\
} while (0)

#define TIMESLICE_NORMALIZE(clock, ts_ms) do {				\
	(clock)->normal = (clock)->normal ? : (ts_ms);			\
} while (0)

#define TIMESLICE_INIT(clock) do {					\
	bzero(clock, sizeof(timeslice_t));				\
} while (0)

#endif /* !PEAK_TIMESLICE_H */