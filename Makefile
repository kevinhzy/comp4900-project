
#
#	Makefile for comp4900-project
#

DEBUG = -g
CC = qcc
LD = qcc


TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntox86
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le


CFLAGS += $(DEBUG) $(TARGET) -Wall
LDFLAGS+= $(DEBUG) $(TARGET)
BINS = simulator sample_BlockController sample_Intersection
all: $(BINS)

clean:
	rm -f *.o $(BINS);
#	cd solutions; make clean


#server.o: server.c server.h
#client.o: client.c server.h

sample_BlockController.o: sample_BlockController.c constants.h
sample_Intersection.o: sample_Intersection.c constants.h
#simulator.o: simulator.c
