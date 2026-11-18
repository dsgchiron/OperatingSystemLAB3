CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -O0 -pthread
TSAN   = -fsanitize=thread

PROGRAMS = part1-race/counter part2-buffer/bbuf part3-deadlock/bank
COPIES   = bonus-semaphore/bbuf_sem bonus-semaphore/bbuf_sem_swap

all: $(PROGRAMS) $(addsuffix -tsan,$(PROGRAMS))

%: %.c
	$(CC) $(CFLAGS) $< -o $@

%-tsan: %.c
	$(CC) $(CFLAGS) $(TSAN) $< -o $@

clean:
	rm -f $(PROGRAMS) $(COPIES) $(addsuffix -tsan,$(PROGRAMS) $(COPIES))

.PHONY: all clean
