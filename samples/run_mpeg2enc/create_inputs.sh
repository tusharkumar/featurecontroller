#!/bin/sh

(cd ../mediabench2_video/mpeg2enc/mpeg2/src/mpeg2dec && make) \
&& ../mediabench2_video/mpeg2enc/mpeg2/src/mpeg2dec/mpeg2decode -b dolbycity640x480.m2v -o3 rec%d

