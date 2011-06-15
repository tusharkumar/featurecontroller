// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_DEBUG_CONTROL_H
#define OPP_DEBUG_CONTROL_H

namespace Opp {

	//Features: Control Settings
	
	void feature_control_MagnifyCount_by_SuccessFailure_DeviationDegree(bool new_setting);
		//Sets whether to magnify the occurence of an Objective Success or Failure based on the
		//  degree of deviation from ideal behavior
		//  (i.e., from window center of Success, and from window edges on Failure).
		//Enabling this feature causes SRT to react faster to larger failures and to favor choices
		//  that meet the Objective more closely, at the risk of shortening the retained history of behavior.
		//
		//Default setting = true

	bool feature_query_MagnifyCount_by_SuccessFailure_DeviationDegree();
		//Returns current set value for the bMagnifyCount_by_SuccessFailure_DeviationDegree feature



	void feature_control_probability_of_exploration(double new_setting);
		//Enables probabilistic exploration of decision-values in an ExecFrame when vForDecisionSet is empty.
		//This is useful for avoiding lock-step exploration across multiple models within same Frame.
		//
		//probability_of_exploration = 0.0 disables probabilistic exploration.
		//probability_of_exploration > 0.0 is the probability with which a valid decision-value
		//  found in descending priority order will be rejected, and the next value tried.
		//
		//Default setting = 0.0

	double feature_query_probability_of_exploration();
		//Returns current value for probability_of_exploration


	void feature_control_DeemphasizeHistoryWithAlphaRate(bool enable, double alpha_rate);
		//The sample-counts in a Frame's history are scaled down by 'alpha_rate' ratio after
		//  each completion of a frame execution. This de-emphasizes history, causing it
		//  to be slowly forgotten unless re-inforced with new experiences.
		//
		//Default setting = (true, 0.99)
	
	std::pair<bool, double> feature_query_DeemphasizeHistoryWithAlphaRate();
		//Returns current value for (enable, deemphasize_history_rate_alpha)
	

	void feature_control_ForgetHistoryBelowBetaThreshold(bool enable, double beta_ratio);
		//Individual cache-entries in bins across each Parameter-Exec-Spread are deleted
		//  if their counts are below a 'beta_ratio' multiple of
		//  the Parameter-Exec-Spread's total-sample-count.
		//
		//Default Setting = (true, 0.001)

	std::pair<bool, double> feature_query_ForgetHistoryBelowBetaThreshold();
		//Returns current value for (enable, forget_history_threshold_ratio_beta)
	


	void feature_control_use_fast_reaction_strategy(bool new_setting);
		//Use Fast Reaction Strategy (Linear Control System) instead of Reinforcement Learning

	bool feature_query_use_fast_reaction_strategy();


	//Debug Messages: Levels

		//controls not added yet
}

#endif //OPP_DEBUG_CONTROL_H
