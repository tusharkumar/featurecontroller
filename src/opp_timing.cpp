// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <iostream>
#include <cassert>
#include "opp_timing.h"

namespace Opp {

#if 0
double get_curr_time_seconds() {
	struct timeval tv;

	gettimeofday(&tv, NULL);

	double curr_time = (double) tv.tv_sec;
	curr_time += tv.tv_usec / 1000000.0;

	return curr_time; //in seconds
}
#endif

timeval get_curr_timeval() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv;
}

double diff_time(timeval start, timeval end) {
	timeval diff;

	std::cout << "diff_time: "
		<< "start = (" << start.tv_sec << ", " << start.tv_usec << ") "
		<< "end = (" << end.tv_sec << ", " << end.tv_usec << ") ";

	assert(end.tv_sec >= start.tv_sec);

	diff.tv_sec = end.tv_sec - start.tv_sec;
	if(end.tv_usec >= start.tv_usec) {
		diff.tv_usec = end.tv_usec - start.tv_usec;
	}
	else {
		diff.tv_sec--;
		diff.tv_usec = end.tv_usec + (1000000 - start.tv_usec);
	}

	double diff_val = double(diff.tv_sec) + double(diff.tv_usec)/1000000;
	std::cout << " diff = " << diff_val << std::endl;

	return diff_val; //in seconds
}

} //namespace Opp
