CC     := gcc
CFLAGS := -Wall -Werror -I ../../include/ -pthread

OS     := $(shell uname -s)
LIBS   := 
ifeq ($(OS),Linux)
	LIBS += -pthread
endif

SRCS   := fork-join.c rendezvous.c barrier.c \
		  reader-writer.c reader-writer-nostarve.c \
		  mutex-nostarve.c dining_savages.c \
		  simple_cigarette_smokers.c complex_cigarette_smokers.c \

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile
	${CC} $< -o $@ ${LIBS}

clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<
