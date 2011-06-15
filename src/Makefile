TARGET=libsrt.a

all: $(TARGET)

test: opp_test.exe


CFLAGS=-Wall -g

INCLUDES= c_opp.h \
		opp.h \
		opp_func.h \
		opp_baseframe.h \
		opp_frame.h \
		opp_exec_time_measure.h \
		opp_parameter_spread.h \
		opp_decision_model.h \
		opp_execframe.h \
		opp_frame_info.h \
		opp_timing.h \
		opp_utilities.h

CPP_SOURCES= c_opp.cpp \
		opp_baseframe.cpp \
		opp_decision_model.cpp \
		opp_exec_time_measure.cpp \
		opp_execframe.cpp \
		opp_frame.cpp \
		opp_func.cpp \
		opp_parameter_spread.cpp \
		opp_timing.cpp \
		opp_statistics.cpp

opp_srt_version.h: $(INCLUDES) $(CPP_SOURCES) Makefile
	echo \#define SRT_VERSION \"$$(basename $(CURDIR))\" > opp_srt_version.h

OBJECTS=$(CPP_SOURCES:.cpp=.o)

$(OBJECTS): $(INCLUDES)

opp_execframe.o: opp_srt_version.h

.cpp.o:
	g++ $(CFLAGS) -c $<

$(TARGET): $(OBJECTS)
	ar -r $(TARGET) $(OBJECTS)

opp_test.exe: $(TARGET) opp_test.cpp
	g++ $(CFLAGS) opp_test.cpp -L. -lsrt -o opp_test.exe
clean:
	rm -f *.o $(TARGET) opp_test.exe opp_test.exe.stackdump
