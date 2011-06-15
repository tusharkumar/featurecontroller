// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_PARAMETER_SPREAD_H
#define OPP_PARAMETER_SPREAD_H

#include <vector>
#include <map>

#include "opp.h"

namespace Opp {


	/////////////////////////////////
	// Caching
	/////////////////////////////////

	class IntCacheEntry {
	public:
		bool valid;
		int tag; //should be >= 0 when valid
		double count;

		IntCacheEntry()
			: valid(false), tag(-1), count(0.0) { }

		inline void initialize(int new_tag) {
			valid = true;
			tag = new_tag;
			count = 0.0;
		}
		//when Cache reaches maximum count, degrade each
		//  cache-entry that did not occur in the current sample
		inline void downgrade(double downgrade_ratio) {
			count = count * downgrade_ratio;
		}

		inline void note_occurence(double add_count = 1.0) {
			count = count + add_count;
		}
	};

	class IntValueCache {
	//caches mapping between observed int-values and their occurence-counts
	public:
		std::vector<IntCacheEntry> vCacheEntries;

	private:
		double max_count;
		double sample_count;
		//Invariant: sample_count == sum-over-i(vCacheEntries[i].count)
		//  (holds mathematically, but in practice may be slightly different
		//    due to floating-point computation).

	public:
		IntValueCache(int num_entries = 0, double max_count = 0.0)
			: max_count(max_count), sample_count(0.0)
		{ vCacheEntries.resize(num_entries); }

		inline double get_sample_count() const { return sample_count; }
	private:
		//finds first empty slot, or evicts entry with lowest count
		int find_cache_entry_index() {
			int lowest_count_entry_index = -1;
			double lowest_entry_count = 0.0;
			for(int i=0; i<(int)vCacheEntries.size(); i++) {
				if(vCacheEntries[i].valid == false) //empty slot
					return i;
				if(lowest_count_entry_index == -1
						|| lowest_entry_count >= vCacheEntries[i].count)
				{ //eviction candidate
					lowest_count_entry_index = i;
					lowest_entry_count = vCacheEntries[i].count;
				}
			}

			return lowest_count_entry_index;
		}


	public:
		  //returns occurence-count of tag, 0.0 if tag does not occur
		double tag_occurence_count(int tag) const {
			for(int i=0; i<(int)vCacheEntries.size(); i++)
				if(vCacheEntries[i].valid && vCacheEntries[i].tag == tag)
					return vCacheEntries[i].count;
			return 0.0; //not found
		}

		  //evicts if necessary to accomodate given tag
		void note_sample(int tag, double add_count = 1.0) {
			int cache_entry_index = -1;
			for(int i=0; i<(int)vCacheEntries.size(); i++) {
				if(vCacheEntries[i].valid && vCacheEntries[i].tag == tag) {
					cache_entry_index = i;
					break;
				}
			}

			if(cache_entry_index == -1) { //tag not found
				//evict something if empty slot not found
				cache_entry_index = find_cache_entry_index();

				vCacheEntries[cache_entry_index].initialize(tag);

				//fix 'sample_count' due to eviction
				sample_count = 0.0;
				for(int i=0; i<(int)vCacheEntries.size(); i++) {
					if(vCacheEntries[i].valid)
						sample_count += vCacheEntries[i].count;
				}
			}

			vCacheEntries[cache_entry_index].note_occurence(add_count);

			sample_count = sample_count + add_count;
			if(sample_count > max_count) { //clamp at max_count
				double curr_sample_count = vCacheEntries[cache_entry_index].count;
				double downgrade_ratio = 0.0;
				if (curr_sample_count < max_count) {
					downgrade_ratio = (max_count - curr_sample_count) / (sample_count - curr_sample_count);
				}
				else { //curr_sample_count >= max_count ==> scrunch count of all other entries to 0.0, resize current entry to max_count
					downgrade_ratio = 0.0;
					vCacheEntries[cache_entry_index].count = max_count;
				}

				//downgrade entries untouched by current sample
				for(int i=0; i<(int)vCacheEntries.size(); i++) {
					if(i != cache_entry_index)
						vCacheEntries[i].downgrade(downgrade_ratio);
				}
				sample_count = max_count; //mathematically true after previous step
			}
		}

			//if sample_count == 0.0, then has no effect
		void normalize_wrt_new_sample_count(double new_sample_count) {
			if(sample_count == 0.0) //no way to scale any count
				return;

			if(new_sample_count > max_count)
				new_sample_count = max_count;

			double ratio = new_sample_count / sample_count;
			for(int i=0; i<(int)vCacheEntries.size(); i++) {
				if(vCacheEntries[i].valid == true)
					vCacheEntries[i].count *= ratio;
			}
			sample_count = new_sample_count;
		}

		void delete_cache_entry(int cache_entry_index) {
			assert(0 <= cache_entry_index && cache_entry_index <= (int)vCacheEntries.size());
			if(vCacheEntries[cache_entry_index].valid) {
				sample_count -= vCacheEntries[cache_entry_index].count;

				vCacheEntries[cache_entry_index].valid = false;
				vCacheEntries[cache_entry_index].tag = -1;
				vCacheEntries[cache_entry_index].count = 0.0;
			}
		}

		void clear() {
			for(int i=0; i<(int)vCacheEntries.size(); i++) {
				IntCacheEntry& ce = vCacheEntries[i];
				ce.valid = false;
				ce.tag = -1;
				ce.count = 0.0;
			}
			sample_count = 0.0;
		}
	};

	/////////////////////////////////
	class ParameterExecSpread {
		//Creates a parameter-value-cache for each spread-bin possible for a given entity
	public:
		static const int num_entries_per_cache = 10;
		static const double max_count = 1000.0;

		std::vector<IntValueCache> vExecSpreadBins;

		ParameterExecSpread(int num_spread_bins) {
			IntValueCache empty_cache(num_entries_per_cache, max_count);
			vExecSpreadBins.resize(num_spread_bins, empty_cache);
		}

		void note_spread_bin_occurence(
			int exec_sample_spread_bin_index,
			int occured_parameter_value_tag,
			double add_count = 1.0
		)
		{ vExecSpreadBins.at(exec_sample_spread_bin_index)
			.note_sample(occured_parameter_value_tag, add_count); }

		double current_sample_count() const {
			double sum = 0.0;
			for(int i=0; i<(int)vExecSpreadBins.size(); i++)
				sum += vExecSpreadBins[i].get_sample_count();
			return sum;
		}

		// Analysis support functionality

		std::vector<int> get_complementary_bin_indices(std::vector<int> vGivenSpreadBinIndices);

		void get_discriminating_values(
			std::vector<int> vGivenSpreadBinIndices, //must not repeat bin-indices
			double FOR_discrimination_factor,            //between 0 .. 1, representing 0% - 100%
			std::vector<int>& result_FOR_vValues,        //values discriminating FOR given-spread-bins
			std::vector<double>& result_FOR_vCounts,     //occurence-counts of corresponding FOR discriminating values, normalized against total-sample-count of all spread bins
			std::vector<double>& result_FOR_vProbs       //probability in favor of FOR, for corresponding discriminating values
		);
		//NOTE: Entries in result_FOR_vValues are sorted in ascending order of values

		void detect_parameter_discrimination_to_spread(
				//return values:
			bool& bIsRunExercising,            // Has the current run sufficiently exercised parameter vs. spread possibilities
			double& min_exercising_run_length, // Length of run needed for it to be exercising
			double& max_D_statistic            // Defined if bIsRunExercising==true, gives the maximal D statistic between
			                                   //    CDFs of parameter-value-probabilites across all execution-spread bins
		);
	};


} //namespace Opp

#endif //OPP_PARAMETER_SPREAD_H
