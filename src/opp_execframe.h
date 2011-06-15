// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_EXECFRAME_H
#define OPP_EXECFRAME_H

#include <vector>
#include <iostream>
#include <algorithm>
#include "opp.h"
#include "opp_decision_model.h"

namespace Opp {

	ExecFrame * get_execframe_from_execframe_id(FrameID_t execframe_id);

	void extract_decision_vector(
		Model model,
		std::vector<int>& result_vDecisionVector,
		std::vector< std::vector<int> >& result_vVarPriority,
		std::vector<int>& result_vDefaultChoice_DecisionValues,
		std::vector<double>& result_vInitialCoeffs_fast_reaction_strategy
	);
	//extracts the vector-of-decision-variable-ids, the
	//  vector-of-max-values taken by corresponding variable-ids,
	//  and the priority of each choice for each variable

	class ExecFrameInfo {
	public:
		ExecFrame * my_execframe;
		ExecFrameDecisionModel decision_model;

		Model model;
		std::vector<int> vDecisionVector;
			//variable-ids
		std::vector< std::vector<int> > vVarPriority;
			//for each variable, identifies the priority of each possible selection.
			//[0 .. vVarPriority[var_id_index].size()-1] is the range of selection values
			// allowed.
		std::vector<int> vDefaultChoice_DecisionValues;
			//for each variable, identifies its default choice (-1 if none provided)
		std::vector<double> vInitialCoeffs_fast_reaction_strategy;
			//for each variable, provides an initial coefficient value to be used if Fast Reaction Strategy is used

		bool bForceDefaultSelectChoice;

		bool bForceFixedCoeff_in_FastReactionStrategy;
			//Suppresses rescaling of coefficients

		Frame * curr_parent_frame;
			//curr_parent_frame = 0 for a top-level frame.
			//Defined only while the execframe is executing.

		int stickiness_runlength_remaining;
		int sticky_decision_vector_int_val;

		ExecFrameInfo(ExecFrame * my_execframe, const Model& model)
			: my_execframe(my_execframe), decision_model(my_execframe),
				model(model), bForceDefaultSelectChoice(false), bForceFixedCoeff_in_FastReactionStrategy(false),
				curr_parent_frame(0), stickiness_runlength_remaining(0), sticky_decision_vector_int_val(-1)
		{
			extract_decision_vector(model, vDecisionVector, vVarPriority, vDefaultChoice_DecisionValues, vInitialCoeffs_fast_reaction_strategy);

			vVariable_SortedPairs_Priority_Value.resize(vVarPriority.size());
			for(int dec_var_index=0; dec_var_index<(int)vVarPriority.size(); dec_var_index++) {
				std::vector< std::pair<int, int> > vPairs_Priority_Value;
				for(int i=0; i<(int)vVarPriority[dec_var_index].size(); i++) {
					int value = i;
					int priority = vVarPriority[dec_var_index][i];
					vPairs_Priority_Value.push_back( std::make_pair(priority, value) );
				}
				assert((int)vPairs_Priority_Value.size() > 0);
				std::sort(vPairs_Priority_Value.begin(), vPairs_Priority_Value.end(), cmp_pair_on_priority);
				vVariable_SortedPairs_Priority_Value[dec_var_index] = vPairs_Priority_Value;
			}

			std::cout << "vVariable_SortedPairs_Priority_Value:" << std::endl;;
			for(int dec_var_index=0; dec_var_index<(int)vVariable_SortedPairs_Priority_Value.size(); dec_var_index++) {
				std::cout << "  [" << dec_var_index << "] = [";
				for(int i=0; i<(int)vVariable_SortedPairs_Priority_Value[dec_var_index].size(); i++) {
					std::cout << "(" << vVariable_SortedPairs_Priority_Value[dec_var_index][i].first
						<< ", " << vVariable_SortedPairs_Priority_Value[dec_var_index][i].second << ")";
				}
				std::cout << "]" << std::endl;
			}
		}

		void run();

		//returns -1 if select_var_id not found in vDecisionVector
		int find_index_of_select_var(int select_var_id) const {
			int found_loc = -1;
			for(int i=0; i<(int)vDecisionVector.size(); i++) {
				if(vDecisionVector[i] == select_var_id) {
					found_loc = i;
					break;
				}
			}
			return found_loc;
		}

		//being a friend of class ExecFrame
		static ExecFrameInfo * get_execframe_info(ExecFrame * execframe)
			{ return execframe->execframe_info; }


		// Conversion utilities between decision-vector-values and int-for-caching

		int convert_decision_vector_to_int(std::vector<int> vDecisionValues) const {
			assert(vDecisionValues.size() == vVarPriority.size());

			int int_val = 0;
			for(int i=0; i<(int)vDecisionValues.size(); i++) {
				int dim_size = vVarPriority[i].size();
				int_val *= dim_size;

				int dec_var_val = vDecisionValues[i];
				assert(0 <= dec_var_val && dec_var_val < (int)vVarPriority[i].size());
				int_val += dec_var_val;
			}
			return int_val;
		}

		std::vector<int> convert_int_to_decision_vector(int int_val) const {
			std::vector<int> dec_vec(vVarPriority.size(), 0);
			for(int i=(int)vVarPriority.size()-1; i >= 0; i--) {
				dec_vec[i] = int_val % ((int)vVarPriority[i].size());
				int_val /= (int)vVarPriority[i].size();
			}
			assert(int_val == 0);
			return dec_vec;
		}


		// Utilities related to priorities of decision-vectors

		int get_num_decision_vectors() const {
			int result = 1;
			for(int i=0; i<(int)vVarPriority.size(); i++)
				result *= (int)vVarPriority[i].size();
			return result;
		}

		std::vector<int> get_highest_priority_order_decision_vector() const {
			std::vector<int> dec_vec;
			for(int i=0; i<(int)vVariable_SortedPairs_Priority_Value.size(); i++)
				dec_vec.push_back( vVariable_SortedPairs_Priority_Value[i][0].second );
			return dec_vec;
		}

		bool is_lowest_priority_order_decision_vector(const std::vector<int>& dec_vec) const {
			assert(dec_vec.size() == vVariable_SortedPairs_Priority_Value.size());

			for(int i=0; i<(int)dec_vec.size(); i++) {
				const std::vector< std::pair<int, int> >& variable_vSortedPairs
					= vVariable_SortedPairs_Priority_Value[i];
				if(dec_vec[i] != variable_vSortedPairs[variable_vSortedPairs.size()-1].second)
					return false; //does not equal the lowest priority order value for this variable
			}
			return true;
		}

		std::vector<int> get_next_lower_priority_order_decision_vector(const std::vector<int>& dec_vec) const {
			assert(dec_vec.size() == vVariable_SortedPairs_Priority_Value.size());
			std::vector<int> next_dec_vec;

			bool continue_to_upper_variable = true;
			for(int i=(int)vVariable_SortedPairs_Priority_Value.size()-1; i >= 0; i--) {
				if(continue_to_upper_variable == false)
					break;
				assert(0 <= dec_vec[i] && dec_vec[i] < (int)vVarPriority[i].size());
				int found_value_loc = -1;
				for(int j=0; j<(int)vVariable_SortedPairs_Priority_Value[i].size(); j++) {
					if(vVariable_SortedPairs_Priority_Value[i][j].second == dec_vec[i]) {
						found_value_loc = j;
						break;
					}
				}
				assert(found_value_loc != -1);
				int next_value = -1;
				if(found_value_loc+1 < (int)vVariable_SortedPairs_Priority_Value[i].size()) {
					next_value = vVariable_SortedPairs_Priority_Value[i][found_value_loc+1].second;
					continue_to_upper_variable = false;
				}
				else {
					next_value = vVariable_SortedPairs_Priority_Value[i][0].second; //cycle around
					continue_to_upper_variable = true;
				}
				next_dec_vec.push_back(next_value);
			}
			if(continue_to_upper_variable == true) //have cycled through all variables
				return dec_vec; //this was already lowest-value, repeat to signal end

			return next_dec_vec;
		}

			//compares priorities; returns -1 when dec1 < dec2, 0 when dec1 == dec2, +1 when dec1 > dec2
		int cmp_decision_vectors_on_priority(
			const std::vector<int>& dec1,
			const std::vector<int>& dec2
		) const
		{
			assert(dec1.size() == vVarPriority.size());
			assert(dec2.size() == vVarPriority.size());

			for(int i=0; i<(int)dec1.size(); i++) {
				assert(0 <= dec1[i] && dec1[i] < (int)vVarPriority[i].size());
				assert(0 <= dec2[i] && dec2[i] < (int)vVarPriority[i].size());

				int priority1 = vVarPriority[i][ dec1[i] ];
				int priority2 = vVarPriority[i][ dec2[i] ];

				if(priority1 < priority2)
					return -1;
				if(priority1 > priority2)
					return +1;
				//else, defer priority comparision to (i+1)th variable
			}

			return 0; //both have equal priority across all variables in decision-vector
		}


	private:
		std::vector< std::vector< std::pair<int, int> > > vVariable_SortedPairs_Priority_Value;
			//for corresponding variable in vDecisionVector, pairs of <priority, variable-value>
			//  sorted from highest-priority value down to lowest

		static bool cmp_pair_on_priority(const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
			int priority1 = p1.first;
			int priority2 = p2.first;

			if(priority1 < priority2)
				return true;
			else
				return false;
		}

		int choose_decision_vector_int_value();

		int fast_reaction_strategy_choice_int_value();
	};

	class RunModel {
	public:
		static void run_model_on_decision_vector(
			const Model& model,
			const std::vector<int>& vDecisionVector_variableIDs,
			const std::vector<int>& vDecisionVector_values
		);
	};

} //namespace Opp

#endif //OPP_EXECFRAME_H
