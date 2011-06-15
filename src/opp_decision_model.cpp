// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <algorithm>
#include <iostream>

#include "opp_frame_info.h"
#include "opp_frame.h"
#include "opp_decision_model.h"

#include "opp_debug_control.h"

namespace Opp {

/////////////////////////////
//class Parameter definitions
/////////////////////////////

long long Parameter::parameter_count = 0;


bool Parameter::has_consumer(Frame * consumer) {
	FrameInfo * consumer_frame_info = FrameInfo::get_frame_info(consumer);
	FrameDecisionModel& consumer_frame_dec_model = consumer_frame_info->decision_model;

	return (consumer_frame_dec_model.map_parm_to_spread.count(this) > 0);
}

void Parameter::add_consumer(Frame * consumer) {
	FrameInfo * consumer_frame_info = FrameInfo::get_frame_info(consumer);
	FrameDecisionModel& consumer_frame_dec_model = consumer_frame_info->decision_model;

	assert(consumer_frame_dec_model.map_parm_to_spread.count(this) == 0);
		//should not already have this frame as consumer
	
	consumer_frame_dec_model.map_parm_to_spread[this]
		= new ParameterExecSpread(consumer_frame_dec_model.exec_time_parameter_num_spread_bins);

	IntValueCache * exec_time_parameter_cache = new IntValueCache(
				FrameDecisionModel::exec_time_parameter_cache_num_entries,
				FrameDecisionModel::exec_time_parameter_cache_max_count
			);
	consumer_frame_dec_model.map_parm_to_curr_record[this] = exec_time_parameter_cache;

	map_consumer_caches[consumer] = exec_time_parameter_cache;
}


void Parameter::remove_consumer(Frame * consumer) {
	FrameInfo * consumer_frame_info = FrameInfo::get_frame_info(consumer);
	FrameDecisionModel& consumer_frame_dec_model = consumer_frame_info->decision_model;

	assert(map_consumer_caches.count(consumer) == 1);
	map_consumer_caches.erase(consumer);
		//IntValueCache * deleted below by consumer_frame_dec_model.map_parm_to_curr_record


	assert(consumer_frame_dec_model.map_parm_to_spread.count(this) == 1);
	delete consumer_frame_dec_model.map_parm_to_spread[this];
	consumer_frame_dec_model.map_parm_to_spread.erase(this);

	assert(consumer_frame_dec_model.map_parm_to_curr_record.count(this) == 1);
	delete consumer_frame_dec_model.map_parm_to_curr_record[this];
	consumer_frame_dec_model.map_parm_to_curr_record.erase(this);
}


void Parameter::inform_enclosing_active_consumers_of_sample_measurement(
	std::vector<Frame *> vActiveEnclosingFrames,
	int sample_value
) {
	for(int i=0; i<(int)vActiveEnclosingFrames.size(); i++) {
		Frame * enclosing_consumer = vActiveEnclosingFrames[i];
		if(map_consumer_caches.count(enclosing_consumer) == 0)
			continue;
		assert(map_consumer_caches.count(enclosing_consumer) == 1);
		IntValueCache * tracking_parm_cache = map_consumer_caches[enclosing_consumer];
		tracking_parm_cache->note_sample(sample_value);
	}
}

////////////////////////////

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

void intersect_sorted_bin_vectors(
	const std::vector<int>& vInputSet1,
	const std::vector<double>& vInputSet1_Counts,
	const std::vector<double>& vInputSet1_Probs,
	const std::vector<int>& vInputSet2,
	const std::vector<double>& vInputSet2_Counts,
	const std::vector<double>& vInputSet2_Probs,
	std::vector<int>& vReturnSet,
	std::vector<double>& vReturnSet_Counts,
	std::vector<double>& vReturnSet_Probs
)
{
	//Assumptions: vInputSet1 and vInputSet2 are sorted in ascending order,
	//  and don't repeat elements within each vector

	assert(vInputSet1.size() == vInputSet1_Counts.size());
	assert(vInputSet1.size() == vInputSet1_Probs.size());
	assert(vInputSet2.size() == vInputSet2_Counts.size());
	assert(vInputSet2.size() == vInputSet2_Probs.size());

	vReturnSet.clear();
	vReturnSet_Counts.clear();
	vReturnSet_Probs.clear();

	int i1=0;
	int i2=0;

	while(i1 < (int)vInputSet1.size() && i2 < (int)vInputSet2.size()) {
		if(vInputSet1[i1] < vInputSet2[i2])
		{ i1++; }
		else if(vInputSet1[i1] == vInputSet2[i2]) { //both have it => value intersects
			vReturnSet.push_back(vInputSet1[i1]);
			vReturnSet_Counts.push_back(vInputSet1_Counts[i1] + vInputSet2_Counts[i2]);
			vReturnSet_Probs.push_back( MIN(vInputSet1_Probs[i1], vInputSet2_Probs[i2]) );
			i1++;
			i2++;
		}
		else //vInputSet1[i1] > vInputSet2[i2]
		{ i2++; }
	}
}

void union_sorted_bin_vectors(
	const std::vector<int>& vInputSet1,
	const std::vector<double>& vInputSet1_Counts,
	const std::vector<double>& vInputSet1_Probs,
	const std::vector<int>& vInputSet2,
	const std::vector<double>& vInputSet2_Counts,
	const std::vector<double>& vInputSet2_Probs,
	std::vector<int>& vReturnSet,
	std::vector<double>& vReturnSet_Counts,
	std::vector<double>& vReturnSet_Probs
)
{
	//Assumptions: vInputSet1 and vInputSet2 are sorted in ascending order,
	//  and don't repeat elements within each vector

	assert(vInputSet1.size() == vInputSet1_Counts.size());
	assert(vInputSet1.size() == vInputSet1_Probs.size());
	assert(vInputSet2.size() == vInputSet2_Counts.size());
	assert(vInputSet2.size() == vInputSet2_Probs.size());

	vReturnSet.clear();
	vReturnSet_Counts.clear();
	vReturnSet_Probs.clear();

	int i1=0;
	int i2=0;

	while(i1 < (int)vInputSet1.size() && i2 < (int)vInputSet2.size()) {
		if(vInputSet1[i1] < vInputSet2[i2]) { //only vInputSet1 has this value
			vReturnSet.push_back(vInputSet1[i1]);
			vReturnSet_Counts.push_back(vInputSet1_Counts[i1]);
			vReturnSet_Probs.push_back(vInputSet1_Probs[i1]);
			i1++;
		}
		else if(vInputSet1[i1] == vInputSet2[i2]) { //both have it
			vReturnSet.push_back(vInputSet1[i1]);
			vReturnSet_Counts.push_back(vInputSet1_Counts[i1] + vInputSet2_Counts[i2]);
			vReturnSet_Probs.push_back( MAX(vInputSet1_Probs[i1], vInputSet2_Probs[i2]) );
			i1++;
			i2++;
		}
		else {//vInputSet1[i1] > vInputSet2[i2]: only vInputSet2 has this value
			vReturnSet.push_back(vInputSet2[i2]);
			vReturnSet_Counts.push_back(vInputSet2_Counts[i2]);
			vReturnSet_Probs.push_back(vInputSet2_Probs[i2]);
			i2++;
		}
	}
	while(i1 < (int)vInputSet1.size()) {
		vReturnSet.push_back(vInputSet1[i1]);
		vReturnSet_Counts.push_back(vInputSet1_Counts[i1]);
		vReturnSet_Probs.push_back(vInputSet1_Probs[i1]);
		i1++;
	}
	while(i2 < (int)vInputSet2.size()) {
		vReturnSet.push_back(vInputSet2[i2]);
		vReturnSet_Counts.push_back(vInputSet2_Counts[i2]);
		vReturnSet_Probs.push_back(vInputSet2_Probs[i2]);
		i2++;
	}
}

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
)
{
	std::vector<Frame *> vActiveParents = get_dynamically_enclosing_frames(innermost_deciding_ancestor_frame);
	vActiveParents.insert(vActiveParents.begin(), innermost_deciding_ancestor_frame);

	//Steps:
	//1) Walk frames in vActiveParents top-down examining if ancestor consumes current
	//    ExecFrame's decision-vector parameter.
	//
	//2) If ancestor consumes, extract the most & least 'desirable' set of decision-vector
	//       integer-tag values
	//     (based on ancestor frame's current setting, set = empty if ancestor has no objective).
	//
	//3) If consuming ancestor has atleast one of most or least sets as non-empty (=> ancestor imposes preferences),
	//     then append sets as next entry of vMostDesirableDecisionSets & vLeastDesirableDecisionSets
	//
	//     'desirable' is defined by discriminating power between ObjectiveWindow
	//        and RejectWindow of execution-time-spread:
	//      - >80% for ObjectiveWindow => most desirable
	//      - <20% for ObjectiveWindow => least desirable
	//
	//     Decision-vectors which are neither most desirable nor least desirable are 'unclassified', and the set
	//       of unclassified entries is appended into vUnknownDesirableDecisionSets. The set is unioned with the unclassified
	//       set of any ancestors skipped just before the current ancestor. The skipped ancestors are those who had
	//       both most and least desirable sets as empty.
	//       
	//      
	//
	//4) Now compute progressive *intersections* of vMostDesirableDecisionSets[max-1 .. 0]
	//     into vForSet[max-1 .. 0],
	//    and correspondingly compute progressive unions of vLeastDesirableDecisionSets[max-1 .. 0]
	//    into vAgainstSet[max-1 .. 0]
	//    (max-1..0 are indices for only those ancestors that impose preferences),
	//
	//    Similarly, compute progressive unions of vUnknownDesirableDecisionSets[max-1 .. 0] into vUnclassifiedSet[max-1 .. 0].
	//
	//
	//5) Criteria to choose dec_index:
	//  - dec_index = -1 (undefined) if no ancestors imposing preference exist.
	//  - dec_index = 0, if only one consuming ancestor expresses a preference
	//  - dec_index = max-1, if vForSet[max-1] == empty (vAgainstSet[max-1] would therefore be non-empty) => early search termination
	//  - Use highest value of 'dec_index' s.t. vForSet[dec_index-1] == empty and vForSet[dec_index] is not,
	//     to identify decision-set (this satisfies the maximum number of hierarchical objectives).       => early search termination
	//  - Use dec_index = 0, if vForSet[0] != empty.
	//
	//6) Return vForSet[dec_index], vUnclassifiedSet[dec_index] & vAgainstSet[dec_index], with accumulated counts and probabilities.
	//    All are empty if dec_index == -1.
	//   Also return boolean flag indicating whether dec_index search was terminated early (thereby preventing atleast
	//      one lower ancestor from expressing its preference).

	std::vector< std::vector<int> > vMostDesirableDecisionSets;
	std::vector< std::vector<double> > vMostDesirableDecisionSetCounts;
	std::vector< std::vector<double> > vMostDesirableDecisionSetProbs;

	std::vector< std::vector<int> > vUnknownDesirableDecisionSets;
	std::vector< std::vector<double> > vUnknownDesirableDecisionSetCounts;
	std::vector< std::vector<double> > vUnknownDesirableDecisionSetProbs;

	std::vector< std::vector<int> > vLeastDesirableDecisionSets;
	std::vector< std::vector<double> > vLeastDesirableDecisionSetCounts;
	std::vector< std::vector<double> > vLeastDesirableDecisionSetProbs;

	std::vector<int> previous_UnclassifiedDecisionSet;
	std::vector<double> previous_UnclassifiedDecisionSetCount;
	std::vector<double> previous_UnclassifiedDecisionSetProb;

	for(int fi=(int)vActiveParents.size()-1; fi >= 0; fi--) {
		Frame * ancestor = vActiveParents[fi];
		if(deciding_parameter.has_consumer(ancestor) == false)
			continue;  //ancestor is not a consumer

		FrameInfo * ancestor_frame_info = FrameInfo::get_frame_info(ancestor);
		FrameDecisionModel& ancestor_dec_model = ancestor_frame_info->decision_model;

		ParameterExecSpread * spread
			= ancestor_dec_model.map_parm_to_spread[ &(deciding_parameter) ];

		std::vector<int> ancestor_MostDesirableDecisionSet;
		std::vector<double> ancestor_MostDesirableDecisionSetCount;
		std::vector<double> ancestor_MostDesirableDecisionSetProb;

		std::vector<int> ancestor_UnclassifiedDecisionSet;
		std::vector<double> ancestor_UnclassifiedDecisionSetCount;
		std::vector<double> ancestor_UnclassifiedDecisionSetProb;

		std::vector<int> ancestor_LeastDesirableDecisionSet;
		std::vector<double> ancestor_LeastDesirableDecisionSetCount;
		std::vector<double> ancestor_LeastDesirableDecisionSetProb;

		std::cout << "get_decision_sets_for_parameter: parent's vFOR_ObjectiveBinIndices = [";
		for(int i=0; i<(int)ancestor_dec_model.vFOR_ObjectiveBinIndices.size(); i++)
			std::cout << ancestor_dec_model.vFOR_ObjectiveBinIndices[i] << " ";
		std::cout << "]" << std::endl;
		std::cout << "get_decision_sets_for_parameter: parent's vAGAINST_ObjectiveBinIndices= [";
		for(int i=0; i<(int)ancestor_dec_model.vAGAINST_ObjectiveBinIndices.size(); i++)
			std::cout << ancestor_dec_model.vAGAINST_ObjectiveBinIndices[i] << " ";
		std::cout << "]" << std::endl;
		spread->get_discriminating_values(
			ancestor_dec_model.vFOR_ObjectiveBinIndices, 0.80,
				//return values:
			ancestor_MostDesirableDecisionSet,
			ancestor_MostDesirableDecisionSetCount,
			ancestor_MostDesirableDecisionSetProb
		);
		spread->get_discriminating_values(
			ancestor_dec_model.vAGAINST_ObjectiveBinIndices, 0.80,
				//return values:
			ancestor_LeastDesirableDecisionSet,
			ancestor_LeastDesirableDecisionSetCount,
			ancestor_LeastDesirableDecisionSetProb
		);

		//construct unclassified set
		std::vector<int> ancestor_AllDecisionSet;
		std::vector<double> ancestor_AllDecisionSetCount;
		std::vector<double> ancestor_AllDecisionSetProb;
		spread->get_discriminating_values(
			ancestor_dec_model.vFOR_ObjectiveBinIndices, 0.0, //basically get all values
				//return values:
			ancestor_AllDecisionSet,
			ancestor_AllDecisionSetCount,
			ancestor_AllDecisionSetProb
		);

		std::cout << " **** ancestor_AllDecisionSet = [";
		for(int x=0; x<(int)ancestor_AllDecisionSet.size(); x++)
			std::cout << ancestor_AllDecisionSet[x] << ", ";
		std::cout << "]" << std::endl;

		int j=0;
		int k=0;
		for(int i=0; i<(int)ancestor_AllDecisionSet.size(); i++) {
			while( j < (int)ancestor_MostDesirableDecisionSet.size()
					&& ancestor_MostDesirableDecisionSet[j] < ancestor_AllDecisionSet[i]
			)
			{ j++; }

			if(j < (int)ancestor_MostDesirableDecisionSet.size()
				&& ancestor_MostDesirableDecisionSet[j] == ancestor_AllDecisionSet[i]) //i'th value is already most desirable
			{ continue; } //skip i'th value

			while( k < (int)ancestor_LeastDesirableDecisionSet.size()
					&& ancestor_LeastDesirableDecisionSet[k] < ancestor_AllDecisionSet[i]
			)
			{ k++; }

			if(k < (int)ancestor_LeastDesirableDecisionSet.size()
				&& ancestor_LeastDesirableDecisionSet[k] == ancestor_AllDecisionSet[i]) //i'th value is already least desirable
			{ continue; } //skip i'th value

			ancestor_UnclassifiedDecisionSet.push_back( ancestor_AllDecisionSet.at(i) );
			ancestor_UnclassifiedDecisionSetCount.push_back( ancestor_AllDecisionSetCount.at(i) );
			ancestor_UnclassifiedDecisionSetProb.push_back( ancestor_AllDecisionSetProb.at(i) );
		}

		std::cout << " **** ancestor_UnclassifiedDecisionSet = [";
		for(int x=0; x<(int)ancestor_UnclassifiedDecisionSet.size(); x++)
			std::cout << ancestor_UnclassifiedDecisionSet[x] << ", ";
		std::cout << "]" << std::endl;

		std::vector<int> cumulative_UnclassifiedDecisionSet;
		std::vector<double> cumulative_UnclassifiedDecisionSetCount;
		std::vector<double> cumulative_UnclassifiedDecisionSetProb;

		union_sorted_bin_vectors(
			previous_UnclassifiedDecisionSet, previous_UnclassifiedDecisionSetCount, previous_UnclassifiedDecisionSetProb,
			ancestor_UnclassifiedDecisionSet, ancestor_UnclassifiedDecisionSetCount, ancestor_UnclassifiedDecisionSetProb,
			cumulative_UnclassifiedDecisionSet, cumulative_UnclassifiedDecisionSetCount, cumulative_UnclassifiedDecisionSetProb);
			
		std::cout << " **** cumulative_UnclassifiedDecisionSet = [";
		for(int x=0; x<(int)cumulative_UnclassifiedDecisionSet.size(); x++)
			std::cout << cumulative_UnclassifiedDecisionSet[x] << ", ";
		std::cout << "]" << std::endl;

		previous_UnclassifiedDecisionSet = cumulative_UnclassifiedDecisionSet;
		previous_UnclassifiedDecisionSetCount = cumulative_UnclassifiedDecisionSetCount;
		previous_UnclassifiedDecisionSetProb = cumulative_UnclassifiedDecisionSetProb;

		if(vMostDesirableDecisionSets.size() > 0) { //top level consumer frame must always be included, even if provides only unclassified values
			if(ancestor_MostDesirableDecisionSet.size() == 0 && ancestor_LeastDesirableDecisionSet.size() == 0)
				continue; //consuming ancestor does not impose preference
		}

		//ancestor does impose preference
		vMostDesirableDecisionSets.push_back(ancestor_MostDesirableDecisionSet);
		vMostDesirableDecisionSetCounts.push_back(ancestor_MostDesirableDecisionSetCount);
		vMostDesirableDecisionSetProbs.push_back(ancestor_MostDesirableDecisionSetProb);

		vUnknownDesirableDecisionSets.push_back(cumulative_UnclassifiedDecisionSet);
		vUnknownDesirableDecisionSetCounts.push_back(cumulative_UnclassifiedDecisionSetCount);
		vUnknownDesirableDecisionSetProbs.push_back(cumulative_UnclassifiedDecisionSetProb);

		vLeastDesirableDecisionSets.push_back(ancestor_LeastDesirableDecisionSet);
		vLeastDesirableDecisionSetCounts.push_back(ancestor_LeastDesirableDecisionSetCount);
		vLeastDesirableDecisionSetProbs.push_back(ancestor_LeastDesirableDecisionSetProb);

		previous_UnclassifiedDecisionSet.clear();
		previous_UnclassifiedDecisionSetCount.clear();
		previous_UnclassifiedDecisionSetProb.clear();
	}

	std::reverse(vMostDesirableDecisionSets.begin(), vMostDesirableDecisionSets.end());
	std::reverse(vMostDesirableDecisionSetCounts.begin(), vMostDesirableDecisionSetCounts.end());
	std::reverse(vMostDesirableDecisionSetProbs.begin(), vMostDesirableDecisionSetProbs.end());
	std::reverse(vUnknownDesirableDecisionSets.begin(), vUnknownDesirableDecisionSets.end());
	std::reverse(vUnknownDesirableDecisionSetCounts.begin(), vUnknownDesirableDecisionSetCounts.end());
	std::reverse(vUnknownDesirableDecisionSetProbs.begin(), vUnknownDesirableDecisionSetProbs.end());
	std::reverse(vLeastDesirableDecisionSets.begin(), vLeastDesirableDecisionSets.end());
	std::reverse(vLeastDesirableDecisionSetCounts.begin(), vLeastDesirableDecisionSetCounts.end());
	std::reverse(vLeastDesirableDecisionSetProbs.begin(), vLeastDesirableDecisionSetProbs.end());

	int num_deciding_levels = (int)vMostDesirableDecisionSets.size();

	std::cout << "get_decision_sets_for_parameter(): num_deciding_levels = " << num_deciding_levels << std::endl;
	for(int dli = num_deciding_levels-1; dli >= 0; dli--) {
		std::cout << "   vMostDesirableDecisionSets[" << dli << "] = [";
		for(int x=0; x<(int)vMostDesirableDecisionSets[dli].size(); x++)
			std::cout << vMostDesirableDecisionSets[dli][x] << " ";
		std::cout << "]\n" << std::endl;

		std::cout << "   vLeastDesirableDecisionSets[" << dli << "] = [";
		for(int x=0; x<(int)vLeastDesirableDecisionSets[dli].size(); x++)
			std::cout << vLeastDesirableDecisionSets[dli][x] << " ";
		std::cout << "]\n" << std::endl;
	}

	//Progressive intersections and unions
	std::vector< std::vector<int> > vForSet(num_deciding_levels);
	std::vector< std::vector<double> > vForSetCounts(num_deciding_levels);
	std::vector< std::vector<double> > vForSetProbs(num_deciding_levels);

	std::vector< std::vector<int> > vUnclassifiedSet(num_deciding_levels);
	std::vector< std::vector<double> > vUnclassifiedSetCounts(num_deciding_levels);
	std::vector< std::vector<double> > vUnclassifiedSetProbs(num_deciding_levels);

	std::vector< std::vector<int> > vAgainstSet(num_deciding_levels);
	std::vector< std::vector<double> > vAgainstSetCounts(num_deciding_levels);
	std::vector< std::vector<double> > vAgainstSetProbs(num_deciding_levels);

	for(int dli = num_deciding_levels-1; dli >= 0; dli--) {
		if(dli == num_deciding_levels-1) {
			vForSet[dli] = vMostDesirableDecisionSets[dli];
			vForSetCounts[dli] = vMostDesirableDecisionSetCounts[dli];
			vForSetProbs[dli] = vMostDesirableDecisionSetProbs[dli];
			vUnclassifiedSet[dli] = vUnknownDesirableDecisionSets[dli];
			vUnclassifiedSetCounts[dli] = vUnknownDesirableDecisionSetCounts[dli];
			vUnclassifiedSetProbs[dli] = vUnknownDesirableDecisionSetProbs[dli];
			vAgainstSet[dli] = vLeastDesirableDecisionSets[dli];
			vAgainstSetCounts[dli] = vLeastDesirableDecisionSetCounts[dli];
			vAgainstSetProbs[dli] = vLeastDesirableDecisionSetProbs[dli];
		}
		else {
			intersect_sorted_bin_vectors(
				vForSet[dli+1], vForSetCounts[dli+1], vForSetProbs[dli+1],
				vMostDesirableDecisionSets[dli], vMostDesirableDecisionSetCounts[dli], vMostDesirableDecisionSetProbs[dli],
				vForSet[dli], vForSetCounts[dli], vForSetProbs[dli]
			);

			union_sorted_bin_vectors(
				vUnclassifiedSet[dli+1], vUnclassifiedSetCounts[dli+1], vUnclassifiedSetProbs[dli+1],
				vUnknownDesirableDecisionSets[dli], vUnknownDesirableDecisionSetCounts[dli], vUnknownDesirableDecisionSetProbs[dli],
				vUnclassifiedSet[dli], vUnclassifiedSetCounts[dli], vUnclassifiedSetProbs[dli]
			);

			union_sorted_bin_vectors(
				vAgainstSet[dli+1], vAgainstSetCounts[dli+1], vAgainstSetProbs[dli+1],
				vLeastDesirableDecisionSets[dli], vLeastDesirableDecisionSetCounts[dli], vLeastDesirableDecisionSetProbs[dli],
				vAgainstSet[dli], vAgainstSetCounts[dli], vAgainstSetProbs[dli]
			);
		}
	}

	//Setup return values
	if(num_deciding_levels == 0) {
		vForDecisionSet.clear();
		vForDecisionSet_Counts.clear();
		vForDecisionSet_Probs.clear();
		vUnclassifiedDecisionSet.clear();
		vUnclassifiedDecisionSet_Counts.clear();
		vUnclassifiedDecisionSet_Probs.clear();
		vAgainstDecisionSet.clear();
		vAgainstDecisionSet_Counts.clear();
		vAgainstDecisionSet_Probs.clear();

		return false; //no consuming parents with preferences exist, so no parents were prevented from expressing their preference
	}
	else if(num_deciding_levels == 1) {
		vForDecisionSet = vForSet[0];
		vForDecisionSet_Counts = vForSetCounts[0];
		vForDecisionSet_Probs = vForSetProbs[0];
		vUnclassifiedDecisionSet = vUnclassifiedSet[0];
		vUnclassifiedDecisionSet_Counts = vUnclassifiedSetCounts[0];
		vUnclassifiedDecisionSet_Probs = vUnclassifiedSetProbs[0];
		vAgainstDecisionSet = vAgainstSet[0];
		vAgainstDecisionSet_Counts = vAgainstSetCounts[0];
		vAgainstDecisionSet_Probs = vAgainstSetProbs[0];

		return false; //no consuming ancestor at a lower-level was prevented from expressing preference
	}
	else { //num_deciding_levels >= 2
		int dec_index = num_deciding_levels-1;
		while(dec_index > 0) {
			if(vForSet[dec_index].size() > 0 && vForSet[dec_index-1].size() == 0)
				break; //found a dec_index > 0
			dec_index--;
		}
		vForDecisionSet = vForSet.at(dec_index);
		vForDecisionSet_Counts = vForSetCounts.at(dec_index);
		vForDecisionSet_Probs = vForSetProbs.at(dec_index);
		vAgainstDecisionSet = vAgainstSet.at(dec_index);
		vAgainstDecisionSet_Counts = vAgainstSetCounts.at(dec_index);
		vAgainstDecisionSet_Probs = vAgainstSetProbs.at(dec_index);

		//Clean out any elements present in vAgainstDecisionSet from vUnclassifiedDecisionSet
		int j=0;
		for(int i=0; i<(int)vUnclassifiedSet.at(dec_index).size(); i++) {
			while(j < (int)vAgainstDecisionSet.size() && vAgainstDecisionSet[j] < vUnclassifiedSet[dec_index][i])
			{ j++; }

			if(j < (int)vAgainstDecisionSet.size() && vAgainstDecisionSet[j] == vUnclassifiedSet[dec_index][i])
				continue; //skip common elements
					
			vUnclassifiedDecisionSet.push_back( vUnclassifiedSet.at(dec_index).at(i) );
			vUnclassifiedDecisionSet_Counts.push_back( vUnclassifiedSetCounts.at(dec_index).at(i) ) ;
			vUnclassifiedDecisionSet_Probs.push_back( vUnclassifiedSetProbs.at(dec_index).at(i) );
		}

		return (dec_index != 0); //all ancestors lower than dec_index were prevented from expressing their preference
	}
}





void activate_decision_model_and_decide_setting(Frame * frame) {
	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);
	FrameDecisionModel& frame_dec = frame_info->decision_model;

	//Make sure parent tracks current frame's execution-time-parameter
	if(frame_info->curr_parent_frame != 0) {
		if(frame_dec.exec_time_parameter.has_consumer(frame_info->curr_parent_frame) == false)
			frame_dec.exec_time_parameter.add_consumer(frame_info->curr_parent_frame);
	}

	//Steps
	//1. Get local-setting:
	//    local_vFOR_ObjectiveWindowBinIndices = [min, max] based only on frame's specified mean-objective
	//      (is [] if no objective defined).
	//    local_vAGAINST_ObjectiveBinIndices = complement of [min, max] if mean-objective specified
	//      (is [] if no objective defined).
	//       Optimization: Can be made a little less stringent that complement of [min, max] based on
	//          specified probability.
	//
	//2. Identify dynamic parent-frames top-down:
	//  a) Relevant parent-frames:
	//    - Examine those parent frames that track current frame's execution-time-parameter,
	//       and who have derived objectives for enforcements
	//      (i.e., atleast one of vFOR_ObjectiveBinIndices and vAGAINST_ObjectiveBinIndices is != [] for that parent)
	//
	//  b) Extract exec-time-parameter-bin-indices for the current-frame in the relevant parent-frames that
	//       with high-probability (say, >= 80%) discriminate FOR and/or AGAINST satisfying the
	//        parents' objectives.
	//          => parent_FOR_set & parent_AGAINST_set
	//
	//        Skip parent frame if both parent_FOR_set & parent_AGAINST_set are []. This is because this parent
	//          is imposing no preference on how the current-frame derives its objectives.
	//
	//  c) Perform:
	//       - Progressive intersections of exec-time-index-values of current-frame 
	//           that discriminate FOR meeting corresponding parent's objectives
	//       - Progressive unions of exec-time-index-values of current-frame 
	//           that discriminate AGAINST meeting corresponding parent's objectives
	//
	//  d) Treat current-frame's local-settings as the last frame in the progressive FOR and AGAINST sets
	//
	//  e) Stop at a level just before progressive FOR set intersection becomes NULL, or until all levels exhausted.
	//     The current progressive FOR set becomes the current-frames vFOR_ObjectiveBinIndices and the corresponding
	//        progressive AGAINST set (at the same level) becomes the current-frame's vAGAINST_ObjectiveBinIndices.
	
	if(frame_dec.isObjectiveInitialized() == false) {
		if(frame_info->objective.isDefined) {
			ExecTime_t mean = 0.0;
			if(frame_info->objective.type == Objective::ObjABSOLUTE)
			{ mean = frame_info->objective.mean; }
			else { //Objective::ObjRELATIVE
				Frame * ref_frame = get_frame_from_frame_id(frame_info->objective.reference_frame_id);
				FrameInfo * ref_frame_info = FrameInfo::get_frame_info(ref_frame);

				assert(ref_frame_info->decision_model.isObjectiveInitialized() == true);
				assert(ref_frame_info->decision_model.bHasMeanObjectiveDefined == true);

				mean = ref_frame_info->decision_model.mean_objective * frame_info->objective.relative_mean_frac;
			}

			frame_dec.initialize_objective(true,
				mean, frame_info->objective.window_frac_lower, frame_info->objective.window_frac_upper, frame_info->objective.prob, frame_info->objective.sliding_window_size, frame_info->objective.impact_rescaler);
		}
		else //no mean-objective specified
		{ frame_dec.initialize_objective(false); }
	}

	//Local setting for objective-bins
	std::vector<int> local_vFOR_ObjectiveWindowBinIndices;
	std::vector<int> local_vAGAINST_ObjectiveBinIndices;
	frame_dec.get_ObjectiveWindowBinIndices_for_local_objective(
		local_vFOR_ObjectiveWindowBinIndices, local_vAGAINST_ObjectiveBinIndices);

	//Preference of dynamic parents
	bool bUpperParentBlocksLowerParentsPreferences = false;
	std::vector<int> vForDecisionSet;
	std::vector<double> vForDecisionSet_Counts;
	std::vector<double> vForDecisionSet_Probs;
	std::vector<int> vUnclassifiedDecisionSet;
	std::vector<double> vUnclassifiedDecisionSet_Counts;
	std::vector<double> vUnclassifiedDecisionSet_Probs;
	std::vector<int> vAgainstDecisionSet;
	std::vector<double> vAgainstDecisionSet_Counts;
	std::vector<double> vAgainstDecisionSet_Probs;

	if(frame_info->curr_parent_frame != 0) {
		bUpperParentBlocksLowerParentsPreferences = get_decision_sets_for_parameter(
				frame_dec.exec_time_parameter, frame_info->curr_parent_frame,
					//return values:
				vForDecisionSet, vForDecisionSet_Counts, vForDecisionSet_Probs,
				vUnclassifiedDecisionSet, vUnclassifiedDecisionSet_Counts, vUnclassifiedDecisionSet_Probs,
				vAgainstDecisionSet, vAgainstDecisionSet_Counts, vAgainstDecisionSet_Probs
			);
	}

	if(bUpperParentBlocksLowerParentsPreferences == true) { //since a upper-level consumer blocks, local-settings will also be blocked
		frame_dec.vFOR_ObjectiveBinIndices = vForDecisionSet;
		frame_dec.vAGAINST_ObjectiveBinIndices = vAgainstDecisionSet;
	}
	else { //upper-level does not block, attempt to incorporate local-settings as well
		if(vForDecisionSet.size() == 0 && vAgainstDecisionSet.size() == 0) { //no preferences from parents
			frame_dec.vFOR_ObjectiveBinIndices = local_vFOR_ObjectiveWindowBinIndices;
			frame_dec.vAGAINST_ObjectiveBinIndices = local_vAGAINST_ObjectiveBinIndices;
		}
		else { //some preferences from parents
			std::vector<int> vFOR_ReturnSet;
			std::vector<double> vFOR_ReturnSet_Counts;
			std::vector<double> vFOR_ReturnSet_Probs;

			//assumption: vForDecisionSet, local_vFOR_ObjectiveWindowBinIndices are sorted in ascending order
			intersect_sorted_bin_vectors(
				vForDecisionSet, vForDecisionSet_Counts, vForDecisionSet_Probs,
				local_vFOR_ObjectiveWindowBinIndices, std::vector<double>(local_vFOR_ObjectiveWindowBinIndices.size(), 0.0),
					std::vector<double>(local_vFOR_ObjectiveWindowBinIndices.size(), 0.0),
				vFOR_ReturnSet, vFOR_ReturnSet_Counts, vFOR_ReturnSet_Probs
			);

			std::vector<int> vAGAINST_ReturnSet;
			std::vector<double> vAGAINST_ReturnSet_Counts;
			std::vector<double> vAGAINST_ReturnSet_Probs;

			//assumption: vAgainstDecisionSet, local_vAGAINST_ObjectiveBinIndices are sorted in ascending order
			union_sorted_bin_vectors(
				vAgainstDecisionSet, vAgainstDecisionSet_Counts, vAgainstDecisionSet_Probs,
				local_vAGAINST_ObjectiveBinIndices, std::vector<double>(local_vAGAINST_ObjectiveBinIndices.size(), 0.0),
					std::vector<double>(local_vAGAINST_ObjectiveBinIndices.size(), 0.0),
				vAGAINST_ReturnSet, vAGAINST_ReturnSet_Counts, vAGAINST_ReturnSet_Probs
			);

			if(vFOR_ReturnSet.size() == 0) { //upper-levels block local-settings
				frame_dec.vFOR_ObjectiveBinIndices = vForDecisionSet;
				frame_dec.vAGAINST_ObjectiveBinIndices = vAgainstDecisionSet;
			}
			else { //all levels including local-settings can express preference
				frame_dec.vFOR_ObjectiveBinIndices = vFOR_ReturnSet;
				frame_dec.vAGAINST_ObjectiveBinIndices = vAGAINST_ReturnSet;
			}
		}
	}
}


//debug control
bool bMagnifyCount_by_SuccessFailure_DeviationDegree = true;

void feature_control_MagnifyCount_by_SuccessFailure_DeviationDegree(bool new_setting) {
	bMagnifyCount_by_SuccessFailure_DeviationDegree = new_setting;
	std::cout << "SRT Feature Control: bMagnifyCount_by_SuccessFailure_DeviationDegree = " << bMagnifyCount_by_SuccessFailure_DeviationDegree
		<< std::endl;
}

bool feature_query_MagnifyCount_by_SuccessFailure_DeviationDegree() {
	return bMagnifyCount_by_SuccessFailure_DeviationDegree;
}



//debug control

bool bDeemphasizeHistoryWithAlphaRate = true;

double deemphasize_history_rate_alpha = 0.99;
	//The sample-counts in a Frame's history are scaled down by this ratio after
	//  each completion of a frame execution. This de-emphasizes history, causing it
	//  to be slowly forgotten unless re-inforced with new experiences.


void feature_control_DeemphasizeHistoryWithAlphaRate(bool enable, double alpha_rate) {
	bDeemphasizeHistoryWithAlphaRate = enable;
	deemphasize_history_rate_alpha = alpha_rate;
	std::cout << "SRT Feature Control: bDeemphasizeHistoryWithAlphaRate = " << bDeemphasizeHistoryWithAlphaRate << " deemphasize_history_rate_alpha = " << deemphasize_history_rate_alpha
		<< std::endl;
}

std::pair<bool, double> feature_query_DeemphasizeHistoryWithAlphaRate() {
	return std::make_pair(bDeemphasizeHistoryWithAlphaRate, deemphasize_history_rate_alpha);
}


//debug control

bool bForgetHistoryBelowBetaThreshold = true;

double forget_history_threshold_ratio_beta = 0.001;
	//Individual cache-entries in bins across each Parameter-Exec-Spread are deleted
	//  if their counts are below a 'forget_history_threshold_ratio_beta' multiple of
	//  the Parameter-Exec-Spread's total-sample-count.

void feature_control_ForgetHistoryBelowBetaThreshold(bool enable, double beta_ratio) {
	bForgetHistoryBelowBetaThreshold = enable;
	forget_history_threshold_ratio_beta = beta_ratio;
	std::cout << "SRT Feature Control: bForgetHistoryBelowBetaThreshold = " << bForgetHistoryBelowBetaThreshold << " forget_history_threshold_ratio_beta = " << forget_history_threshold_ratio_beta
		<< std::endl;
}

std::pair<bool, double> feature_query_ForgetHistoryBelowBetaThreshold() {
	return std::make_pair(bForgetHistoryBelowBetaThreshold, forget_history_threshold_ratio_beta);
}


void update_decision_model_on_completion(Frame * frame) {
	//1. Read frame_info->current_invocation_exec_time, update statistics
	//    - frame_dec.exec_time_record
	//    - frame_dec.exec_time_parameter
	//
	//2. Update map_parm_to_spread using *normalized* map_parm_to_curr_record, and clear map_parm_to_curr_record
	//
	//3. Optimizations: propagate tracking of parameters to parent frames, if some parameters are
	//     not being meaningfully tracked by current frame (FIXME: implement)
	//
	//   => Occasionally re-compute 'non-discriminating decision vectors' for frame
	//       - frequency of re-computation inversely proportional to frame's invocation count
	//
	//
	//4. FIXME: what else??

	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);
	FrameDecisionModel& frame_dec = frame_info->decision_model;

	frame_dec.exec_time_sliding_window.push(frame_info->current_invocation_exec_time);
	std::cout << "  exec_time_sliding_window = " << frame_dec.exec_time_sliding_window.print_string() << std::endl;

	ExecTime_t rescaled_current_invocation_exec_time
		= frame_dec.impact_rescaler( frame_dec.exec_time_sliding_window.get_average() );

	
	if(frame_dec.bHasMeanObjectiveDefined) {
		bool bUnbinnedObjectiveSuccess = ( rescaled_current_invocation_exec_time >= frame_dec.mean_objective * (1.0 - frame_dec.window_frac_lower)
			&& rescaled_current_invocation_exec_time <= frame_dec.mean_objective * (1.0 + frame_dec.window_frac_upper) );

		frame_dec.total_invoke_count++;
		frame_dec.unbinned_satisfaction_ratio = ( frame_dec.unbinned_satisfaction_ratio * (frame_dec.total_invoke_count - 1)
							+ (bUnbinnedObjectiveSuccess ? 1 : 0) ) / frame_dec.total_invoke_count;
		frame_dec.unbinned_mean = ( frame_dec.unbinned_mean * (frame_dec.total_invoke_count - 1)
							+ rescaled_current_invocation_exec_time ) / frame_dec.total_invoke_count;
		frame_dec.unbinned_sq_mean = ( frame_dec.unbinned_sq_mean * (frame_dec.total_invoke_count - 1)
							+ rescaled_current_invocation_exec_time * rescaled_current_invocation_exec_time ) / frame_dec.total_invoke_count;
		frame_dec.unbinned_variance = frame_dec.unbinned_sq_mean - frame_dec.unbinned_mean * frame_dec.unbinned_mean;

		frame_dec.unbinned_variance_from_mean_objective = frame_dec.unbinned_sq_mean - frame_dec.mean_objective * frame_dec.mean_objective;
	}

	int current_exec_time_as_bin_index
		= frame_dec.convert_exec_time_to_int_bin(rescaled_current_invocation_exec_time);

	frame_dec.exec_time_parameter.inform_enclosing_active_consumers_of_sample_measurement(
			get_dynamically_enclosing_frames(frame), current_exec_time_as_bin_index);

	double unbinned_std = sqrt(frame_dec.unbinned_variance);
	double unbinned_std_from_mean_objective = sqrt(frame_dec.unbinned_variance_from_mean_objective);
	std::cout << "Frame #" << frame->id << " exec-time occurred: current_exec_time_as_bin_index = "
		<< current_exec_time_as_bin_index
		<< " on rescaled_current_invocation_exec_time = " << rescaled_current_invocation_exec_time
		<< " unbinned_satisfaction_ratio = " << frame_dec.unbinned_satisfaction_ratio << " total_invoke_count = " << frame_dec.total_invoke_count
		<< " unbinned_mean = " << frame_dec.unbinned_mean << " unbinned_variance = " << frame_dec.unbinned_variance << " unbinned_std = " << unbinned_std
		<< " unbinned_variance_from_mean_objective = " << frame_dec.unbinned_variance_from_mean_objective << " unbinned_std_from_mean_objective = " << unbinned_std_from_mean_objective
		<< std::endl;
	frame_dec.exec_time_record.note_sample(current_exec_time_as_bin_index);

	bool bActiveObjectiveSuccess = ( std::find(
				frame_dec.vFOR_ObjectiveBinIndices.begin(), frame_dec.vFOR_ObjectiveBinIndices.end(),
				current_exec_time_as_bin_index) != frame_dec.vFOR_ObjectiveBinIndices.end() );

	bool bActiveObjectiveFailure = ( std::find(
				frame_dec.vAGAINST_ObjectiveBinIndices.begin(), frame_dec.vAGAINST_ObjectiveBinIndices.end(),
				current_exec_time_as_bin_index) != frame_dec.vAGAINST_ObjectiveBinIndices.end() );

	assert(not bActiveObjectiveSuccess or not bActiveObjectiveFailure);
		//vFOR_ObjectiveBinIndices and vAGAINST_ObjectiveBinIndices must be mutually exclusive, though not necessarily
		// exhaustive i.e. complementary

	double magnified_count = 1.0;
	if(bActiveObjectiveSuccess) {
		assert(frame_dec.vFOR_ObjectiveBinIndices.size() > 0);
			//assuming vFOR_ObjectiveBinIndices is a contiguous region of bin-indices
		double lower_boundary = frame_dec.convert_int_bin_to_lower_boundary_of_exec_time(frame_dec.vFOR_ObjectiveBinIndices[0]);
		double upper_boundary = frame_dec.convert_int_bin_to_upper_boundary_of_exec_time(frame_dec.vFOR_ObjectiveBinIndices[frame_dec.vFOR_ObjectiveBinIndices.size() - 1]);
		double range = upper_boundary - lower_boundary; // strictly > 0.0

		double mean = frame_dec.mean_objective;
		double deviation = fabs(rescaled_current_invocation_exec_time - mean) / range; // 0 <= deviation < 1.0
		assert(0 <= deviation && deviation < 1.0);

		magnified_count = 1.5 / (1.0 + 2.0 * deviation); // (reward for hitting bullseys) 1.5 >= magnified_count > 0.50 (penalize somewhat for missing mean)

		if(not bMagnifyCount_by_SuccessFailure_DeviationDegree)
			magnified_count = 1.0;

		std::cout << "Frame #" << frame->id << " Objective SUCCESS: magnified_count = " << magnified_count << " on deviation = " << deviation;
		std::cout << " for current_exec_time_as_bin_index = " << current_exec_time_as_bin_index << std::endl;
	}
	
	else if(bActiveObjectiveFailure) {
		if(frame_dec.vFOR_ObjectiveBinIndices.size() > 0) {
				//assuming vFOR_ObjectiveBinIndices is a contiguous region of bin-indices
			double lower_boundary = frame_dec.convert_int_bin_to_lower_boundary_of_exec_time(frame_dec.vFOR_ObjectiveBinIndices[0]);
			double upper_boundary = frame_dec.convert_int_bin_to_upper_boundary_of_exec_time(frame_dec.vFOR_ObjectiveBinIndices[frame_dec.vFOR_ObjectiveBinIndices.size() - 1]);
			double range = upper_boundary - lower_boundary; // strictly > 0.0

			double deviation_with_lower = fabs(rescaled_current_invocation_exec_time - lower_boundary) / range;
			double deviation_with_upper = fabs(rescaled_current_invocation_exec_time - upper_boundary) / range;
				//0 < deviation_with_* < infinity

			double deviation = (deviation_with_lower < deviation_with_upper ? deviation_with_lower : deviation_with_upper);

			if(deviation < 0.2)
				magnified_count = 1.0;
			else if(deviation < 0.4)
				magnified_count = 2.0; //penalize
			else {//deviation >= 0.4
				double history_factor = 1.0 / (2.0 + 1.0/deviation);
					//max = 0.5 at deviation = infinity, min = 0.22 at deviation = 0.4
				magnified_count = -history_factor; //negative value serves as flag
			}

			if(not bMagnifyCount_by_SuccessFailure_DeviationDegree)
				magnified_count = 1.0;

			std::cout << "Frame #" << frame->id << " Objective FAILURE: magnified_count = " << magnified_count << " on deviation = " << deviation;
		}
		else { // vFOR_ObjectiveBinIndices empty, so no way to judge severity of deviation
			magnified_count = 1.0;

			std::cout << "Frame #" << frame->id << " Objective FAILURE: magnified_count = " << magnified_count << " on deviation = N/A";
		}
		std::cout << " for current_exec_time_as_bin_index = " << current_exec_time_as_bin_index << std::endl;
	}

	//Update failure run-length statistics
	if(bActiveObjectiveFailure)
	{ frame_dec.active_objective_failure_run_length++; }
	else { //not a failure
		if(frame_dec.active_objective_failure_run_length > 0) { //note run-length, and reset
			int log2_val = -1;
			while(frame_dec.active_objective_failure_run_length > 0) {
				log2_val++;
				frame_dec.active_objective_failure_run_length /= 2;
			}
			if(log2_val >= (int)frame_dec.vFailure_Runlengths_wrt_active_objective.size())
				frame_dec.vFailure_Runlengths_wrt_active_objective.resize(log2_val+1, 0);
			frame_dec.vFailure_Runlengths_wrt_active_objective.at(log2_val)++;
		}
	}

	std::vector<int> local_vFOR_ObjectiveWindowBinIndices;
	std::vector<int> local_vAGAINST_ObjectiveBinIndices;
	frame_dec.get_ObjectiveWindowBinIndices_for_local_objective(
		local_vFOR_ObjectiveWindowBinIndices, local_vAGAINST_ObjectiveBinIndices);

	bool bSpecifiedObjectiveFailure = ( std::find(
			local_vAGAINST_ObjectiveBinIndices.begin(), local_vAGAINST_ObjectiveBinIndices.end(),
			current_exec_time_as_bin_index) != local_vAGAINST_ObjectiveBinIndices.end() );
	if(bSpecifiedObjectiveFailure)
	{ frame_dec.specified_objective_failure_run_length++; }
	else {
		if(frame_dec.specified_objective_failure_run_length > 0) { //note run-length, and reset
			int log2_val = -1;
			while(frame_dec.specified_objective_failure_run_length > 0) {
				log2_val++;
				frame_dec.specified_objective_failure_run_length /= 2;
			}
			if(log2_val >= (int)frame_dec.vFailure_Runlengths_wrt_specified_objective.size())
				frame_dec.vFailure_Runlengths_wrt_specified_objective.resize(log2_val+1, 0);
			frame_dec.vFailure_Runlengths_wrt_specified_objective.at(log2_val)++;
		}
	}

	//Update spread
	for(std::map<Parameter *, IntValueCache *>::iterator mit = frame_dec.map_parm_to_curr_record.begin();
			mit != frame_dec.map_parm_to_curr_record.end();
			mit++
	) {
		Parameter * parm = mit->first;
		IntValueCache * ivc = mit->second;

		assert(frame_dec.map_parm_to_spread.count(parm) > 0);
		ParameterExecSpread * corresponding_spread = frame_dec.map_parm_to_spread[parm];

		double count_update;
		if(magnified_count >= 0.0)
			count_update = magnified_count;
		else { //negative => magnified_count represents history-fraction
			count_update = (-magnified_count) * corresponding_spread->current_sample_count();
			if(count_update == 0.0) //when current_sample_count() == 0.0
				count_update = 1.0;
		}

		ivc->normalize_wrt_new_sample_count(count_update);
		for(int i=0; i<(int)ivc->vCacheEntries.size(); i++) {
			IntCacheEntry& ce = ivc->vCacheEntries[i];
			if(ce.valid) {
				corresponding_spread->note_spread_bin_occurence(current_exec_time_as_bin_index, ce.tag, ce.count);
			}
		}

		ivc->clear();
	}

	if(bDeemphasizeHistoryWithAlphaRate) {
		//Uniformly de-emphasize history
		for(std::map<Parameter *, ParameterExecSpread *>::iterator mit
					= frame_dec.map_parm_to_spread.begin();
				mit != frame_dec.map_parm_to_spread.end();
				mit++
		) {
			ParameterExecSpread& parm_exec_spread = *(mit->second);
			for(int i=0; i<(int)parm_exec_spread.vExecSpreadBins.size(); i++) {
				IntValueCache& ivc = parm_exec_spread.vExecSpreadBins[i];
				ivc.normalize_wrt_new_sample_count( ivc.get_sample_count() * deemphasize_history_rate_alpha );
			}
		}
	}

	if(bForgetHistoryBelowBetaThreshold) {
		//Drop cache tags with sufficiently low count
		for(std::map<Parameter *, ParameterExecSpread *>::iterator mit
					= frame_dec.map_parm_to_spread.begin();
				mit != frame_dec.map_parm_to_spread.end();
				mit++
		) {
			ParameterExecSpread& parm_exec_spread = *(mit->second);
			double total_count_parm_exec_spread = parm_exec_spread.current_sample_count();
			double min_threshold = total_count_parm_exec_spread * forget_history_threshold_ratio_beta;

			for(int i=0; i<(int)parm_exec_spread.vExecSpreadBins.size(); i++) {
				IntValueCache& ivc = parm_exec_spread.vExecSpreadBins[i];
				for(int j=0; j<(int)ivc.vCacheEntries.size(); j++) {
					IntCacheEntry& cache_entry = ivc.vCacheEntries[j];
					if(cache_entry.valid && cache_entry.count < min_threshold)
						ivc.delete_cache_entry(j);
				}
			}
		}
	}
	
	frame_dec.previous_invocation_exec_time = rescaled_current_invocation_exec_time;
}


ExecTime_t IDENTITY_impact_rescaler(ExecTime_t measured_execution_time_in_seconds)
{ return measured_execution_time_in_seconds; }


} //namespace Opp
