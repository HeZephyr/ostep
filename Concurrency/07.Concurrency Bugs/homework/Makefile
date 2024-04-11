
CFLAGS := -Wall -Werror -I../../include -pthread

COMMON = vector-header.h main-common.c main-header.h

OS     := $(shell uname -s)
LIBS   := 
ifeq ($(OS),Linux)
	LIBS += -pthread
endif

SRCS   := vector-deadlock.c \
	vector-global-order.c \
	vector-try-wait.c \
	vector-avoid-hold-and-wait.c \
	vector-nolock.c

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile
	${CC} $< -o $@ ${LIBS}

clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} ${common} -c $<