// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <map>
#include <algorithm>

#include "opp_parameter_spread.h"

namespace Opp {


std::vector<int> ParameterExecSpread::get_complementary_bin_indices(std::vector<int> vGivenSpreadBinIndices) {
	std::sort(vGivenSpreadBinIndices.begin(), vGivenSpreadBinIndices.end()); //sorts local vector ascending
		//Assumption: vGivenSpreadBinIndices does not contain repeated bin-indices

	std::vector<int> vComplementarySpreadBinIndices;
	int j = 0; //indexes vGivenSpreadBinIndices
	for(int i=0; i<(int)vExecSpreadBins.size(); i++) {
		if(j < (int)vGivenSpreadBinIndices.size()) {
			int curr_given_bin_index = vGivenSpreadBinIndices[j];
			if(curr_given_bin_index > i)
				vComplementarySpreadBinIndices.push_back(i);
			else if(curr_given_bin_index == i)
				j++;
			else
				assert(0);
		}
		else
		{ vComplementarySpreadBinIndices.push_back(i); }
	}

	return vComplementarySpreadBinIndices;
}

void ParameterExecSpread::get_discriminating_values(
	std::vector<int> vGivenSpreadBinIndices,
	double FOR_discrimination_factor,
	std::vector<int>& result_FOR_vValues,
	std::vector<double>& result_FOR_vCounts,
	std::vector<double>& result_FOR_vProbs
)
{

	result_FOR_vValues.clear();
	result_FOR_vCounts.clear();
	result_FOR_vProbs.clear();


	std::vector<int> vOpposingSpreadBinIndices = get_complementary_bin_indices(vGivenSpreadBinIndices);

	//Now: vOpposingSpreadBinIndices contains the complement of the bin-indices in vGivenSpreadBinIndices

	std::map<int, double> map_FOR_value_to_count;
	std::map<int, double> map_AGAINST_value_to_count;
		//both maps will have identical keys

	for(int j=0; j<(int)vGivenSpreadBinIndices.size(); j++) {
		int bin_index = vGivenSpreadBinIndices[j];
		assert(0 <= bin_index && bin_index < (int)vExecSpreadBins.size());

		IntValueCache& spread_bin = vExecSpreadBins[bin_index];
		for(int k=0; k<(int)spread_bin.vCacheEntries.size(); k++) { //for each tag-value in cache
			IntCacheEntry& ce = spread_bin.vCacheEntries[k];
			if(ce.valid) {
				if(map_FOR_value_to_count.count(ce.tag) == 0)
					map_FOR_value_to_count[ce.tag] = 0.0;
				if(map_AGAINST_value_to_count.count(ce.tag) == 0)
					map_AGAINST_value_to_count[ce.tag] = 0.0;

				map_FOR_value_to_count[ce.tag] += ce.count;
			}
		}
	}

	for(int j=0; j<(int)vOpposingSpreadBinIndices.size(); j++) {
		int bin_index = vOpposingSpreadBinIndices[j];
		assert(0 <= bin_index && bin_index < (int)vExecSpreadBins.size());

		IntValueCache& spread_bin = vExecSpreadBins[bin_index];
		for(int k=0; k<(int)spread_bin.vCacheEntries.size(); k++) { //for each tag-value in cache
			IntCacheEntry& ce = spread_bin.vCacheEntries[k];
			if(ce.valid) {
				if(map_FOR_value_to_count.count(ce.tag) == 0)
					map_FOR_value_to_count[ce.tag] = 0.0;
				if(map_AGAINST_value_to_count.count(ce.tag) == 0)
					map_AGAINST_value_to_count[ce.tag] = 0.0;

				map_AGAINST_value_to_count[ce.tag] += ce.count;
			}
		}
	}

	double spread_total_sample_count = 0.0;
	for(int i=0; i<(int)vExecSpreadBins.size(); i++)
		spread_total_sample_count += vExecSpreadBins[i].get_sample_count();
	if(spread_total_sample_count == 0.0)
		spread_total_sample_count = 1.0; //avoid divide-by-zero, all numerators will be 0.0 anyways

	std::vector<int> vSorted_Values; //occuring values in sorted order

	for(std::map<int, double>::iterator mit_FOR = map_FOR_value_to_count.begin();
			mit_FOR != map_FOR_value_to_count.end();
			mit_FOR++
	) {
		int value = mit_FOR->first;
		vSorted_Values.push_back(value);
	}
	std::sort(vSorted_Values.begin(), vSorted_Values.end()); //sort ascending

	for(int i=0; i<(int)vSorted_Values.size(); i++) {
		int value = vSorted_Values[i];
		double count_FOR = map_FOR_value_to_count[value];
		double count_AGAINST = map_AGAINST_value_to_count[value];
		double total_count = count_FOR + count_AGAINST;

		if(count_FOR / total_count >= FOR_discrimination_factor) {
			result_FOR_vValues.push_back(value);
			result_FOR_vCounts.push_back(count_FOR / spread_total_sample_count);
			result_FOR_vProbs.push_back(count_FOR / total_count);
		}
	}
}


void ParameterExecSpread::detect_parameter_discrimination_to_spread(
	bool& bIsRunExercising,
	double& min_exercising_run_length,
	double& max_D_statistic
)
{
	//Steps
	//1) Identify "dominant" spread-bins based on invocation counts. Ignore all other "insignificant" bins subsequently.
	//
	//2) Domain values:
	//   - Identify all paramater values that occur across all dominant bins.
	//   - Sort asecending.
	//   - These will be the domain of the CDFs.
	//
	//3) Compute min_exercising_run_length:
	//   - min_exercising_run_length = num-dominant-spread-bins * num-parameter-values-in-domain + 1
	//
	//4) Compute bIsRunExercising:
	//   - bIsRunExercising = (total-invocation-count-across-dominant-bins >= min_exercising_run_length)
	//
	//5) If bIsRunExercising == true, Compute maximal D statistic (else = 0.0; 1.0 if only one bin and one value):
	//   - Compute CDFs progressively for all the dominant bins
	//     (each step is next parameter value from domain in ascending order)
	//   - At each step, update max D statistic by comparing against the step's max_CDF - min_CDF value

	double total_sample_count = current_sample_count();
	double dominant_sample_count_threshold = total_sample_count * 0.01; //Should constitute atleast 1% of invocations

	std::vector<int> vDominantBinIndices;
	for(int i=0; i<(int)vExecSpreadBins.size(); i++)
		if(vExecSpreadBins[i].get_sample_count() >= dominant_sample_count_threshold)
			vDominantBinIndices.push_back(i);

	std::vector<int> vParameterValuesConcatenation;
	for(int bi=0; bi<(int)vDominantBinIndices.size(); bi++) {
		IntValueCache& exec_bin = vExecSpreadBins[ vDominantBinIndices[bi] ];
		for(int j=0; j<(int)exec_bin.vCacheEntries.size(); j++) {
			IntCacheEntry& entry = exec_bin.vCacheEntries[j];
			if(entry.valid == true)
				vParameterValuesConcatenation.push_back(entry.tag);
		}
	}
	std::sort(vParameterValuesConcatenation.begin(), vParameterValuesConcatenation.end());

	std::vector<int> vDomainParameterValues;
	for(int i=0; i<(int)vParameterValuesConcatenation.size(); i++) { //eliminate duplicated parameter values
		if((int)vDomainParameterValues.size() == 0
			|| vDomainParameterValues[vDomainParameterValues.size()-1] != vParameterValuesConcatenation[i]
		)
		{ vDomainParameterValues.push_back( vParameterValuesConcatenation[i] ); }
	}

	min_exercising_run_length = (double)vDominantBinIndices.size() * (double)vDomainParameterValues.size() + 1.0;
	if(min_exercising_run_length > max_count)
		min_exercising_run_length = max_count;

	bIsRunExercising = (total_sample_count >= min_exercising_run_length);

	if(bIsRunExercising == false) {
		max_D_statistic = 0.0;
		return;
	}

	// Now, bIsRunExercising == true

	assert((int)vDominantBinIndices.size() >= 1);
	if((int)vDominantBinIndices.size() == 1) { //only a single CDF => D statistic cannot be computed
		if((int)vDomainParameterValues.size() <= 1)
			max_D_statistic = 1.0; //only one exec-bin, only one parameter-value bin => no additional discrimination needed
		else
			max_D_statistic = 0.0; //different parameter-values are unable to force separation into multiple exec-bins
		return;
	}

	//general case
	assert((int)vDomainParameterValues.size() > 0);
	max_D_statistic = 0.0;
	std::vector<double> vProgCDF(vDominantBinIndices.size(), 0.0);
	for(int vi=0; vi<(int)vDomainParameterValues.size(); vi++) {
		int value = vDomainParameterValues[vi];

			//assumption: vDominantBinIndices.size() > 0, therefore step_m??_CDF will always be corrected
		double step_min_CDF = 1.0;
		double step_max_CDF = 0.0;
		for(int bi=0; bi<(int)vDominantBinIndices.size(); bi++) {
			const IntValueCache& exec_bin = vExecSpreadBins[ vDominantBinIndices[bi] ];
			double value_count_in_bin = exec_bin.tag_occurence_count(value);
			double prob = value_count_in_bin / exec_bin.get_sample_count();
			vProgCDF[bi] += prob;

			if(step_min_CDF > vProgCDF[bi])
				step_min_CDF = vProgCDF[bi];
			if(step_max_CDF < vProgCDF[bi])
				step_max_CDF = vProgCDF[bi];
		}
		assert(step_max_CDF >= step_min_CDF);
		double step_D = step_max_CDF - step_min_CDF;
		if(step_D > max_D_statistic)
			max_D_statistic = step_D;
	}
}

} //namespace Opp
