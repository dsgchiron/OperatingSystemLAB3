#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_THREADS 64
#define MAX_ACCOUNTS 1000
#define INITIAL_BALANCE 1000

struct account {
    int id;
    long balance;
    pthread_mutex_t lock;
};

static struct account accounts[MAX_ACCOUNTS];
static int account_count;
static long transfers_per_thread;

static void transfer(struct account *from, struct account *to, long amount)
{
    /* TODO: тут можливе взаємоблокування. Захоплюйте м'ютекси в одному
     * глобальному порядку, наприклад за зростанням id рахунку. */
    struct account *first = from->id < to->id ? from : to;
    struct account *second = from->id < to->id ? to : from;
    pthread_mutex_lock(&first->lock);
    pthread_mutex_lock(&second->lock);
    if (from->balance >= amount) {
        from->balance -= amount;
        to->balance += amount;
    }
    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

static void *teller(void *arg)
{
    unsigned int seed = *(unsigned int *)arg;
    for (long i = 0; i < transfers_per_thread; i++) {
        int from = rand_r(&seed) % account_count;
        int to = rand_r(&seed) % (account_count - 1);
        if (to >= from)
            to++;
        transfer(&accounts[from], &accounts[to], rand_r(&seed) % 100);
    }
    return NULL;
}

static long parse_positive(const char *text, long min, long max)
{
    char *end;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value < min || value > max) {
        fprintf(stderr, "invalid number: %s (expected %ld..%ld)\n", text, min, max);
        exit(2);
    }
    return value;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s THREADS ACCOUNTS TRANSFERS\n", argv[0]);
        return 2;
    }
    int threads = parse_positive(argv[1], 1, MAX_THREADS);
    account_count = parse_positive(argv[2], 2, MAX_ACCOUNTS);
    transfers_per_thread = parse_positive(argv[3], 1, 1000000000);

    for (int i = 0; i < account_count; i++) {
        accounts[i].id = i;
        accounts[i].balance = INITIAL_BALANCE;
        pthread_mutex_init(&accounts[i].lock, NULL);
    }

    printf("pid %d: %d threads, %d accounts, %ld transfers each\n",
           getpid(), threads, account_count, transfers_per_thread);
    fflush(stdout);

    pthread_t tids[MAX_THREADS];
    unsigned int seeds[MAX_THREADS];
    for (int i = 0; i < threads; i++) {
        seeds[i] = i + 1;
        int err = pthread_create(&tids[i], NULL, teller, &seeds[i]);
        if (err != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            return 1;
        }
    }
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);

    long total = 0;
    for (int i = 0; i < account_count; i++)
        total += accounts[i].balance;
    long expected = (long)account_count * INITIAL_BALANCE;
    printf("total balance: %ld (expected %ld)\n", total, expected);
    return total == expected ? 0 : 1;
}
