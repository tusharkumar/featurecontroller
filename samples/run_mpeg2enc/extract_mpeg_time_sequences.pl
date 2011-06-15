#!/usr/bin/perl -w

while(<>) {
	if(/decision_vector_int_value = (\d+)/) {
		print $1;
		print " ";
	}
	if(/ rescaled_current_invocation_exec_time = (\d+\.\d+)/) {
		print $1;
		print " ";
	}
	if(/occurred: current_exec_time_as_bin_index = (\d+)/) {
		print $1;
		print "\n";
	}
}

