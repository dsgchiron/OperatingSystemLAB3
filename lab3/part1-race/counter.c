#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_THREADS 64

static long counter;
static long iterations;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

static void *increment_racy(void *arg) {
  (void)arg;
  for (long i = 0; i < iterations; i++)
    counter++;
  return NULL;
}

static void *increment_locked(void *arg) {
  (void)arg;
  /* TODO 1: код повинен виконувати те саме, що increment_racy, але кожен
   * counter++ потрібно виконувати під мʼютексом counter_lock:
   * pthread_mutex_lock перед інкрементом, pthread_mutex_unlock після. */
  for (long i = 0; i < iterations; i++) {
    pthread_mutex_lock(&counter_lock);
    counter++;
    pthread_mutex_unlock(&counter_lock);
  }
  return NULL;
}

static void *increment_local(void *arg) {
  (void)arg;
  /* TODO 2: робити інкремент локальній змінній в циклі, після цього додати
   * значення змінної до counter "під counter_lock". */
  long local = 0;
  for (long i = 0; i < iterations; i++)
    local++;
  pthread_mutex_lock(&counter_lock);
  counter += local;
  pthread_mutex_unlock(&counter_lock);
  return NULL;
}

static long parse_positive(const char *text, long max) {
  char *end;
  long value = strtol(text, &end, 10);
  if (end == text || *end != '\0' || value <= 0 || value > max) {
    fprintf(stderr, "invalid number: %s (expected 1..%ld)\n", text, max);
    exit(2);
  }
  return value;
}

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s racy|locked|local THREADS ITERATIONS\n",
            argv[0]);
    return 2;
  }

  void *(*worker)(void *);
  if (strcmp(argv[1], "racy") == 0)
    worker = increment_racy;
  else if (strcmp(argv[1], "locked") == 0)
    worker = increment_locked;
  else if (strcmp(argv[1], "local") == 0)
    worker = increment_local;
  else {
    fprintf(stderr, "unknown mode: %s\n", argv[1]);
    return 2;
  }
  long threads = parse_positive(argv[2], MAX_THREADS);
  iterations = parse_positive(argv[3], 1000000000);

  pthread_t tids[MAX_THREADS];
  double start = now_ms();
  for (long i = 0; i < threads; i++) {
    int err = pthread_create(&tids[i], NULL, worker, NULL);
    if (err != 0) {
      fprintf(stderr, "pthread_create: %s\n", strerror(err));
      return 1;
    }
  }
  for (long i = 0; i < threads; i++)
    pthread_join(tids[i], NULL);
  double elapsed = now_ms() - start;

  long expected = threads * iterations;
  printf("%-6s threads: %2ld  counter: %11ld  expected: %11ld  lost: %11ld  "
         "time: %8.1f ms\n",
         argv[1], threads, counter, expected, expected - counter, elapsed);
  return 0;
}
