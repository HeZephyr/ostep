CFLAGS := -Wall -Werror -I../include -pthread

OS     := $(shell uname -s)
LIBS   := 
ifeq ($(OS),Linux)
	LIBS += -pthread
endif

SRCS := join_modular.c \
		wait_child_spin.c \
		wait_child_use_CV.c \
		join_no_lock.c \
		join_no_state_var.c \
		join.c \
		pc.c \
		pc_single_cv.c \
		main-two-cvs-while.c

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS}: %: %.o Makefile
	${CC} ${CFLAGS} -o $@ ${@:=.o} ${LIBS}

clean:
	rm -f ${PROGS} ${OBJS}

%.0: %.c
	${CC} ${CFLAGS} -c $<