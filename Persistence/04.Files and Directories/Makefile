CC     := gcc


SRCS   := fork-seek.c dup.c fsync.c read_dir.c \
			stat.c myls.c mytail.c myfind.c

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile
	${CC} $< -o $@

clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} -c $<
