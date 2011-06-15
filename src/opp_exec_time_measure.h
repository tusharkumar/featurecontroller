// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_EXEC_TIME_MEASURE_H
#define OPP_EXEC_TIME_MEASURE_H

#include <cmath>
#include "opp.h"

namespace Opp {
	/////////////////////////////////
	// Execution Time measurement
	/////////////////////////////////

	class ExecutionTimeSpread_MeanRelative {
		//Discretizes execution time measurements as bins spread exponentially around objective-means.
		//Allows precise measurement around the objective-mean.
		//
		//Measurements are Relative, and unitless.
	public:
		static double deviation_bin_centers[22];//14];//[] = {-1.0, -0.5, -0.2, -0.1, 0, 0.1, 0.2, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0};
		inline static int num_spread_bins() {
			return 22; //14; //ENFORCE: number of elements in 'deviation_bin_centers[]'
		}

		static int get_spread_bin_index(double exec_time_to_objective_ratio) {
			double offset = exec_time_to_objective_ratio - 1.0;

			int closest_bin_index = 0;
			double closest_absolute_difference = fabs(offset - deviation_bin_centers[0]);
			for(int i=1; i<(int)num_spread_bins(); i++) {
				double abs_diff = fabs(offset - deviation_bin_centers[i]);
				if(abs_diff < closest_absolute_difference) {
					closest_bin_index = i;
					closest_absolute_difference = abs_diff;
				}
			}

			return closest_bin_index;
		}

		static int get_spread_bin_index(ExecTime_t exec_time, ExecTime_t objective) {
			double exec_time_to_objective_ratio = exec_time / objective;
			return get_spread_bin_index(exec_time_to_objective_ratio);
		}

		static ExecTime_t get_exec_time_for_index(int bin_index, ExecTime_t objective) {
			assert(0 <= bin_index && bin_index < num_spread_bins());
			double ratio = deviation_bin_centers[bin_index] + 1.0;
			return ratio * objective;
		}

		static ExecTime_t get_lower_boundary_exec_time_for_index(int bin_index, ExecTime_t objective) {
			ExecTime_t center = get_exec_time_for_index(bin_index, objective);
			ExecTime_t next_lower_bin_center = (bin_index == 0 ? 0.0 : get_exec_time_for_index(bin_index-1, objective));
			ExecTime_t lower = (next_lower_bin_center + center) / 2.0;
			return lower;
		}

		static ExecTime_t get_upper_boundary_exec_time_for_index(int bin_index, ExecTime_t objective) {
			ExecTime_t center = get_exec_time_for_index(bin_index, objective);
			ExecTime_t next_upper_bin_center = (bin_index == num_spread_bins()-1
					? (2 * center - get_exec_time_for_index(bin_index-1, objective))
						//use distance to lower bin-center to extrapolate beyond highest bin
					: get_exec_time_for_index(bin_index+1, objective));

			ExecTime_t upper = (center + next_upper_bin_center) / 2.0;
			return upper;
		}
	};

	class ExecutionTimeSpread_Absolute {
		//Discretizes execution time using an exponentially increasing scale.
		//Used to approximately bin execution-time when there are no guidelines
		//  about where highest precision is needed.
		//
		//Measurements are Absolute, in seconds, with a best precision of milliseconds.
	public:
		static const int num_bins = 20;
		static const double step_ratio = 1.5;
		static const ExecTime_t min_bin_value = 0.001; //1 millisecond
			//=>  max bin = step_ratio^num_bins * min_bin_value

		static int get_spread_bin_index(ExecTime_t exec_time) {
			int num_steps = 0;
			while(exec_time > min_bin_value) {
				exec_time /= step_ratio;
				num_steps++;
			}

			if(num_steps >= num_bins)
				num_steps = num_bins - 1;

			return num_steps;
		}

		static ExecTime_t get_exec_time_for_index(int bin_index) {
			assert(bin_index >= 0 && bin_index < num_bins);

			ExecTime_t multiplier = 1.0;
			for(int i=0; i<(int)bin_index; i++)
				multiplier *= step_ratio;

			return min_bin_value * multiplier;
		}

		static ExecTime_t get_lower_boundary_exec_time_for_index(int bin_index) {
			ExecTime_t center = get_exec_time_for_index(bin_index);
			ExecTime_t next_lower_bin_center = (bin_index == 0 ? 0.0 : get_exec_time_for_index(bin_index-1));
			ExecTime_t lower = (next_lower_bin_center + center) / 2.0;
			return lower;
		}

		static ExecTime_t get_upper_boundary_exec_time_for_index(int bin_index) {
			ExecTime_t center = get_exec_time_for_index(bin_index);
			ExecTime_t next_upper_bin_center = (bin_index == num_bins-1
					? (2 * center - get_exec_time_for_index(bin_index-1))
						//use distance to lower bin-center to extrapolate beyond highest bin
					: get_exec_time_for_index(bin_index+1));

			ExecTime_t upper = (center + next_upper_bin_center) / 2.0;
			return upper;
		}
	};

}

#endif // OPP_EXEC_TIME_MEASURE_H
