#ifndef SRT_MPEG2ENC_INTERFACE_H
#define SRT_MPEG2ENC_INTERFACE_H

#include "c_opp.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char * plot_filename;

typedef enum {
  fMAIN, fMOTION_EST, FRAME_COUNT
} SRT_Frame_Names_e;

typedef enum {
  efCHOOSE_SEARCH_WINDOW, EXECFRAME_COUNT
} SRT_ExecFrame_Names_e;

Opp_FrameID_t frame_name_to_id[FRAME_COUNT];
Opp_FrameID_t execframe_name_to_id[EXECFRAME_COUNT];

extern Opp_Caller oc_window_0;
extern Opp_Caller oc_window_1;
extern Opp_Caller oc_window_2;
extern Opp_Caller oc_window_3;
extern Opp_Caller oc_window_4;
extern Opp_Caller oc_window_5;
extern Opp_Caller oc_window_6;
extern Opp_Caller oc_window_7;

void fmain_fractional_enter();
void fmain_fractional_exit();

void init_srt_interface();

void print_srt_stats_for_frame(SRT_Frame_Names_e frame_name);
void print_srt_stats_for_execframe(SRT_ExecFrame_Names_e execframe_name);
#ifdef __cplusplus
} //extern "C"
#endif

#endif //SRT_MPEG2ENC_INTERFACE_H
