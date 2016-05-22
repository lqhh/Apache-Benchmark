CC=gcc
INCS=-I./src
CFLAGS=-g -Wall $(INCS)
ABBIN=ab
ABSRC=./src

ABFILES=$(ABSRC)/main.c         \
	$(ABSRC)/ab_param.c         \
	$(ABSRC)/ab_buf.c           \
	$(ABSRC)/ab_event.c         \
	$(ABSRC)/ab_connection.c    \
	$(ABSRC)/ab_test.c          \
	$(ABSRC)/ab_http.c          \
	$(ABSRC)/ab_stat.c          \
	$(ABSRC)/ab_log.c           \

ABHEADERS=$(ABSRC)/ab_param.h    \
	$(ABSRC)/ab_buf.h            \
	$(ABSRC)/ab_event.h          \
	$(ABSRC)/ab_connection.h     \
	$(ABSRC)/ab_test.h           \
	$(ABSRC)/ab_http.h           \
	$(ABSRC)/ab_stat.h           \
	$(ABSRC)/ab_log.h            \

ABOBJS=$(ABFILES:.c=.o)

.PHONY: all

all: $(ABBIN)

$(ABBIN): $(ABOBJS)
	$(CC) $(CFLAGS) $(ABOBJS) -o $(ABBIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $? -o $@

.PHONE: clean

clean:
	-rm -rf src/*.o
	-rm $(ABBIN)
