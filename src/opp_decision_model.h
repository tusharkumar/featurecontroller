// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_DECISION_MODEL_H
#define OPP_DECISION_MODEL_H

#include <sstream>
#include <list>
#include <map>

#include "opp_exec_time_measure.h"
#include "opp_parameter_spread.h"

namespace Opp {

	////////////////////////////////

	// Allocating Parameter object
	// 1) Allocation of ExecFrame would allocate its Parameter representing the "decision-vector",
	//     with a pointer to Parameter going around to frames that need to track that parameter
	// 2) Similarly, ExecFrame and Frame would allocate a Parameter for their execution-time.
	//
	// -> These are the Parameter "sources", i.e., the entities whose behavior is measured by the
	//    corresponding Parameter.
	//
	// Sharing of Values taken by a Parameter
	// 1) Each Frame that is tracking a Parameter (using a pointer), will hold the values taken
	//     by that Parameter in a IntValueCache local to that Frame. The IntValueCache will
	//     be cleared when the Frame starts, and the final version of the IntValueCache will be
	//     "normalized" as 1 data-sample and used to update the Frame's model when the Frame completes.
	//
	// 2) Each Parameter "source" will keep a list of all Frames ("consumers") tracking that
	//    Parameter. Whenever the source generates a new data-sample for the Parameter, the source
	//    will update the IntValueCache at each consumer that is currently dynamically scoping the
	//    source.
	//
	// Updates using observed Parameter values:
	// 1) A source should only be updating those consumers that are currently dynamically scoping the
	//     source.
	//
	// 2) A source may be consumed by a Frame at multiple possible places => effectively as
	//    one of multiple sources. For example, ExecFrame E may be contained by Frame F1 and F2.
	//    And, Frame F3 may contain F1 and F2. If E's decision-vector is exposed to F3, then
	//    what are the correct semantics for E to be source in F3?
	//      --> Currently, E's parameter will be treated as a single parameter in F3, merging
	//           observations under F1 and F2.

	class Parameter {
		static long long parameter_count;
	public:
		const long long parmID;

		BaseFrame * source; //corresponding to where measurement of Parameter is made

		std::map<Frame *, IntValueCache *> map_consumer_caches;
			//for each Frame that consumes this parameter,  gives the IntValueCache * caching the Parameter


		Parameter(BaseFrame * source)
			: parmID(parameter_count), source(source)
		{ parameter_count++; }

		//Protocol:
		//  1) An IntValueCache object is allocated at the consumer (tracking Frame) by this call.
		//  2) Expectation from Consumer:
		//    a) The consumer is expected to read the IntValueCache to get a summary of the
		//       sample-values encountered for this parameter while the Consumer was in 'Executing' state
		//    b) The consumer is expected to reset the IntValueCache whenever the consumer goes into
		//       Active state.
		//
		//  3) Expectation from Source:
		//    a) Each time a measurement of the Parameter is made for the Source, the Source will
		//       update the IntValueCache for the Consumer, provided the Consumer was found 
		//       to be currently dynamically enclosing the execution of the Source.
		bool has_consumer(Frame * consumer);

		void add_consumer(Frame * consumer);

		void remove_consumer(Frame * consumer);

		void inform_enclosing_active_consumers_of_sample_measurement(
			std::vector<Frame *> vActiveEnclosingFrames,
			int sample_value
		);

	};

	template<typename T>
	class SlidingWindow {
		int sliding_window_size;

		int next_index;
		std::vector<T> vQ;

	public:
		SlidingWindow(int sliding_window_size)
		{ initialize(sliding_window_size); }

		void initialize(int sliding_window_size) {
			this->sliding_window_size = sliding_window_size;
			next_index = 0;
			vQ.clear();
		}

		void push(const T& t) {
			if((int)vQ.size() < sliding_window_size) {
				vQ.push_back(t);
				next_index = (int)vQ.size();
			}
			else { //vQ.size() == sliding_window_size
				vQ.at(next_index) = t;
				next_index++;
			}

			if(next_index == sliding_window_size)
				next_index = 0;
		}

		T get_average() {
			T sum = 0;
			for(int i=0; i<(int)vQ.size(); i++)
				sum += vQ[i];
			return (T) (sum / (double)vQ.size());
		}

		std::string print_string() {
			std::ostringstream oss;
			oss << "[";
			for(int i=0; i<(int)vQ.size(); i++)
				oss << vQ[i] << ", ";
			oss << "]";
			return oss.str();
		}
	};

	ExecTime_t IDENTITY_impact_rescaler(ExecTime_t measured_execution_time_in_seconds);

	class FrameDecisionModel {
	public:
		//definitions for IntValueCache that will cache exec_time_parameter values
		static const int exec_time_parameter_cache_num_entries = 10;
		static const double exec_time_parameter_cache_max_count = 100.0;


	//following fields should be set only via initialize_objective(),
	//  and should be treated as read-only after that
		bool bHasMeanObjectiveDefined;
			// == true iff corresponding frame has been provided a mean-objective

			// defined iff bHasMeanObjectiveDefined == true
		ExecTime_t mean_objective;
		double window_frac_lower;
		double window_frac_upper;
		double prob;
		int sliding_window_size;
		ImpactRescaler_f impact_rescaler;

		int exec_time_parameter_num_spread_bins;
	//till here

		SlidingWindow<ExecTime_t> exec_time_sliding_window;


		Parameter exec_time_parameter;
			//communicates execution-time of current frame

		IntValueCache exec_time_record;
			//tracks the execution-time distribution of the current frame

		long long specified_objective_failure_run_length; //defined iff bHasMeanObjectiveDefined = true
		std::vector<long long> vFailure_Runlengths_wrt_specified_objective;
		
		long long active_objective_failure_run_length; //w.r.t. to current vAGAINST_ObjectiveBinIndices
		std::vector<long long> vFailure_Runlengths_wrt_active_objective;
		

		std::map<Parameter *, ParameterExecSpread *> map_parm_to_spread;

		std::map<Parameter *, IntValueCache *> map_parm_to_curr_record;
			//Caches values taken by a parameter within the current invocation of
			//  frame. Used to update corresponding map_parm_to_spread entry.
			//  Therefore. map_parm_to_curr_record and map_parm_to_spread must
			//    have an identical set of Parameter * keys.

		//NOTE: all allocation & de-allocation of map_parm_to_spread
		//  and map_parm_to_curr_record entries must be done via the
		//  Parameter::add_consumer() & Parameter::remove_consumer() calls

			//Following relevant for Fast Reaction Strategy
		std::vector<double> vCoeffs_a;
				//Coefficients for Y = a1 * x1 + a2 * x2 + ..n an * xn
		ExecTime_t previous_invocation_exec_time;
				//Execution Time consumed by previous invocation of this frame. =0.0 if first invocation
		//RESCALE for RANGE-PRECESION
		std::vector<double> vPrevious_model_choice_double_value;
				//Choices made by each model (if any) within this frame. Values bounded to lie within valid range.
		std::vector<double> vUnbounded_Previous_model_choice_double_value;
				//Same as vPrevious_model_choice_double_value values, except without being bounded to lie within valid range
		std::vector<double> vAverage_X_deviation;
				//average deviation in values of X observed between consecutive choices for each model
		long long int current_window_length_X_deviation;
				//number of contiguous samples over which current average X deviation has been calculated
		std::vector<double> vSum_X_deviation;
				//cumulative deviation in values of X observed between consecutive choices for each model

		long long int current_failure_unidirectional_runlenth;
				//+ve => run of increasing Y in failures, -ve => run of decreasing Y in objective failures
				//  (until all X's saturate, or Y changes direction, or SUCCESS is achieved), magnitude = length of run
		double average_continuous_unidirectional_failure_runlength;
				//Counts the average number of contiguous increments or decrements needed in Y while Objective is failing
				//Averaged over current_number_unidirectional_runs
		long long int current_number_unidirectional_runs;

		//RESCALE for CONTROL-LAG
		double previous_Y;
				//previous value of previous_invocation_exec_time, =0.0 if first time
		int halfcycle_start_deflection_sign;
				//+1 if halfcycle started with Y in a positive deflection w.r.t mean-objective, -1 if halfcycle started with Y in a negative deflection
		double halfcycle_Y_positive_max_deflection;
		double halfcycle_Y_negative_max_deflection;
				//The maximum magnitude deflection from mean objective achieved (maximum +ve and minimum -ve value) so far by the current ongoing halfcycle
		long long int halfcycle_length;
				//The number of frames over which the current halfcycle extends
		bool has_halfcycle_crossed_mean; 
				//Has the halfcycle made a transition from non-positive to non-negative, or vice versa
				//   (a potential halfcycle is realized as true halfcycle only after crossing over the mean-objective)
		double sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window;
				//cumulative weighted sum of quantity of halfcycles completed for non-overlapping sliding windows so far

		//RESCALE for RESPONSIVENESS
		long long int contiguous_onesided_failure_runlength;
			//+ve implies contiguous failures with Y_failure_delta +ve, -ve implies contiguous failures with Y_failure_delta -ve, magnitude is runlength of failures
		std::vector<double> vDeflectionInX_during_onesided_failure;
		long long int correction_runlength_onesided_failure;
		std::vector<double> correction_vDeflectionInX_during_onesided_failure;

		std::vector< std::vector< std::vector<long long int> > > vvvVarChoiceStats;
				//Statistics, for each variable: for each choice value taken: window boundaries into which observed exectime fell
			//till here

		double unbinned_satisfaction_ratio;
		long long int total_invoke_count;
		double unbinned_mean;
		double unbinned_sq_mean;
		double unbinned_variance;
		double unbinned_variance_from_mean_objective;

		FrameDecisionModel(Frame * my_frame)
			: bHasMeanObjectiveDefined(false),
				mean_objective(0.0), window_frac_lower(0.0), window_frac_upper(0.0), prob(0.0), sliding_window_size(1), impact_rescaler(0),
				exec_time_parameter_num_spread_bins(-1),
				exec_time_sliding_window(1), exec_time_parameter(my_frame),
				specified_objective_failure_run_length(0), active_objective_failure_run_length(0),
				previous_invocation_exec_time(0.0),
				unbinned_satisfaction_ratio(0.0), total_invoke_count(0), unbinned_mean(0.0), unbinned_sq_mean(0.0), unbinned_variance(0.0), unbinned_variance_from_mean_objective(0.0)
		{ }

		~FrameDecisionModel() { } //FIXME: must deallocate all dynamically allocated stuff in map_parm_to_spread, map_parm_to_curr_record

		bool isObjectiveInitialized() const
			{ return (exec_time_parameter_num_spread_bins != -1); }

		//Note: objective must be initialized before any Parameters are added to current Frame
		void initialize_objective(
			bool bHasMeanObjectiveDefined,
			ExecTime_t mean_objective = 0.0,
			double window_frac_lower = 0.0,
			double window_frac_upper = 0.0,
			double prob = 0.0,
			int sliding_window_size = 1,
			ImpactRescaler_f impact_rescaler = 0
		)
		{
			assert(exec_time_parameter_num_spread_bins == -1);
				//initialize_objective() should be done only once

			this->bHasMeanObjectiveDefined = bHasMeanObjectiveDefined;
			this->mean_objective = mean_objective;
			this->window_frac_lower = window_frac_lower;
			this->window_frac_upper = window_frac_upper;
			this->prob = prob;
			this->sliding_window_size = sliding_window_size;
			this->impact_rescaler = (impact_rescaler == 0)
										? IDENTITY_impact_rescaler : impact_rescaler;

			if(this->bHasMeanObjectiveDefined)
				exec_time_parameter_num_spread_bins = ExecutionTimeSpread_MeanRelative::num_spread_bins();
			else
				exec_time_parameter_num_spread_bins = ExecutionTimeSpread_Absolute::num_bins;

			exec_time_sliding_window.initialize(sliding_window_size);

			exec_time_record = IntValueCache(exec_time_parameter_num_spread_bins, 100000.0);
		}


			//Get the vFOR and vAGAINST window bin indices based only on the
			//  Objective specified by the user for this frame.
			//Returns empty for both if bHasMeanObjectiveDefined == false.
		void get_ObjectiveWindowBinIndices_for_local_objective(
				//return values:
			std::vector<int>& local_vFOR_ObjectiveWindowBinIndices,
			std::vector<int>& local_vAGAINST_ObjectiveBinIndices
		) const {
			local_vFOR_ObjectiveWindowBinIndices.clear();
			local_vAGAINST_ObjectiveBinIndices.clear();
			if(bHasMeanObjectiveDefined) {
				int range_min_bin_index = convert_exec_time_to_int_bin(
						((1.0 - window_frac_lower) * mean_objective) );
				int range_max_bin_index = convert_exec_time_to_int_bin(
						((1.0 + window_frac_upper) * mean_objective) );

				for(int i=range_min_bin_index; i<=range_max_bin_index; i++)
					local_vFOR_ObjectiveWindowBinIndices.push_back(i);

				for(int i=0; i<range_min_bin_index; i++)
					local_vAGAINST_ObjectiveBinIndices.push_back(i);
				for(int i=range_max_bin_index+1; i<exec_time_parameter_num_spread_bins; i++)
					local_vAGAINST_ObjectiveBinIndices.push_back(i);
			}
		}


		// Conversion utilities between measured execution-time and int-bin

		inline int convert_exec_time_to_int_bin(ExecTime_t exec_time) const
		{
			if(bHasMeanObjectiveDefined)
				return ExecutionTimeSpread_MeanRelative::get_spread_bin_index(exec_time, mean_objective);
			else
				return ExecutionTimeSpread_Absolute::get_spread_bin_index(exec_time);
		}

		inline ExecTime_t convert_int_bin_to_exec_time(int int_bin) const
		{
			if(bHasMeanObjectiveDefined)
				return ExecutionTimeSpread_MeanRelative::get_exec_time_for_index(int_bin, mean_objective);
			else
				return ExecutionTimeSpread_Absolute::get_exec_time_for_index(int_bin);
		}

		inline ExecTime_t convert_int_bin_to_lower_boundary_of_exec_time(int int_bin) const
		{
			if(bHasMeanObjectiveDefined)
				return ExecutionTimeSpread_MeanRelative::get_lower_boundary_exec_time_for_index(int_bin, mean_objective);
			else
				return ExecutionTimeSpread_Absolute::get_lower_boundary_exec_time_for_index(int_bin);
		}

		inline ExecTime_t convert_int_bin_to_upper_boundary_of_exec_time(int int_bin) const
		{
			if(bHasMeanObjectiveDefined)
				return ExecutionTimeSpread_MeanRelative::get_upper_boundary_exec_time_for_index(int_bin, mean_objective);
			else
				return ExecutionTimeSpread_Absolute::get_upper_boundary_exec_time_for_index(int_bin);
		}

		//"decision-setting"
		// Basic Idea:
		// 1) See if a dynamically enclosing parent has a "decision-setting":
		//   a) Top-level parent's setting is most important, with importance reducing
		//      as we go to lower level enclosing frames
		//   b) At any level, see if this frame's execution-time Parameter plays a role
		//       in the enclosing frame's objective being met

		std::vector<int> vFOR_ObjectiveBinIndices;
		std::vector<int> vAGAINST_ObjectiveBinIndices;

	};



	class ExecFrameDecisionModel {
	public:
		Parameter decision_vector_parameter;
		//Parameter exec_time_parameter; //FIXME: ignoring for now

		ExecFrameDecisionModel(ExecFrame * my_execframe)
			: decision_vector_parameter(my_execframe)//,
				//exec_time_parameter(my_execframe)
		{ }


		// class static Conversion utilities between measured execution-time and int-bin

		static inline int convert_exec_time_to_int_bin(ExecTime_t exec_time)
		{ return ExecutionTimeSpread_Absolute::get_spread_bin_index(exec_time); }

		static inline ExecTime_t convert_int_bin_to_exec_time(int int_bin)
		{ return ExecutionTimeSpread_Absolute::get_exec_time_for_index(int_bin); }

	};

	void activate_decision_model_and_decide_setting(Frame * frame);
	void update_decision_model_on_completion(Frame * frame);

	bool get_decision_sets_for_parameter(
		Parameter& deciding_parameter,
		Frame * innermost_deciding_ancestor_frame,
			//return values:
		std::vector<int>& vForDecisionSet,
		std::vector<double>& vForDecisionSet_Counts,
		std::vector<double>& vForDecisionSet_Probs,
		std::vector<int>& vUnclassifiedDecisionSet,
		std::vector<double>& vUnclassifiedDecisionSet_Counts,
		std::vector<double>& vUnclassifiedDecisionSet_Probs,
		std::vector<int>& vAgainstDecisionSet,
		std::vector<double>& vAgainstDecisionSet_Counts,
		std::vector<double>& vAgainstDecisionSet_Probs
	);
	//returns true if some dynamic parent-frame (ancestor) prevented some lower-level parent's preferences from being accomodated
	//else false (i.e., that all dynamic parents were given a chance to express their preference over values of the deciding_parameter)
}

#endif //OPP_DECISION_MODEL_H
