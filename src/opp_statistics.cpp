// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <sstream>

#include "opp.h"
#include "opp_frame.h"
#include "opp_frame_info.h"
#include "opp_execframe.h"

#include "opp_utilities.h"

namespace Opp {

//////////////////////////
// class FrameStatistics
//////////////////////////

FrameStatistics& FrameStatistics::refresh() {

	vExecTime_bin_centers.clear();
	vExecTime_bin_frequencies.clear();
	vSpecified_Objective_bin_indices.clear();
	vActive_Objective_bin_indices.clear();
	satisfaction_ratio_wrt_specified_objective = 0.0;
	vFailure_Runlengths_wrt_specified_objective.clear();
	satisfaction_ratio_wrt_active_objective = 0.0;
	vFailure_Runlengths_wrt_active_objective.clear();

	Frame * frame = get_frame_from_frame_id(frame_id);
	if(frame == 0) //frame not yet defined, or has been destroyed
		return *this;

	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);
	FrameDecisionModel& frame_dec = frame_info->decision_model;

	std::vector<int> local_vFOR_ObjectiveWindowBinIndices;
	std::vector<int> local_vAGAINST_ObjectiveBinIndices;
	frame_dec.get_ObjectiveWindowBinIndices_for_local_objective(
		local_vFOR_ObjectiveWindowBinIndices, local_vAGAINST_ObjectiveBinIndices);

	//WARNING: frame_dec.exec_time_parameter_num_spread_bins == -1 before frame initializes objective
	for(int i=0; i<frame_dec.exec_time_parameter_num_spread_bins; i++)
		vExecTime_bin_centers.push_back( frame_dec.convert_int_bin_to_exec_time(i) );

	vExecTime_bin_frequencies.resize(vExecTime_bin_centers.size(), 0.0);
	for(int ic=0; ic<(int)frame_dec.exec_time_record.vCacheEntries.size(); ic++) {
		if(frame_dec.exec_time_record.vCacheEntries[ic].valid)
			vExecTime_bin_frequencies.at( frame_dec.exec_time_record.vCacheEntries[ic].tag ) = frame_dec.exec_time_record.vCacheEntries[ic].count;
	}

	vSpecified_Objective_bin_indices = local_vFOR_ObjectiveWindowBinIndices;

	if(frame_info->bIsActive)
		vActive_Objective_bin_indices = frame_dec.vFOR_ObjectiveBinIndices;

	double total_count = 0.0;
	for(int i=0; i<(int)vExecTime_bin_frequencies.size(); i++)
		total_count += vExecTime_bin_frequencies[i];
			
	for(int i=0; i<(int)vSpecified_Objective_bin_indices.size(); i++) {
		int FOR_bin_index = vSpecified_Objective_bin_indices[i];
		satisfaction_ratio_wrt_specified_objective += vExecTime_bin_frequencies.at(FOR_bin_index);
	}
	if(total_count > 0.0)
		satisfaction_ratio_wrt_specified_objective /= total_count;

	vFailure_Runlengths_wrt_specified_objective = frame_dec.vFailure_Runlengths_wrt_specified_objective;

	for(int i=0; i<(int)vActive_Objective_bin_indices.size(); i++) {
		int FOR_bin_index = vActive_Objective_bin_indices[i];
		satisfaction_ratio_wrt_active_objective += vExecTime_bin_frequencies.at(FOR_bin_index);
	}
	if(total_count > 0.0)
		satisfaction_ratio_wrt_active_objective /= total_count;
	
	vFailure_Runlengths_wrt_active_objective = frame_dec.vFailure_Runlengths_wrt_active_objective;

	return *this;
}

std::string FrameStatistics::print_string() const {
	std::ostringstream oss;

	oss << "$$ Frame #" << frame_id << " : Statistics" << std::endl;
	oss << "$$   vExecTime_bin_centers     = " << vExecTime_bin_centers << std::endl;
	oss << "$$   vExecTime_bin_frequencies = " << vExecTime_bin_frequencies << std::endl;
	oss << "$$   vSpecified_Objective_bin_indices = " << vSpecified_Objective_bin_indices << std::endl;
	oss << "$$   satisfaction_ratio_wrt_specified_objective = " << satisfaction_ratio_wrt_specified_objective << std::endl;
	oss << "$$   vFailure_Runlengths_wrt_specified_objective = " << vFailure_Runlengths_wrt_specified_objective << std::endl;
	oss << "$$   vActive_Objective_bin_indices    = " << vActive_Objective_bin_indices << std::endl;
	oss << "$$   satisfaction_ratio_wrt_active_objective    = " << satisfaction_ratio_wrt_active_objective << std::endl;
	oss << "$$   vFailure_Runlengths_wrt_active_objective    = " << vFailure_Runlengths_wrt_active_objective << std::endl;

	return oss.str();
}




//////////////////////////
// class ExecFrameStatistics
//////////////////////////

void create_ExecTime_vs_ModelDecision_Distribution(
	const ParameterExecSpread * parm_exec_spread,
	ExecTime_vs_ModelDecision_Distribution& etmd_distr //return value: updates vModelChoices and vvCounts fields
) {
	etmd_distr.vModelChoices.clear();
	etmd_distr.vvCounts.clear();

	std::map<int, int> map_decision_value_to_index_in_vModelChoices;
	for(int i=0; i<(int)parm_exec_spread->vExecSpreadBins.size(); i++) {
		for(int j=0; j<(int)parm_exec_spread->vExecSpreadBins[i].vCacheEntries.size(); j++) {
			const IntCacheEntry& ice = parm_exec_spread->vExecSpreadBins[i].vCacheEntries[j];
			if(ice.valid == false)
				continue;
			int decision_value = ice.tag;
			if(map_decision_value_to_index_in_vModelChoices.count(decision_value) == 0) {
				map_decision_value_to_index_in_vModelChoices[decision_value] = (int)etmd_distr.vModelChoices.size();
				etmd_distr.vModelChoices.push_back(decision_value);
			}
		}
	}
	std::sort(etmd_distr.vModelChoices.begin(), etmd_distr.vModelChoices.end());
	map_decision_value_to_index_in_vModelChoices.clear();
	for(int k=0; k<(int)etmd_distr.vModelChoices.size(); k++)
		map_decision_value_to_index_in_vModelChoices[ etmd_distr.vModelChoices[k] ] = k;

	std::vector<double> vCountColumn(etmd_distr.vModelChoices.size(), 0.0);
	etmd_distr.vvCounts.resize( parm_exec_spread->vExecSpreadBins.size(), vCountColumn );

	for(int i=0; i<(int)parm_exec_spread->vExecSpreadBins.size(); i++) {
		for(int j=0; j<(int)parm_exec_spread->vExecSpreadBins[i].vCacheEntries.size(); j++) {
			const IntCacheEntry& ice = parm_exec_spread->vExecSpreadBins[i].vCacheEntries[j];
			if(ice.valid == false)
				continue;
			int decision_value = ice.tag;
			double count = ice.count;

			int column_index = map_decision_value_to_index_in_vModelChoices[decision_value];
			etmd_distr.vvCounts.at(i).at(column_index) = count;
		}
	}
}

ExecFrameStatistics& ExecFrameStatistics::refresh()
{
	vTracking_FrameIDs.clear();
	map_tracking_frame_to_exectime_distribution.clear();

	ExecFrame * execframe = get_execframe_from_execframe_id(execframe_id);
	if(execframe == 0) //execframe not yet defined, or has been destroyed
		return *this;

	ExecFrameInfo * execframe_info = ExecFrameInfo::get_execframe_info(execframe);
	ExecFrameDecisionModel& execframe_dec = execframe_info->decision_model;

	for(std::map<Frame *, IntValueCache *>::iterator mit = execframe_dec.decision_vector_parameter.map_consumer_caches.begin();
		mit != execframe_dec.decision_vector_parameter.map_consumer_caches.end();
		mit++
	) {
		Frame * tracking_frame = mit->first;
		vTracking_FrameIDs.push_back(tracking_frame->id);
	}

	std::sort(vTracking_FrameIDs.begin(), vTracking_FrameIDs.end());

	for(int i=0; i<(int)vTracking_FrameIDs.size(); i++) {
		Frame * tracking_frame = get_frame_from_frame_id(vTracking_FrameIDs[i]);
		FrameInfo * tracking_frame_info = FrameInfo::get_frame_info(tracking_frame);
		FrameDecisionModel& tracking_frame_dec = tracking_frame_info->decision_model;

		ParameterExecSpread * tracking_parm_exec_spread = tracking_frame_dec.map_parm_to_spread[ &(execframe_dec.decision_vector_parameter) ];

		map_tracking_frame_to_exectime_distribution[tracking_frame->id] = ExecTime_vs_ModelDecision_Distribution();
		ExecTime_vs_ModelDecision_Distribution& etmd_distr = map_tracking_frame_to_exectime_distribution[tracking_frame->id];
		create_ExecTime_vs_ModelDecision_Distribution( tracking_parm_exec_spread, etmd_distr );

		for(int i=0; i<tracking_frame_dec.exec_time_parameter_num_spread_bins; i++)
			etmd_distr.vExecTime_bin_centers.push_back( tracking_frame_dec.convert_int_bin_to_exec_time(i) );
	}

	return *this;
}


std::string print_ExecTime_vs_ModelDecision_Distribution(const ExecTime_vs_ModelDecision_Distribution& etmd_distr) {
	std::ostringstream bos;

	bos << "$$   || vModelChoices" << std::endl;
	bos << "$$   \\/   vExecTime_bin_centers ==> ";
	for(int i=0; i<(int)etmd_distr.vExecTime_bin_centers.size(); i++)
		bos << i << "(" << etmd_distr.vExecTime_bin_centers[i] << ")  ";
	bos << std::endl;

	for(int j=0; j<(int)etmd_distr.vModelChoices.size(); j++) {
		bos << "$$           " << etmd_distr.vModelChoices[j] << ":                    ";
		for(int i=0; i<(int)etmd_distr.vvCounts.size(); i++)
			bos << etmd_distr.vvCounts[i].at(j) << "     ";
		bos << std::endl;
	}

	return bos.str();
}

std::string ExecFrameStatistics::print_string() {
	std::ostringstream oss;

	oss << "$$ ExecFrame #" << execframe_id << ": Statistics" << std::endl;
	oss << "$$   vTracking_FrameIDs = " << vTracking_FrameIDs << std::endl;

	for(int f=0; f<(int)vTracking_FrameIDs.size(); f++) {
		oss << "$$ ---- Tracking Frame #" << vTracking_FrameIDs[f] << " ----" << std::endl;
		oss << print_ExecTime_vs_ModelDecision_Distribution(map_tracking_frame_to_exectime_distribution[ vTracking_FrameIDs[f] ]);
		oss << "$$" << std::endl;
	}
	
	return oss.str();
}





} //namespace Opp
