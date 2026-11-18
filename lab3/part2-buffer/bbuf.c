#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_THREADS 64
#define STOP (-1)

struct buffer {
    int *items;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    long waits_full;
    long waits_empty;
};

static struct buffer buf;
static int producers;
static int items_total;
static unsigned char *seen;

static void buffer_put(struct buffer *b, int item)
{
    /* TODO 1: захопити b->lock; поки буфер повний, чекати на b->not_full
     * (кожен виклик pthread_cond_wait рахувати в b->waits_full); покласти item у
     * b->items[b->tail], зсунути tail по колу, збільшити count; повідомити
     * consumer через b->not_empty; відпустити lock. */
    pthread_mutex_lock(&b->lock);
    while (b->count == b->capacity) {
        b->waits_full++;
        pthread_cond_wait(&b->not_full, &b->lock);
    }
    b->items[b->tail] = item;
    b->tail = (b->tail + 1) % b->capacity;
    b->count++;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->lock);
}

static int buffer_take(struct buffer *b)
{
    /* TODO 2: дзеркально до buffer_put: чекати, поки буфер порожній
     * (рахувати в b->waits_empty), взяти елемент з b->items[b->head],
     * повідомити producer через b->not_full. */
    pthread_mutex_lock(&b->lock);
    while (b->count == 0) {
        b->waits_empty++;
        pthread_cond_wait(&b->not_empty, &b->lock);
    }
    int item = b->items[b->head];
    b->head = (b->head + 1) % b->capacity;
    b->count--;
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->lock);
    return item;
}

static void *producer(void *arg)
{
    int id = *(int *)arg;
    for (int item = id; item < items_total; item += producers)
        buffer_put(&buf, item);
    return NULL;
}

struct consumer {
    pthread_t thread;
    long taken;
};

static void *consumer(void *arg)
{
    struct consumer *self = arg;
    for (;;) {
        int item = buffer_take(&buf);
        if (item == STOP)
            break;
        seen[item]++;
        self->taken++;
    }
    return NULL;
}

static long parse_positive(const char *text, long max)
{
    char *end;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0 || value > max) {
        fprintf(stderr, "invalid number: %s (expected 1..%ld)\n", text, max);
        exit(2);
    }
    return value;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static void start_thread(pthread_t *thread, void *(*fn)(void *), void *arg)
{
    int err = pthread_create(thread, NULL, fn, arg);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: %s PRODUCERS CONSUMERS ITEMS CAPACITY\n", argv[0]);
        return 2;
    }
    producers = parse_positive(argv[1], MAX_THREADS);
    int consumers = parse_positive(argv[2], MAX_THREADS);
    items_total = parse_positive(argv[3], 100000000);
    buf.capacity = parse_positive(argv[4], 1000000);

    buf.items = malloc(buf.capacity * sizeof *buf.items);
    seen = calloc(items_total, sizeof *seen);
    if (buf.items == NULL || seen == NULL) {
        perror("malloc");
        return 1;
    }
    pthread_mutex_init(&buf.lock, NULL);
    pthread_cond_init(&buf.not_full, NULL);
    pthread_cond_init(&buf.not_empty, NULL);

    pthread_t producer_threads[MAX_THREADS];
    int producer_ids[MAX_THREADS];
    struct consumer consumer_threads[MAX_THREADS] = {0};

    double start = now_ms();
    for (int i = 0; i < consumers; i++)
        start_thread(&consumer_threads[i].thread, consumer, &consumer_threads[i]);
    for (int i = 0; i < producers; i++) {
        producer_ids[i] = i;
        start_thread(&producer_threads[i], producer, &producer_ids[i]);
    }

    for (int i = 0; i < producers; i++)
        pthread_join(producer_threads[i], NULL);
    /* TODO 3: завершити consumers. */
    for (int i = 0; i < consumers; i++)
        buffer_put(&buf, STOP);
    for (int i = 0; i < consumers; i++)
        pthread_join(consumer_threads[i].thread, NULL);
    double elapsed = now_ms() - start;

    printf("producers: %d  consumers: %d  items: %d  capacity: %d  time: %.1f ms\n",
           producers, consumers, items_total, buf.capacity, elapsed);
    for (int i = 0; i < consumers; i++)
        printf("  consumer %d took %ld\n", i, consumer_threads[i].taken);
    printf("waits: full %ld, empty %ld\n", buf.waits_full, buf.waits_empty);

    long missing = 0, duplicated = 0;
    for (int i = 0; i < items_total; i++) {
        if (seen[i] == 0)
            missing++;
        else if (seen[i] > 1)
            duplicated++;
    }
    if (missing == 0 && duplicated == 0) {
        printf("check: ok\n");
        return 0;
    }
    printf("check: FAILED, missing %ld, duplicated %ld\n", missing, duplicated);
    return 1;
}
