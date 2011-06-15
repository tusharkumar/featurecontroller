#!/usr/bin/perl -w

use strict;

my $EXECUTABLE = "../mediabench2_video/mpeg2enc/mpeg2/src/mpeg2enc/mpeg2encode";

my %explore_space;
$explore_space{"mean"} = [0.12, 0.02, 0.04, 0.06, 0.08, 0.16];
$explore_space{"window_frac"} = [0.20, 0.10, 0.40];
$explore_space{"sliding_window"} = [7, 3, 11, 15];
$explore_space{"coeff"} = [-1, 1/100.0, 1/50.0, 1/20.0, 1/16.0, 1/8.0, 1/4.0, 1/2.0, 2, 10, 20];
$explore_space{"srt"} = [-1, 0, 1, 2, 3, 4, 5, 6, 7];
$explore_space{"test"} = ["testing", "running"];


my $usage = "
Usage: run_exp.perl <log_number> [srt] [mean] [window_frac] [sliding_window] [coeff] [test]
	If one or more of mean, window_frac, sliding_window, coeff are specified, then only the default case for that aspect will be run.

	srt => no fixed-level cases will be run
	coeff => no fixed-coefficient srt cases will be run
	test => show lists of commands, but does not execute any

Recommended:
	1) Importance of a good sliding window (SL runs):
		./run_exp.perl <> mean window_frac coeff

	2) For demonstrating the need to carefully choose coeff value for SRT system to perform well (CV runs):
		./run_exp.perl <> mean window_frac sliding_window srt

	3) For studyimg efficacy of SRT system in different use-scenarios
	  a) Mean setting: simulate architecture variations (ME runs)
		./run_exp.perl <> window_frac sliding_window coeff

	  b) Window-fraction setting: simulate different levels of stringent requirements (WF runs)
		./run_exp.perl <> mean sliding_window coeff

	4) Single general SRT run on default settings
		./run_exp.perl <> mean window_frac sliding_window coeff srt

";

die $usage if (@ARGV < 1);

for(my $i=1; $i<@ARGV; $i++) {
	my $arg = $ARGV[$i];
	die "Unknown option: $arg\n$usage" if($explore_space{$arg} == undef);
	$explore_space{$arg} = [ $explore_space{$arg}[0] ]; #limit to just the default case
	print "## Limiting exploration of \"$arg\" to " . $explore_space{$arg}[0] . "\n";
}



###
my $log_number = $ARGV[0];
print "log_number = $log_number\n";


#my @configs = ("", "_lbr", "_vlbr");
my @configs = ("");

my $multi_frame_count = 1;
my $stickiness_length = 0;

foreach my $config (@configs) {
	foreach my $level (@{$explore_space{"srt"}}) {

		foreach my $mean (@{$explore_space{"mean"}}) {
			foreach my $window_frac (@{$explore_space{"window_frac"}}) {
				foreach my $sliding_window (@{$explore_space{"sliding_window"}}) {
					COEFF: foreach my $coeff (@{$explore_space{"coeff"}}) {

my $level_string;
if($level >= 0)
{ $level_string = "fixed$level"; }
else
{ $level_string = "srt"; }

my $suffix = "${config}_${level_string}_${log_number}_mean${mean}_wf${window_frac}_sl${sliding_window}";

my $fast_reaction_strategy_coeff = $coeff;
my $force_fixed_frs_coeff = 1;
if($coeff == -1) {
	$fast_reaction_strategy_coeff = 0.0;
	$force_fixed_frs_coeff = 0;
}

if($level_string eq "srt") {
	if($fast_reaction_strategy_coeff == 0.0)
	{ $suffix = $suffix . "_rescale"; }
	else
	{ $suffix = $suffix . "_fixedcoeff${fast_reaction_strategy_coeff}"; }
}


my $command = "SRT_EXP_PLOT_FILENAME=plot${suffix} SRT_EXP_MULTI_FRAME_COUNT=${multi_frame_count} SRT_EXP_MEAN=$mean SRT_EXP_WINDOW_FRAC=$window_frac SRT_EXP_MODE=$level SRT_EXP_STICKINESS_LENGTH=${stickiness_length} SRT_EXP_SLIDING_WINDOW=${sliding_window} SRT_EXP_FAST_REACTION_STRATEGY_COEFF=$fast_reaction_strategy_coeff SRT_EXP_FORCE_FIXED_FRS_COEFF=$force_fixed_frs_coeff time $EXECUTABLE config${config}.par out${suffix}.m2v > log${suffix} 2>&1";
print "--- Invoking --- $suffix\n";
print "$command\n";

if(@{$explore_space{"test"}} > 1) {
	system $command;

	system "mv output_base_4CIF_96bps_15.stat stat${suffix}";
	system "perl extract_mpeg_time_sequences.pl log${suffix} > xy${suffix}";
}

last COEFF if(not $level_string eq "srt"); #fixed-levels don't use fast_reaction_strategy_coeff

					}
				}
			}
		}

	}
}
