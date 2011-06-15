// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <iostream>
#include <list>
#include <sys/time.h>
#include <algorithm>
#include <cstdlib>

#include "opp.h"
#include "opp_timing.h"
#include "opp_baseframe.h"
#include "opp_frame.h"
#include "opp_frame_info.h"
#include "opp_decision_model.h"
#include "opp_execframe.h"

#include "opp_utilities.h"

namespace Opp {

/////////////////////////////
//class ExecFrame definitions
/////////////////////////////

ExecFrame::ExecFrame() : BaseFrame(), stickiness_length(0) {
	//ExecFrame not instantiated with Model, does not do anything
	execframe_info = 0;
}

ExecFrame::ExecFrame(const Model& model, int stickiness_length) : BaseFrame(), stickiness_length(stickiness_length) {
	execframe_info = new ExecFrameInfo(this, model);
}

ExecFrame::~ExecFrame() {
	if(execframe_info != 0)
		delete execframe_info;
	execframe_info = 0;
}

void ExecFrame::run() {
	ExecFrameInfo * execframe_info = ExecFrameInfo::get_execframe_info(this);
	execframe_info->run();
}

void ExecFrame::force_default_selection(bool bEnable) {
	ExecFrameInfo * execframe_info = ExecFrameInfo::get_execframe_info(this);
	execframe_info->bForceDefaultSelectChoice = bEnable;
}

void ExecFrame::force_fast_reaction_strategy_fixed_coeff(bool bEnable) {
	ExecFrameInfo * execframe_info = ExecFrameInfo::get_execframe_info(this);
	execframe_info->bForceFixedCoeff_in_FastReactionStrategy = bEnable;
}


/////////////////////////////
//API function-calls related to ExecFrame
/////////////////////////////

ExecFrame * get_execframe_from_execframe_id(FrameID_t execframe_id)
{
	BaseFrame * base_frame = vBaseFrames.at(execframe_id);
	ExecFrame * execframe = dynamic_cast<ExecFrame *>(base_frame);
	assert(execframe != 0);
	return execframe;
}





/////////////////////////////
//class ExecFrameInfo definitions
/////////////////////////////

//debug control
double probability_of_exploration = 0.0;

void feature_control_probability_of_exploration(double new_setting) {
	assert(0.0 <= new_setting && new_setting < 1.0);
	probability_of_exploration = new_setting;
	std::cout << "SRT Feature Control: probability_of_exploration = " << probability_of_exploration << std::endl;
}

double feature_query_probability_of_exploration() {
	return probability_of_exploration;
}


//debug control
bool use_fast_reaction_strategy = false;

void feature_control_use_fast_reaction_strategy(bool new_setting) {
	use_fast_reaction_strategy = new_setting;
	std::cout << "SRT Feature Control: use_fast_reaction_strategy = " << use_fast_reaction_strategy << std::endl;
}

bool feature_query_use_fast_reaction_strategy() {
	return use_fast_reaction_strategy;
}



bool sort_helper_vRank_Index(std::pair<double, int> x, std::pair<double, int> y)
{ return (y.first < x.first); } //sort descending

int ExecFrameInfo::choose_decision_vector_int_value() {
	ExecFrameInfo * execframe_info = this;

	ExecFrameDecisionModel& execframe_dec_model = execframe_info->decision_model;

	if(curr_parent_frame == 0) { // simply return the highest priority order decision-vector
		std::vector<int> highest_po_dec_vec = get_highest_priority_order_decision_vector();
		return convert_decision_vector_to_int(highest_po_dec_vec);
	}

	// Now, curr_parent_frame != 0, parent frame present

	std::cout << "ExecFrameInfo::choose_decision_vector_int_value(): has parent frame" << std::endl;
	
	if(use_fast_reaction_strategy) {
		return fast_reaction_strategy_choice_int_value();
	}


	std::vector<int> vForDecisionSet;
	std::vector<double> vForDecisionSet_Counts;
	std::vector<double> vForDecisionSet_Probs;
	std::vector<int> vUnclassifiedDecisionSet;
	std::vector<double> vUnclassifiedDecisionSet_Counts;
	std::vector<double> vUnclassifiedDecisionSet_Probs;
	std::vector<int> vAgainstDecisionSet;
	std::vector<double> vAgainstDecisionSet_Counts;
	std::vector<double> vAgainstDecisionSet_Probs;
	bool bUpperParentBlocksLowerParentsPreferences
		= get_decision_sets_for_parameter(
				execframe_dec_model.decision_vector_parameter, curr_parent_frame,
					//return values:
				vForDecisionSet, vForDecisionSet_Counts, vForDecisionSet_Probs,
				vUnclassifiedDecisionSet, vUnclassifiedDecisionSet_Counts, vUnclassifiedDecisionSet_Probs,
				vAgainstDecisionSet, vAgainstDecisionSet_Counts, vAgainstDecisionSet_Probs
			);

	if(vForDecisionSet.size() > 0) { //some discrimating decision-vectors are known
		//pick highest 'rank' one from the known decision-vectors at the decision-level dec_index
		// Sort Criteria (in decreasing order of importance)
		//   - FOR-probability => best chance of meeting objectives for all levels
		//   - weight (count)  => go with tried and tested decision
		//   - priority        => in the absence of predictive knowledge, pick programatically preferred decision-vector
		// ==> create 'rank' for each decision-vector value
		//        (OPTIMIZATION: rank can be stored and incrementally adjusted, instead of recomputed)

		std::cout << "ExecFrameInfo::choose_decision_vector_int_value(): decision-vector ranks FOR:";
		std::vector<double> vRanks(vForDecisionSet.size(), 0.0); //corresponding to values in vForDecisionSet
		for(int i=0; i<(int)vForDecisionSet.size(); i++) {
			double rank = vForDecisionSet_Probs[i] * 100.0 + vForDecisionSet_Counts[i] * 10.0
				- ((double)vForDecisionSet[i]) / ((double)get_num_decision_vectors());
			vRanks[i] = rank;
			std::cout << "(" << vForDecisionSet[i] << ", " << rank << ") ";
		}
		std::cout << std::endl;
		double max_rank = vRanks.at(0);
		int max_rank_index = 0;
		for(int i=1; i<(int)vRanks.size(); i++) {
			if(max_rank < vRanks[i]) {
				max_rank = vRanks[i];
				max_rank_index = i;
			}
		}
		stickiness_runlength_remaining = 0;
		return vForDecisionSet.at(max_rank_index);
	}
	else { //no decision level worked, or no decision level exists
		//choose highest 'rank' decision-vector that is not present in vAgainstDecisionSet
		// Sort Criteria:
		//   rank of decision-vector = prob-SUCCESS * (1 - count-ratio) + weight * normalized-priority
		//
		//   where:
		//     prob-SUCCESS = (# of SUCCESS occurences) / (occurence count)  of this decision-vector
		//      prob-SUCCESS = 1.0 when (occurence count) = 0
		//
		//     max-count = maximum occurence count across all decision-vectors
		//     count-ratio = (occurence count for decision-vector) / max-count
		//
		//     normalized-priority = 1 - priority / (total number of decision-vectors)  (priority = 0 is highest priority)
		//
		//     weight = constant, choose value s.t.
		//        a low priority decision-vector gets a higher rank than a high-priority "stabilized" decision-vector.
		//        stabilized => decision-vector with a large count, but one whose probability has stabilized to a value
		//           insufficient for inclusion in vForDecisionSet.
		//
		//         0.8 * 0.5 + w * 1 <= 1 * 1 + w * 0 => w <= 0.6. Choose w = 0.6
		//

		std::cout << "ExecFrameInfo::choose_decision_vector_int_value(): no decision level worked" << std::endl;
		std::cout << "   vAgainstDecisionSet = [";
		for(int x=0; x<(int)vAgainstDecisionSet.size(); x++)
			std::cout << vAgainstDecisionSet[x] << " ";
		std::cout << "]" << std::endl;
		std::cout << "   Unprocessed vUnclassifiedDecisionSet_Probs = [";
		for(int x=0; x<(int)vUnclassifiedDecisionSet.size(); x++)
			std::cout << vUnclassifiedDecisionSet[x] << " ";
		std::cout << "]" << std::endl;

		std::vector<int> highest_po_dec_vec = get_highest_priority_order_decision_vector();

		std::vector<int> curr_dec_vec = highest_po_dec_vec;
		int curr_dec_vec_int_val = -1;
		int last_dec_vec_int_val = -1;
		int first_prob_expl_skipped_dec_vec_int_val = -1;
		int first_prob_expl_skipped_stickiness_runlength = 0;

		//Add to vUnclassifiedDecisionSet_Probs all decision-vectors which are not already present
		//  and which are not in vAgainstDecisionSet. Main idea is to add untried decision-vectors.
		while(1) {
			//FIXME: Needlessly inefficient. O(n^2) operation where O(n) should have sufficed.
			curr_dec_vec_int_val = convert_decision_vector_to_int(curr_dec_vec);
			bool isPresentInUnclassifiedSet = false;
			for(int i=0; i<(int)vUnclassifiedDecisionSet.size(); i++) { //search vAgainstDecisionSet
				if(vUnclassifiedDecisionSet[i] == curr_dec_vec_int_val) {
					isPresentInUnclassifiedSet = true;
					break;
				}
			}

			if(isPresentInUnclassifiedSet == false) {
				bool isPresentInAgainstSet = false;
				for(int i=0; i<(int)vAgainstDecisionSet.size(); i++) { //search vAgainstDecisionSet
					if(vAgainstDecisionSet[i] == curr_dec_vec_int_val) {
						isPresentInAgainstSet = true;
						break;
					}
				}

				if(isPresentInAgainstSet == false) { //add to vUnclassifiedDecisionSet
					vUnclassifiedDecisionSet.push_back( curr_dec_vec_int_val );
					vUnclassifiedDecisionSet_Counts.push_back( 0.0 );
					vUnclassifiedDecisionSet_Probs.push_back( 1.0 );
				}
			}

			curr_dec_vec = get_next_lower_priority_order_decision_vector(curr_dec_vec);

			if(last_dec_vec_int_val == curr_dec_vec_int_val)
				break;

			last_dec_vec_int_val = curr_dec_vec_int_val;
		}

		std::vector< std::pair<double, int> > vRank_Index;
			//<rank,index>: rank for decision-vector given by vUnclassifiedDecisionSet[index]

		vRank_Index.resize( vUnclassifiedDecisionSet.size() );

		double total_count = 0.0;
		for(int i=0; i<(int)vUnclassifiedDecisionSet.size(); i++)
			total_count += vUnclassifiedDecisionSet_Counts.at(i);

		if(total_count == 0.0)
			total_count = 1.0; //safe to do, since all numerators == 0

		//Compute ranks
		int num_decision_vectors = get_num_decision_vectors();
		for(int i=0; i<(int)vUnclassifiedDecisionSet.size(); i++) {
			int dec_vec_int_val = vUnclassifiedDecisionSet[i];
			double count_ratio = vUnclassifiedDecisionSet_Counts.at(i) / total_count;
			double normalized_priority = 1.0 - ((double)dec_vec_int_val) / num_decision_vectors;
			double prob_success = vUnclassifiedDecisionSet_Probs.at(i);

			double rank = prob_success * (1.0 - count_ratio) + 0.6 * normalized_priority;

			vRank_Index.at(i) = std::make_pair(rank, i);
		}

		//Sort vRank_Index in descending order of rank

		std::sort(vRank_Index.begin(), vRank_Index.end(), sort_helper_vRank_Index);

		std::cout << "   vUnclassifiedDecisionSet = [";
		for(int x=0; x<(int)vRank_Index.size(); x++) {
			double rank = vRank_Index[x].first;
			int index = vRank_Index[x].second;

			int dec_vec_int_val = vUnclassifiedDecisionSet.at(index);
			double count = vUnclassifiedDecisionSet_Counts.at(index);
			double prob = vUnclassifiedDecisionSet_Probs.at(index);

			std::cout << "(" << dec_vec_int_val << ", " << rank << " {" << count << "," << prob << "}), ";
		}
		std::cout << "]" << std::endl;

		//choose a decision-vector in rank order
		curr_dec_vec_int_val = -1;
		stickiness_runlength_remaining = 0;
		for(int i=0; i<(int)vRank_Index.size(); i++) {
			int index = vRank_Index[i].second;
			curr_dec_vec_int_val = vUnclassifiedDecisionSet.at(index);
			if(vUnclassifiedDecisionSet_Counts.at(index) < my_execframe->stickiness_length) {
				stickiness_runlength_remaining = my_execframe->stickiness_length;
				sticky_decision_vector_int_val = curr_dec_vec_int_val;
			}

			if(probability_of_exploration > 0.0) { //debug control: probabilitic exploration
				double random_sample = (std::rand() % 1000) / 1000.0; // 0 <= random_sample <= 1
				if(probability_of_exploration > random_sample) {
					//pretend curr_dec_vec_int_val cannot be used, force further exploration
					if(first_prob_expl_skipped_dec_vec_int_val == -1) { //is this the first value being skipped
						first_prob_expl_skipped_dec_vec_int_val = curr_dec_vec_int_val;
						first_prob_expl_skipped_stickiness_runlength = stickiness_runlength_remaining;
					}

					std::cout << "ExecFrame #" << my_execframe->id << ": Probabilistic Exploration forced past workable "
						<< "curr_dec_vec_int_val = " << curr_dec_vec_int_val << std::endl;

					curr_dec_vec_int_val = -1;
					stickiness_runlength_remaining = 0;
				}
			}

			if(curr_dec_vec_int_val != -1) { //not skipped at index
				break;
			}
		}

		if(curr_dec_vec_int_val == -1) {
			//vUnclassifiedDecisionSet empty or everything skipped
			//Therefore, find choice in vAgainstDecisionSet that is currently least harmful
			//
			//FIXME: Determine a manner to optimize away having to recompute ranks each time.

			if(first_prob_expl_skipped_dec_vec_int_val != -1) {
				//probabilistic exploration skipped over good value. Use that instead of one from vAgainstDecisionSet.
				stickiness_runlength_remaining = first_prob_expl_skipped_stickiness_runlength;
				return first_prob_expl_skipped_dec_vec_int_val;
			} //else: no probabilistic skipping occured, no workable values exist, have to use vAgainstDecisionSet

			std::cout << "ExecFrameInfo::choose_decision_vector_int_value(): decision-vector ranks AGAINST:";
			std::vector<double> vRanks(vAgainstDecisionSet.size(), 0.0); //corresponding to values in vAgainstDecisionSet
			for(int i=0; i<(int)vAgainstDecisionSet.size(); i++) {
				double rank = - vAgainstDecisionSet_Probs[i] * 100.0 - vAgainstDecisionSet_Counts[i] * 10.0
					- ((double)vAgainstDecisionSet[i]) / ((double)get_num_decision_vectors());
				vRanks[i] = rank;
				std::cout << "(" << vAgainstDecisionSet[i] << ", " << rank << ") ";
			}
			std::cout << std::endl;
			double max_rank = vRanks.at(0);
			int max_rank_index = 0;
			for(int i=1; i<(int)vRanks.size(); i++) {
				if(max_rank < vRanks[i]) {
					max_rank = vRanks[i];
					max_rank_index = i;
				}
			}
			curr_dec_vec_int_val = vAgainstDecisionSet.at(max_rank_index);
			if(vAgainstDecisionSet_Counts.at(max_rank_index) < my_execframe->stickiness_length) {
				stickiness_runlength_remaining = my_execframe->stickiness_length;
				sticky_decision_vector_int_val = curr_dec_vec_int_val;
			}
		}

		return curr_dec_vec_int_val;
	}
}


const double max_window_length_X_deviation = 100;
	//length of window over which to compute average deviation in X

// sum_deviation = s(n) + d * s(n-1) + d^2 * s(n-2) + ... = ~ 1/(1-d) * S (when: S = ~ s(n) = ~ s(n-1) = ~ s(n-2) ...)
const double deviation_weight = 0.9;
const double deviation_geometric_convergence = 1.0 / (1.0 - deviation_weight);

const double deviation_weight_short = 0.6;
const double deviation_geometric_convergence_short = 1.0 / (1.0 - deviation_weight_short);

const double array_StatWindowBoundaries[] = {-1.0, -.75, -.40, -.20, -.10, .10, .20, .40, .75, 1.0, 1.5, 2.0, 3.0, 5.0};
const std::vector<double> vStatWindowBoundaries( array_StatWindowBoundaries, array_StatWindowBoundaries + sizeof(array_StatWindowBoundaries)/sizeof(*array_StatWindowBoundaries) );

#include "opp_srt_version.h"
#ifndef SRT_VERSION
#error "Macro variable SRT_VERSION must be set to current path or current version of SRT for reporting in logs"
#endif

bool bFirstTime = true;

int ExecFrameInfo::fast_reaction_strategy_choice_int_value() {
	assert(curr_parent_frame != 0);

	//Decision Strategy optimizes only for achieving the immediate parent's objective.
	//Assumption: choice 0 is most complex, and other choices monotonically decrease in complexity (hopefully also linearly)

	FrameInfo * parent_frame_info = FrameInfo::get_frame_info(curr_parent_frame);
	FrameDecisionModel& parent_frame_dec = parent_frame_info->decision_model;

	if(bFirstTime) {
		std::cout << "INVOKING fast_reaction_strategy_choice_int_value from SRT_VERSION: " << SRT_VERSION << std::endl;
		bFirstTime = false;
	}

	if(parent_frame_dec.bHasMeanObjectiveDefined == false) {
		return 0; //impose most complex choice for all (x1, x2, .. xn)
	}

	double Y_failure_delta = parent_frame_dec.previous_invocation_exec_time - parent_frame_dec.mean_objective;

	assert(vDecisionVector.size() > 0); //Model must contain atleast one Select model, which can then be controlled to achieve Objective
	if(parent_frame_dec.vPrevious_model_choice_double_value.size() == 0) { //not initialized
		parent_frame_dec.vPrevious_model_choice_double_value.resize( vDecisionVector.size(), -1.0 );
		parent_frame_dec.vUnbounded_Previous_model_choice_double_value.resize( vDecisionVector.size(), -1.0 );
		parent_frame_dec.vAverage_X_deviation.resize( vDecisionVector.size(), 0.0 );
		parent_frame_dec.vSum_X_deviation.resize( vDecisionVector.size(), 0.0 );

		parent_frame_dec.vCoeffs_a.resize( vDecisionVector.size(), -1.0/5000.0 );
		for(int i=0; i<(int)parent_frame_dec.vCoeffs_a.size(); i++) {
			if(vInitialCoeffs_fast_reaction_strategy.at(i) != 0.0)
				parent_frame_dec.vCoeffs_a[i] = (-1) * vInitialCoeffs_fast_reaction_strategy[i];
		}
		
		parent_frame_dec.current_window_length_X_deviation = 0;
		parent_frame_dec.current_failure_unidirectional_runlenth = 0;
		parent_frame_dec.average_continuous_unidirectional_failure_runlength = 0.0;
		parent_frame_dec.current_number_unidirectional_runs = 0;

		parent_frame_dec.previous_Y = 0.0;
		parent_frame_dec.halfcycle_start_deflection_sign = (parent_frame_dec.previous_Y <= parent_frame_dec.mean_objective ? -1 : +1);
		parent_frame_dec.halfcycle_Y_positive_max_deflection = 0.0;
		parent_frame_dec.halfcycle_Y_negative_max_deflection = 0.0;
		parent_frame_dec.halfcycle_length = 0;
		parent_frame_dec.has_halfcycle_crossed_mean = false;
		parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window = 0.0;

		parent_frame_dec.contiguous_onesided_failure_runlength = 0;
		parent_frame_dec.vDeflectionInX_during_onesided_failure.resize( vDecisionVector.size(), 0.0 );
		parent_frame_dec.correction_runlength_onesided_failure = 0;
		parent_frame_dec.correction_vDeflectionInX_during_onesided_failure.resize( vDecisionVector.size(), 0.0 );

		parent_frame_dec.vvvVarChoiceStats.resize( vDecisionVector.size() );
		for(int i=0; i<(int)parent_frame_dec.vvvVarChoiceStats.size(); i++) { //for each variable
			parent_frame_dec.vvvVarChoiceStats[i].resize( vVarPriority[i].size(), std::vector<long long int>(vStatWindowBoundaries.size(), 0) );
		}

		std::cout << "fast_reaction_strategy_choice_int_value(): Initializing: vCoeffs_a = " << vector_print_string(parent_frame_dec.vCoeffs_a) << std::endl;
	}

	//half-cycle updates
	double Y_deflection_since_previous = parent_frame_dec.previous_invocation_exec_time - parent_frame_dec.previous_Y;


	if(parent_frame_dec.has_halfcycle_crossed_mean == true) { //previously crossed mean-objective => true halfcycle
		assert( (parent_frame_dec.halfcycle_start_deflection_sign == -1 && parent_frame_dec.previous_Y >= parent_frame_dec.mean_objective)
			|| (parent_frame_dec.halfcycle_start_deflection_sign == +1 && parent_frame_dec.previous_Y <= parent_frame_dec.mean_objective) );

		//potentially end true halfcycle
		if(
			(parent_frame_dec.halfcycle_start_deflection_sign == -1 && Y_deflection_since_previous < 0.0)
			|| (parent_frame_dec.halfcycle_start_deflection_sign == +1 && Y_deflection_since_previous > 0.0)
		) {
			//end current true halfcycle
			assert(parent_frame_dec.halfcycle_length > 0);
			double magnitude_wrt_sliding_window = (parent_frame_dec.halfcycle_Y_positive_max_deflection - parent_frame_dec.halfcycle_Y_negative_max_deflection)
														/ (parent_frame_dec.halfcycle_length / (double)parent_frame_dec.sliding_window_size);

			parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window
				= parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window * deviation_weight_short
					+ magnitude_wrt_sliding_window;

			//start new potential halfcycle
			parent_frame_dec.halfcycle_start_deflection_sign = (Y_failure_delta > 0.0 ? +1 : -1);
			parent_frame_dec.halfcycle_Y_positive_max_deflection = (Y_failure_delta >= 0.0 ? Y_failure_delta : 0.0);
			parent_frame_dec.halfcycle_Y_negative_max_deflection = (Y_failure_delta <= 0.0 ? Y_failure_delta : 0.0);
			parent_frame_dec.halfcycle_length = 0;
			parent_frame_dec.has_halfcycle_crossed_mean = false;
		}
	}
	else { //halfcycle has not previously crossed mean-objective => not yet a true halfcycle
		assert( (parent_frame_dec.halfcycle_start_deflection_sign == -1 && parent_frame_dec.previous_Y <= parent_frame_dec.mean_objective)
			|| (parent_frame_dec.halfcycle_start_deflection_sign == +1 && parent_frame_dec.previous_Y >= parent_frame_dec.mean_objective) );

		if( //check for mean-crossing
			(parent_frame_dec.halfcycle_start_deflection_sign == -1 && Y_failure_delta > 0.0)
			|| (parent_frame_dec.halfcycle_start_deflection_sign == +1 && Y_failure_delta < 0.0)
		)
		{ parent_frame_dec.has_halfcycle_crossed_mean = true; }
	}

	if(Y_failure_delta > parent_frame_dec.halfcycle_Y_positive_max_deflection)
		parent_frame_dec.halfcycle_Y_positive_max_deflection = Y_failure_delta;
	if(Y_failure_delta < parent_frame_dec.halfcycle_Y_negative_max_deflection)
		parent_frame_dec.halfcycle_Y_negative_max_deflection = Y_failure_delta;
	assert(parent_frame_dec.halfcycle_Y_positive_max_deflection >= 0.0);
	assert(parent_frame_dec.halfcycle_Y_negative_max_deflection <= 0.0);

	parent_frame_dec.halfcycle_length++;
	parent_frame_dec.previous_Y = parent_frame_dec.previous_invocation_exec_time;


	std::cout << "fast_reaction_strategy_choice_int_value(): Y_failure_delta = " << Y_failure_delta
		<< " Y_deflection_since_previous = " << Y_deflection_since_previous
		<< " halfcycle_start_deflection_sign = " << parent_frame_dec.halfcycle_start_deflection_sign
		<< " halfcycle_Y_positive_max_deflection = " << parent_frame_dec.halfcycle_Y_positive_max_deflection
		<< " halfcycle_Y_negative_max_deflection = " << parent_frame_dec.halfcycle_Y_negative_max_deflection
		<< " halfcycle_length = " << parent_frame_dec.halfcycle_length
		<< " has_halfcycle_crossed_mean = " << parent_frame_dec.has_halfcycle_crossed_mean
		<< " sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window = " << parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window
		<< std::endl;


	//Update stats
	int occured_stat_window_bin = -1;
	for(int b=0; b<(int)vStatWindowBoundaries.size(); b++) {
		if(b < (int)vStatWindowBoundaries.size() - 1) {
			if( parent_frame_dec.previous_invocation_exec_time >= parent_frame_dec.mean_objective * (1.0 + vStatWindowBoundaries[b])
				&& parent_frame_dec.previous_invocation_exec_time < parent_frame_dec.mean_objective * (1.0 + vStatWindowBoundaries[b+1]) )
			{
				occured_stat_window_bin = b;
				break;
			}
		}
		else { //last bin
			if( parent_frame_dec.previous_invocation_exec_time >= parent_frame_dec.mean_objective * (1.0 + vStatWindowBoundaries[b]) )
			{
				occured_stat_window_bin = b;
				break;
			}
		}
	}
	assert(occured_stat_window_bin >= 0 && occured_stat_window_bin < (int)vStatWindowBoundaries.size());

	for(int i=0; i<(int)parent_frame_dec.vvvVarChoiceStats.size(); i++) { //for each variable
		int previous_choice = int(parent_frame_dec.vPrevious_model_choice_double_value[i] + 0.5);
		if(previous_choice < 0) //first time
			continue;

		parent_frame_dec.vvvVarChoiceStats[i].at(previous_choice).at(occured_stat_window_bin)++;
	}

	std::cout << " fast_reaction_strategy_choice_int_value(): vvvVarChoiceStats:" << std::endl;
	for(int i=0; i<(int)parent_frame_dec.vvvVarChoiceStats.size(); i++) { //for each variable
		std::cout << " X" << i << ":";
		for(int k=0; k<(int)vStatWindowBoundaries.size(); k++)
			std::cout << vStatWindowBoundaries[k]*100.0 << "%" << "  ";
		std::cout << std::endl;
		for(int j=0; j<(int)parent_frame_dec.vvvVarChoiceStats[i].size(); j++) { //for each choice value of current variable
			std::cout << "   " << j << ": ";
			for(int k=0; k<(int)parent_frame_dec.vvvVarChoiceStats[i][j].size(); k++)
				std::cout << parent_frame_dec.vvvVarChoiceStats[i][j][k] << "    ";
			std::cout << std::endl;
		}
	}


#if 0 //Use binned objective window
	assert(parent_frame_dec.vFOR_ObjectiveBinIndices.size() != 0);

	int previous_exec_time_as_bin_index = parent_frame_dec.convert_exec_time_to_int_bin( parent_frame_dec.previous_invocation_exec_time );

	bool bActiveObjectiveSuccess = ( std::find(
				parent_frame_dec.vFOR_ObjectiveBinIndices.begin(), parent_frame_dec.vFOR_ObjectiveBinIndices.end(),
				previous_exec_time_as_bin_index) != parent_frame_dec.vFOR_ObjectiveBinIndices.end() );
#else //Use exact objective window
	bool bActiveObjectiveSuccess = ( parent_frame_dec.previous_invocation_exec_time >= parent_frame_dec.mean_objective * (1.0 - parent_frame_dec.window_frac_lower)
		&& parent_frame_dec.previous_invocation_exec_time <= parent_frame_dec.mean_objective * (1.0 + parent_frame_dec.window_frac_upper) );

#endif


	if(bActiveObjectiveSuccess) {
		std::cout << "fast_reaction_strategy_choice_int_value(): previous SUCCESS: re-use" << std::endl;
		std::vector<int> vNewDecisionValues;
		for(int i=0; i<(int)parent_frame_dec.vPrevious_model_choice_double_value.size(); i++) {
			int new_decision_val;
			if(parent_frame_dec.vPrevious_model_choice_double_value[i] < 0.0) //first time
				new_decision_val = 0;
			else
				new_decision_val = int(parent_frame_dec.vPrevious_model_choice_double_value[i] + 0.5);
			vNewDecisionValues.push_back( new_decision_val );

			//Downgrade distortion metrics

			//double X_deviation = 0.0; //since SUCCESS //FIXME: HACK!
			//parent_frame_dec.vAverage_X_deviation[i] = ( parent_frame_dec.vAverage_X_deviation[i] * (parent_frame_dec.current_window_length_X_deviation - 1)
			//								+ X_deviation ) / parent_frame_dec.current_window_length_X_deviation;
			
			//parent_frame_dec.vSum_X_deviation[i] = parent_frame_dec.vSum_X_deviation[i] * deviation_weight + X_deviation;
		}

		if(parent_frame_dec.current_failure_unidirectional_runlenth != 0) //this success terminated a run
		{
			parent_frame_dec.current_number_unidirectional_runs++;
			parent_frame_dec.average_continuous_unidirectional_failure_runlength =
				( parent_frame_dec.average_continuous_unidirectional_failure_runlength * (parent_frame_dec.current_number_unidirectional_runs - 1)
					+ fabs(parent_frame_dec.current_failure_unidirectional_runlenth) ) / parent_frame_dec.current_number_unidirectional_runs;
			parent_frame_dec.current_failure_unidirectional_runlenth = 0; //success => no start of failure run
		}

		parent_frame_dec.correction_runlength_onesided_failure = fabs(parent_frame_dec.contiguous_onesided_failure_runlength);
		parent_frame_dec.contiguous_onesided_failure_runlength = 0;
		parent_frame_dec.correction_vDeflectionInX_during_onesided_failure = parent_frame_dec.vDeflectionInX_during_onesided_failure;
		for(int i=0; i<(int)parent_frame_dec.vDeflectionInX_during_onesided_failure.size(); i++)
			parent_frame_dec.vDeflectionInX_during_onesided_failure[i] = 0.0;

		return convert_decision_vector_to_int( vNewDecisionValues );
	}

	//Now: bActiveObjectiveSuccess == false ==> FAILURE
	
	parent_frame_dec.current_window_length_X_deviation++;

	if(
		(Y_failure_delta > 0.0 && parent_frame_dec.contiguous_onesided_failure_runlength < 0)
		|| (Y_failure_delta < 0.0 && parent_frame_dec.contiguous_onesided_failure_runlength > 0)
		|| ( fabs(parent_frame_dec.contiguous_onesided_failure_runlength) >= 2 * parent_frame_dec.sliding_window_size )
	)
	{
		parent_frame_dec.correction_runlength_onesided_failure = fabs(parent_frame_dec.contiguous_onesided_failure_runlength);
		parent_frame_dec.contiguous_onesided_failure_runlength = 0;
		parent_frame_dec.correction_vDeflectionInX_during_onesided_failure = parent_frame_dec.vDeflectionInX_during_onesided_failure;
		for(int i=0; i<(int)parent_frame_dec.vDeflectionInX_during_onesided_failure.size(); i++)
			parent_frame_dec.vDeflectionInX_during_onesided_failure[i] = 0.0;
	}

	if(Y_failure_delta >= 0.0)
		parent_frame_dec.contiguous_onesided_failure_runlength++;
	else
		parent_frame_dec.contiguous_onesided_failure_runlength--;
		

#if 0
	//General form:
	// y = a1 * x1 + a2 * x2 + ... + an * xn
	//   vCoeffs_a = (a1, a2, ... an)
	//
	// dy = desired change in y
	//
	// change parameter: t = dy / (a1^2 + a2^2 + ... + an^2)
	//
	// dx1 = a1 * t, dx2 = a2 * t, ... dxn = an * t  (also direction of maximal change in y, i.e. along vector gradient(y))
	

	double a_square = 0.0;
	for(int i=0; i<(int)parent_frame_dec.vCoeffs_a.size(); i++)
		a_square += parent_frame_dec.vCoeffs_a[i] * parent_frame_dec.vCoeffs_a[i];

	assert(a_square > 0.0);
	double t = Y_failure_delta / a_square;

	std::vector<double> vNew_X(parent_frame_dec.vPrevious_model_choice_double_value.size(), 0.0);
	for(int i=0; i<(int)vNew_X.size(); i++) {
		if(parent_frame_dec.vPrevious_model_choice_double_value[i] < 0.0) //first time
			vNew_X[i] = 0.0; //start with most complex
		else {
			vNew_X[i] = parent_frame_dec.vPrevious_model_choice_double_value[i] - parent_frame_dec.vCoeffs_a[i] * t;

			double X_deviation = fabs(vNew_X[i] - parent_frame_dec.vPrevious_model_choice_double_value[i]);
			parent_frame_dec.vAverage_X_deviation[i] = ( parent_frame_dec.vAverage_X_deviation[i] * (parent_frame_dec.current_window_length_X_deviation - 1)
											+ X_deviation ) / parent_frame_dec.current_window_length_X_deviation;
			
			parent_frame_dec.vSum_X_deviation[i] = parent_frame_dec.vSum_X_deviation[i] * deviation_weight + X_deviation;

			parent_frame_dec.vDeflectionInX_during_onesided_failure[i] += X_deviation;
		}
	}
#else
	//General form:
	// y = a1 * x1, y = a2 * x2, ... y = an * xn
	//   vCoeffs_a = (a1, a2, ... an)
	//
	// dy = desired change in y
	//
	// dx1 = 1/a1 * dy, dx2 = 1/a2 * dy, ... dxn = 1/an * dy

	std::vector<double> vNew_X(parent_frame_dec.vPrevious_model_choice_double_value.size(), 0.0);
	for(int i=0; i<(int)vNew_X.size(); i++) {
		if(parent_frame_dec.vPrevious_model_choice_double_value[i] < 0.0) //first time
			vNew_X[i] = 0.0; //start with most complex
		else {
			vNew_X[i] = parent_frame_dec.vPrevious_model_choice_double_value[i] - 1.0 / parent_frame_dec.vCoeffs_a[i] * Y_failure_delta;

			double X_deviation = fabs(vNew_X[i] - parent_frame_dec.vPrevious_model_choice_double_value[i]);
			parent_frame_dec.vAverage_X_deviation[i] = ( parent_frame_dec.vAverage_X_deviation[i] * (parent_frame_dec.current_window_length_X_deviation - 1)
											+ X_deviation ) / parent_frame_dec.current_window_length_X_deviation;
			
			parent_frame_dec.vSum_X_deviation[i] = parent_frame_dec.vSum_X_deviation[i] * deviation_weight + X_deviation;

			parent_frame_dec.vDeflectionInX_during_onesided_failure[i] += X_deviation;
		}
	}
	
#endif

	std::vector<double> vNew_X_bounded = vNew_X;
	for(int i=0; i<(int)vNew_X_bounded.size(); i++) {
		if(vNew_X_bounded[i] < 0.0)
			vNew_X_bounded[i] = 0.0;
		if(vNew_X_bounded[i] > (int)vVarPriority.at(i).size() - 1)
			vNew_X_bounded[i] = vVarPriority[i].size() - 1;
	}

	std::vector<int> vNewDecisionValues;
	for(int i=0; i<(int)vNew_X_bounded.size(); i++)
		vNewDecisionValues.push_back( int(vNew_X_bounded[i] + 0.5) );

	
	std::vector<bool> vX_StuckAtBoundary(vNew_X.size(), false);
		//indicates for each variable whether it has already exceeded a bound previously, and continues to be stuck at bound this time
	for(int i=0; i<(int)vNew_X.size(); i++) {
		if(
			(parent_frame_dec.vUnbounded_Previous_model_choice_double_value[i] < 0.0 && vNew_X[i] < 0.0)           //stuck below lower bound
			|| (parent_frame_dec.vUnbounded_Previous_model_choice_double_value[i] > vVarPriority.at(i).size() - 1
					&& vNew_X[i] > vVarPriority.at(i).size() - 1)                                                  //stuck above upper bound
		)
		{ vX_StuckAtBoundary[i] = true; }
	}

	bool bAll_X_StuckAtBoundary = true;
	for(int i=0; i<(int)vX_StuckAtBoundary.size(); i++)
		bAll_X_StuckAtBoundary = bAll_X_StuckAtBoundary && vX_StuckAtBoundary[i];

	if(bAll_X_StuckAtBoundary == false) {
		if(Y_failure_delta > 0 && parent_frame_dec.current_failure_unidirectional_runlenth >= 0) //+ve add to run-magnitude
			parent_frame_dec.current_failure_unidirectional_runlenth++;
		else if(Y_failure_delta < 0 && parent_frame_dec.current_failure_unidirectional_runlenth <= 0) //-ve add to run-magnitude
			parent_frame_dec.current_failure_unidirectional_runlenth--;
		else //current_failure_unidirectional_runlenth != 0 and of opposite sign to Y_failure_delta => end of previous run, start of new run
		{
			parent_frame_dec.current_number_unidirectional_runs++;
			parent_frame_dec.average_continuous_unidirectional_failure_runlength =
				( parent_frame_dec.average_continuous_unidirectional_failure_runlength * (parent_frame_dec.current_number_unidirectional_runs - 1)
					+ fabs(parent_frame_dec.current_failure_unidirectional_runlenth) ) / parent_frame_dec.current_number_unidirectional_runs;
			parent_frame_dec.current_failure_unidirectional_runlenth = 1; //since this failure was first leg of run in opposite direction
		}
	}
	else //bAll_X_StuckAtBoundary == true, Range Saturation occured for all variables
	{
		if(parent_frame_dec.current_failure_unidirectional_runlenth != 0) { //a run was on previously
			parent_frame_dec.current_number_unidirectional_runs++;
			parent_frame_dec.average_continuous_unidirectional_failure_runlength =
				( parent_frame_dec.average_continuous_unidirectional_failure_runlength * (parent_frame_dec.current_number_unidirectional_runs - 1)
					+ fabs(parent_frame_dec.current_failure_unidirectional_runlenth) ) / parent_frame_dec.current_number_unidirectional_runs;
			parent_frame_dec.current_failure_unidirectional_runlenth = 0; //since still saturated, this failure makes no contribution to a run
		}
	}

	

	std::cout << "fast_reaction_strategy_choice_int_value(): previous FAILURE: Y_failure_delta = " << Y_failure_delta
			<< " vPrevious_model_choice_double_value = " << vector_print_string(parent_frame_dec.vPrevious_model_choice_double_value)
			<< " vNew_X = " << vector_print_string(vNew_X) << " vNewDecisionValues = " << vector_print_string(vNewDecisionValues)
			<< std::endl
			<< "fast_reaction_strategy_choice_int_value(): vAverage_X_deviation = " << vector_print_string(parent_frame_dec.vAverage_X_deviation)
			<< " vSum_X_deviation = " << vector_print_string(parent_frame_dec.vSum_X_deviation)
			<< " current_window_length_X_deviation = " << parent_frame_dec.current_window_length_X_deviation
			<< std::endl
			<< " fast_reaction_strategy_choice_int_value(): average_continuous_unidirectional_failure_runlength = " << parent_frame_dec.average_continuous_unidirectional_failure_runlength
			<< " current_failure_unidirectional_runlenth = " << parent_frame_dec.current_failure_unidirectional_runlenth
			<< " current_number_unidirectional_runs = " << parent_frame_dec.current_number_unidirectional_runs
			<< std::endl;

	std::cout << "fast_reaction_strategy_choice_int_value(): contiguous_onesided_failure_runlength = " << parent_frame_dec.contiguous_onesided_failure_runlength
			<< " vDeflectionInX_during_onesided_failure = " << vector_print_string(parent_frame_dec.vDeflectionInX_during_onesided_failure)
			<< " correction_runlength_onesided_failure = " << parent_frame_dec.correction_runlength_onesided_failure
			<< " correction_vDeflectionInX_during_onesided_failure = " << vector_print_string(parent_frame_dec.correction_vDeflectionInX_during_onesided_failure)
			<< std::endl;


	//Update System-Model Parameters, if needed
	std::vector<double> vRescale_X_factors( parent_frame_dec.vPrevious_model_choice_double_value.size(), 0.0 );
	std::string rescale_cause = "";
	for(int i=0; i<(int)parent_frame_dec.vSum_X_deviation.size(); i++) { //RESCALING for RANGE PRECISION
		//if(parent_frame_dec.vSum_X_deviation[i] > deviation_geometric_convergence * vVarPriority.at(i).size()) //excessive deviation accumulated
		if(parent_frame_dec.vSum_X_deviation[i] > deviation_geometric_convergence * 1.0) //excessive deviation accumulated
		{
			double rescale_factor = 0.0;
			//if(parent_frame_dec.vAverage_X_deviation[i] >= vVarPriority.at(i).size()) //variation is end-to-end, variations saturate range => reduce variation in X
			if(parent_frame_dec.vAverage_X_deviation[i] >= 1.0) //variation is end-to-end, variations saturate range => reduce variation in X
			{
				rescale_factor = parent_frame_dec.vAverage_X_deviation[i];
					//reduce average variation down to size of 1.0
			}
			//else: deviation is accumuluting due to continuous recent failures, but failures are not due to incorrect range scaling

			if(rescale_factor != 0) {
				vRescale_X_factors[i] = rescale_factor;
				rescale_cause = "RANGE-PRECISION";
			}
		}
	}

#if 0
	if(overall_rescale_factor == 0.0) { //RESCALING for RESPONSIVENESS
		if(parent_frame_dec.average_continuous_unidirectional_failure_runlength >= 2.0) {
			overall_rescale_factor = 1.0 / parent_frame_dec.average_continuous_unidirectional_failure_runlength;
		}
		overall_rescale_factor = 0.0; //FIXME: HACK!
	}
#endif
	if(rescale_cause == "") { //RESCALING for RESPONSIVENESS
		if(parent_frame_dec.correction_runlength_onesided_failure > parent_frame_dec.sliding_window_size) {
			double num_failure_sliding_windows = parent_frame_dec.correction_runlength_onesided_failure / parent_frame_dec.sliding_window_size;

			double min_rescale_factor = 0.0;
			int min_rescale_var_pos = -1;
			for(int i=0; i<(int)parent_frame_dec.correction_vDeflectionInX_during_onesided_failure.size(); i++) {
				double X_distortion_per_sliding_window = parent_frame_dec.correction_vDeflectionInX_during_onesided_failure[i] / num_failure_sliding_windows;
				double local_rescale_factor = 0.0;
				if(X_distortion_per_sliding_window < 1.0)
					local_rescale_factor = X_distortion_per_sliding_window / 1.0;

				if(local_rescale_factor != 0) {
					if(min_rescale_var_pos == -1 || local_rescale_factor < min_rescale_factor) {
						min_rescale_var_pos = i;
						min_rescale_factor = local_rescale_factor;
					}
				}
			}
			if(min_rescale_var_pos != -1) {
				vRescale_X_factors.at(min_rescale_var_pos) = min_rescale_factor;
				rescale_cause = "RESPONSIVENESS";
			}
			//overall_rescale_factor = parent_frame_dec.sliding_window_size / (double)parent_frame_dec.correction_runlength_onesided_failure;
		}
	}

	if(rescale_cause == "") { //RESCALING for CONTROL-LAG
		double objective_window_height = parent_frame_dec.mean_objective * (parent_frame_dec.window_frac_lower + parent_frame_dec.window_frac_upper);
		double threshold = deviation_geometric_convergence_short * objective_window_height * 1.0;
		double rescale_factor = 0.0;
		if(parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window > threshold) 
		{ rescale_factor = parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window / threshold; }
		if(rescale_factor < 1.2)
			rescale_factor = 0.0;

		if(rescale_factor != 0.0) {
			vRescale_X_factors.clear();
			vRescale_X_factors.resize( parent_frame_dec.vPrevious_model_choice_double_value.size(), rescale_factor );
			rescale_cause = "CONTROL-LAG";
		}
	}

	if(bForceFixedCoeff_in_FastReactionStrategy == true)
		rescale_cause = "";

	if(
		((rescale_cause == "RANGE-PRECISION" || rescale_cause == "CONTROL-LAG") && parent_frame_dec.current_window_length_X_deviation < parent_frame_dec.sliding_window_size) //RANGE PRECISION or CONTROL-LAG
		//|| (overall_rescale_factor < 1.0 && parent_frame_dec.current_number_unidirectional_runs < parent_frame_dec.sliding_window_size) //RESPONSIVENESS
	) //wait for atleast one period before considering rescaling => o.w., a single outlier behavior can mess up estimation of rescaling parameters
	{
		vRescale_X_factors.clear();
		vRescale_X_factors.resize( parent_frame_dec.vPrevious_model_choice_double_value.size(), 0.0 );
		rescale_cause = "";
	}

	if(rescale_cause != "") {
		for(int i=0; i<(int)parent_frame_dec.vCoeffs_a.size(); i++) {
			if(vRescale_X_factors[i] != 0.0)
				parent_frame_dec.vCoeffs_a[i] *= vRescale_X_factors[i];
			parent_frame_dec.vAverage_X_deviation[i] = 0.0;
			parent_frame_dec.vSum_X_deviation[i] = 0.0;
		}
		parent_frame_dec.current_window_length_X_deviation = 0;

		parent_frame_dec.current_failure_unidirectional_runlenth = 0;
		parent_frame_dec.average_continuous_unidirectional_failure_runlength = 0.0;
		parent_frame_dec.current_number_unidirectional_runs = 0;

		parent_frame_dec.halfcycle_start_deflection_sign = (parent_frame_dec.previous_invocation_exec_time <= parent_frame_dec.mean_objective ? -1 : +1);
		parent_frame_dec.halfcycle_Y_positive_max_deflection = 0.0;
		parent_frame_dec.halfcycle_Y_negative_max_deflection = 0.0;
		parent_frame_dec.halfcycle_length = 0;
		parent_frame_dec.has_halfcycle_crossed_mean = false;
		parent_frame_dec.sum_of_weighted_quantity_of_halfcycles_completed_per_sliding_window = 0.0;

		parent_frame_dec.contiguous_onesided_failure_runlength = 0;
		for(int i=0; i<(int)parent_frame_dec.vDeflectionInX_during_onesided_failure.size(); i++)
			parent_frame_dec.vDeflectionInX_during_onesided_failure[i] = 0.0;
		parent_frame_dec.correction_runlength_onesided_failure = 0;
		for(int i=0; i<(int)parent_frame_dec.correction_vDeflectionInX_during_onesided_failure.size(); i++)
			parent_frame_dec.correction_vDeflectionInX_during_onesided_failure[i] = 0.0;

		std::cout << "fast_reaction_strategy_choice_int_value: RESCALING vCoeffs_a by vRescale_X_factors = " << vector_print_string(vRescale_X_factors)
			<< " for " << rescale_cause << "  NEW vCoeffs_a = " << vector_print_string(parent_frame_dec.vCoeffs_a)
			<< std::endl;
	}

	for(int i=0; i<(int)parent_frame_dec.vPrevious_model_choice_double_value.size(); i++) {
		parent_frame_dec.vPrevious_model_choice_double_value[i] = vNew_X_bounded[i];
		parent_frame_dec.vUnbounded_Previous_model_choice_double_value[i] = vNew_X[i];
	}

	return convert_decision_vector_to_int( vNewDecisionValues );
}


void ExecFrameInfo::run() {
	curr_parent_frame = get_innermost_executing_frame();
	
	ExecFrameDecisionModel& execframe_dec_model = this->decision_model;
	if(curr_parent_frame != 0) {
		if(execframe_dec_model.decision_vector_parameter.has_consumer(curr_parent_frame) == false) {
			execframe_dec_model.decision_vector_parameter.add_consumer(curr_parent_frame);
			//execframe_dec_model.exec_time_parameter.add_consumer(curr_parent_frame); //FIXME
		}
	}

	//check if curr_parent_frame != 0, and this is not first invocation
	//   of current execframe since curr_parent_frame was last Activated.
	//   if so => use previously cached decision-vector, else recompute it
	int decision_vector_int_value = -1;

#if 0
	bool bWillUseForcedDefaultSelectChoice = bForceDefaultSelectChoice
			&& (model.type() == Model::Select && model.access_default_choice_index_for_select_var_id() != -1);
		//FIXME: Assuming model can be a Select model only at the top level.
		//  More general fix may require modifications to get_highest_priority_order_decision_vector(),
		//    get_next_lower_priority_order_decision_vector() and is_lowest_priority_order_decision_vector().
	if(bWillUseForcedDefaultSelectChoice) {
		assert( vDecisionVector.size() == 1
			&& 0 <= model.access_default_choice_index_for_select_var_id()
			&& model.access_default_choice_index_for_select_var_id() < (int)vVarPriority.at(0).size() ); //FIXME: see above FIXME
		decision_vector_int_value = model.access_default_choice_index_for_select_var_id();
		std::cout << "ExecFrameInfo::run() : ExecFrame #" << my_execframe->id << " Forced DEFAULT Select model choice decision_vector_int_value = " << decision_vector_int_value << std::endl;
	}
#else
	if(bForceDefaultSelectChoice) {
		
		//check if all Select sub-models have a default choice. Either all should have a default choice, or non should have, o.w. error
		int num_default_choices = 0;
		for(int i=0; i<(int)vDefaultChoice_DecisionValues.size(); i++) {
			if(vDefaultChoice_DecisionValues[i] != -1)
				num_default_choices++;
		}
		assert(num_default_choices == 0 || num_default_choices == (int)vDecisionVector.size());

		//FIXME: Different Select sub-models should be allowed to independently choose to have a default choice or not
		//  More general fix may require modifications to get_highest_priority_order_decision_vector(),
		//    get_next_lower_priority_order_decision_vector() and is_lowest_priority_order_decision_vector().

		if(num_default_choices > 0) {
			decision_vector_int_value = convert_decision_vector_to_int( vDefaultChoice_DecisionValues );
			std::cout << "ExecFrameInfo::run() : ExecFrame #" << my_execframe->id << " Forced DEFAULT for Select sub-models choice decision_vector_int_value = " << decision_vector_int_value << std::endl;
		}
	}
#endif

#if 0
	//attempt to re-use a previously cached decision-vector
	if(bWillUseForcedDefaultSelectChoice == false && curr_parent_frame != 0) {
		FrameInfo * parent_frame_info = FrameInfo::get_frame_info(curr_parent_frame);
		FrameDecisionModel& parent_dec_model = parent_frame_info->decision_model;
		Parameter * ptr_decision_vector_parameter = &(decision_model.decision_vector_parameter);

		assert(parent_dec_model.map_parm_to_curr_record.count(ptr_decision_vector_parameter) > 0);

		IntValueCache * ptr_value_cache
			= parent_dec_model.map_parm_to_curr_record[ptr_decision_vector_parameter];

		for(int i=0; i<(int)ptr_value_cache->vCacheEntries.size(); i++) {
			IntCacheEntry& ce = ptr_value_cache->vCacheEntries[i];
			if(ce.valid) { //found one
				decision_vector_int_value = ce.tag;
				std::cout << "ExecFrameInfo::run() : ExecFrame #" << my_execframe->id << " REUSING decision_vector_int_value = " << decision_vector_int_value << std::endl;
				break;
			}
		}
	}
#endif

	if(stickiness_runlength_remaining > 0) {
		decision_vector_int_value = sticky_decision_vector_int_val;
		stickiness_runlength_remaining--;
		std::cout << "ExecFrameInfo::run() : ExecFrame #" << my_execframe->id << " STICKY decision_vector_int_value = " << decision_vector_int_value << " with stickiness_runlength_remaining = " << stickiness_runlength_remaining << std::endl;
	}
	
	if(decision_vector_int_value == -1) { //re-usable decision-vector not found
		decision_vector_int_value = choose_decision_vector_int_value();
		std::cout << "ExecFrameInfo::run() : ExecFrame #" << my_execframe->id << " NEW decision_vector_int_value = " << decision_vector_int_value << std::endl;
	}
	std::vector<int> decision_vector_values_to_run = convert_int_to_decision_vector(decision_vector_int_value);


	//Run
	timeval start_timeval = get_curr_timeval();
	RunModel::run_model_on_decision_vector(model, vDecisionVector, decision_vector_values_to_run); //run user-code
	timeval end_timeval = get_curr_timeval();


	//update Parameters
	ExecTime_t consumed_time = diff_time(start_timeval, end_timeval);
	int consumed_time_int_bin = decision_model.convert_exec_time_to_int_bin(consumed_time);
		//FIXME: update exec_time_parameter
	
	if(curr_parent_frame != 0) {
		std::vector<Frame *> vActiveParents = get_dynamically_enclosing_frames(curr_parent_frame);
		vActiveParents.insert(vActiveParents.begin(), curr_parent_frame);

		decision_model.decision_vector_parameter.inform_enclosing_active_consumers_of_sample_measurement(
			vActiveParents, decision_vector_int_value);
	}

}


///////////////////////

void extract_decision_vector(
	Model model,
	std::vector<int>& result_vDecisionVector,
	std::vector< std::vector<int> >& result_vVarPriority,
	std::vector<int>& result_vDefaultChoice_DecisionValues,
	std::vector<double>& result_vInitialCoeffs_fast_reaction_strategy
) {
	result_vDecisionVector.clear();
	result_vVarPriority.clear();
	result_vDefaultChoice_DecisionValues.clear();
	result_vInitialCoeffs_fast_reaction_strategy.clear();

	std::list<Model *> queue;

	//Perform Breadth-First-Search into model
	queue.push_back(&model);

	while(queue.empty() == false) {
		Model * curr_model = queue.front();
		queue.pop_front();

		switch(curr_model->type()) {
		case Model::None:
			{ break; }
		case Model::Binder:
			{ break; }
		case Model::Sequence:
			{
				std::vector<Model>& modelList = curr_model->access_modelList();
				for(int i=0; i<(int)modelList.size(); i++) {
					queue.push_back( &(modelList[i]) );
				}
				break;
			}
		case Model::Select:
			{
				std::vector<Model>& modelList = curr_model->access_modelList();
				for(int i=0; i<(int)modelList.size(); i++) {
					queue.push_back( &(modelList[i]) );
				}

				int select_var_id = curr_model->access_select_var_id();
				int default_choice_index_for_select_var_id = curr_model->access_default_choice_index_for_select_var_id();
				double fast_reaction_strategy_coeff = curr_model->access_fast_reaction_strategy_coeff();
				int found_loc = -1; //check if this variable is a repeat
				for(int i=0; i<(int)result_vDecisionVector.size(); i++) {
					if(result_vDecisionVector[i] == select_var_id) {
						found_loc = i;
						break;
					}
				}

				std::vector<int>& select_priority = curr_model->access_select_priority();
				assert(select_priority.size() == 0 || select_priority.size() == modelList.size());

				if(found_loc != -1) { //select variable is a repeat
					if(result_vVarPriority.at(found_loc).size() != modelList.size()) {
						std::cerr << "extract_decision_vector(): ERROR:"
								<< " model contains unequal selection sizes across"
								<< "\n   multiple occurences of select_var_id = " << select_var_id
								<< std::endl;
						exit(1);
					}

					if(select_priority.size() > 0) { //priorities given by current model
						bool bHasPreviouslySpecifiedPriorities = false;
						for(int i=0; i<(int)result_vVarPriority.at(found_loc).size(); i++) {
							if(result_vVarPriority[found_loc][i] != 0)
								bHasPreviouslySpecifiedPriorities = true;
						}

						if(bHasPreviouslySpecifiedPriorities) {
							//Verify that current do not conflict with previous
							for(int i=0; i<(int)select_priority.size(); i++) {
								if(select_priority[i] != result_vVarPriority[found_loc].at(i)) {
									std::cerr << "extract_decision_vector(): ERROR:"
											<< " model has conflicting priorities"
											<< "\n    for select_var_id = " << select_var_id
											<< std::endl;
									exit(1);
								}
							}
						}
						else //previously unspecified priorities (all 0's), define from current
						{ result_vVarPriority.push_back(select_priority); }
					}

					if(default_choice_index_for_select_var_id != -1) {
						if(result_vDefaultChoice_DecisionValues.at(found_loc) != -1
							&& default_choice_index_for_select_var_id != result_vDefaultChoice_DecisionValues.at(found_loc)
						) {
							std::cerr << "extract_decision_vector(): ERROR: model has conflicting default choices for select_var_id = "
								<< select_var_id << std::endl;
							exit(1);
						}
					}

					if(fast_reaction_strategy_coeff != 0.0) {
						if(result_vInitialCoeffs_fast_reaction_strategy.at(found_loc) != 0.0
							&& fast_reaction_strategy_coeff != result_vInitialCoeffs_fast_reaction_strategy.at(found_loc)
						) {
							std::cerr << "extract_decision_vector(): ERROR: model has conflicting fast_reaction_strategy_coeff for select_var_id = "
								<< select_var_id << std::endl;
							exit(1);
						}
					}
				}
				else { //first occurence of select variable
					found_loc = (int)result_vDecisionVector.size();
					result_vDecisionVector.push_back(select_var_id);
					result_vDefaultChoice_DecisionValues.push_back(default_choice_index_for_select_var_id);
					result_vInitialCoeffs_fast_reaction_strategy.push_back(fast_reaction_strategy_coeff);

					if(select_priority.size() == 0) {
						result_vVarPriority.push_back( std::vector<int>(modelList.size(), 0) );
						//equal priorities of 0
					}
					else
					{ result_vVarPriority.push_back(select_priority); }
				}

				if(default_choice_index_for_select_var_id != -1
					&& (default_choice_index_for_select_var_id < 0 || default_choice_index_for_select_var_id >= (int)result_vVarPriority.at(found_loc).size())
				) {
					std::cerr << "extract_decision_vector(): ERROR: model has invalid default choice for: "
							<< " select_var_id = " << select_var_id << " default choice = " << default_choice_index_for_select_var_id
							<< std::endl;
					exit(1);
				}

				if(fast_reaction_strategy_coeff < 0.0) {
					std::cerr << "extract_decision_vector(): ERROR: model has invalid fast_reaction_strategy_coeff for: "
							<< " select_var_id = " << select_var_id << " fast_reaction_strategy_coeff = " << fast_reaction_strategy_coeff
							<< " must be 0.0 (to ignore) or POSITIVE to specify"
							<< std::endl;
					exit(1);
				}

				break;
			}
		default:
			{
				assert(0);
				break;
			}
		}
	}
}



/////////////////////////////
//class RunModel definitions
/////////////////////////////

void RunModel::run_model_on_decision_vector(
	const Model& model,
	const std::vector<int>& vDecisionVector_variableIDs,
	const std::vector<int>& vDecisionVector_values
)
{
	switch(model.type()) {
		case Model::None: {
			break;
		}

		case Model::Binder: {
			if(model.caller->func == 0) {
				std::cerr << "RunModel: ERROR:: model has a Caller not bound to Func" << std::endl;
				assert(0); //to allow stack to be printed to facilitate debugging of which ExecFrame in user-code is responsible
				exit(1);
			}
			model.caller->func->eval();
			delete model.caller->func;
			model.caller->func = 0;

			break;
		}

		case Model::Sequence: {
			for(int i=0; i<(int)model.modelList.size(); i++)
				run_model_on_decision_vector(
					model.modelList[i], vDecisionVector_variableIDs, vDecisionVector_values);

			break;
		}

		case Model::Select: {
			int variable_index = -1;
			for(int i=0; i<(int)vDecisionVector_variableIDs.size(); i++) {
				if(vDecisionVector_variableIDs[i] == model.select_var_id) {
					variable_index = i;
					break;
				}
			}
			assert(variable_index != -1);

			int choice_value = vDecisionVector_values.at(variable_index);
			run_model_on_decision_vector(
				model.modelList.at(choice_value), vDecisionVector_variableIDs, vDecisionVector_values);

			break;
		}

		default: {
			assert(0);
			break;
		}
	}
}


} //namespace Opp
