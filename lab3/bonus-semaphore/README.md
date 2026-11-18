# Бонус. Обмежений буфер на семафорах

Виконується після частини 2. Інформація: OSTEP [31. Semaphores](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-sema.pdf) (розділ The Producer/Consumer (Bounded Buffer) Problem), `man sem_overview`.

## Мета

Реалізувати той самий обмежений буфер на POSIX-семафорах замість умовних змінних і порівняти два підходи.

## Дано

Ваш `part2-buffer/bbuf.c`. Семафор — лічильник, який не опускається нижче нуля: `sem_wait` зменшує його, а якщо він уже 0, чекає, доки хтось не викличе `sem_post`; `sem_post` збільшує його і будить одного з тих, хто чекає. На відміну від умовної змінної, семафор пам'ятає `sem_post`, навіть якщо в цей момент ніхто не чекав.

## Завдання

1. Скопіюйте буфер:

   ```sh
   cp part2-buffer/bbuf.c bonus-semaphore/bbuf_sem.c
   ```

   У копії замініть обидві умовні змінні двома семафорами: `free_slots` (скільки вільних місць, спочатку `capacity`) і `used_slots` (скільки елементів, спочатку 0). Потрібні `#include <semaphore.h>` і `sem_init(&s, 0, початкове_значення)`; другий аргумент 0 означає, що семафор спільний між потоками одного процесу. М'ютекс лишається для `head` і `tail`. `count`, `waits_full`, `waits_empty` і їхній `printf` більше не потрібні.

2. Зберіть і перевірте так само, як у кроці 3 частини 2:

   ```sh
   make bonus-semaphore/bbuf_sem bonus-semaphore/bbuf_sem-tsan
   timeout 10 ./bonus-semaphore/bbuf_sem 4 4 100000 4
   timeout 10 ./bonus-semaphore/bbuf_sem-tsan 3 3 100000 2
   for i in $(seq 20); do timeout 30 ./bonus-semaphore/bbuf_sem 4 4 100000 1 > /dev/null || echo "FAIL $i"; done
   ```

3. Порівняйте час з частиною 2 на `2 2 100000` з розміром буфера 1 і 64.

4. Скопіюйте `bbuf_sem.c` у `bbuf_sem_swap.c` і в копії поміняйте місцями `sem_wait` і `pthread_mutex_lock` в обох функціях, тобто спершу захоплюйте м'ютекс, а потім чекайте на семафор:

   ```sh
   cp bonus-semaphore/bbuf_sem.c bonus-semaphore/bbuf_sem_swap.c
   # редагування
   make bonus-semaphore/bbuf_sem_swap
   timeout 10 ./bonus-semaphore/bbuf_sem_swap 2 2 100000 1; echo $?
   ```

   Поясніть, що сталося.

## Запитання для самоперевірки

- Навіщо м'ютекс, якщо семафори вже рахують елементи? Чи потрібен він, коли producer і consumer по одному?
- Чому в цій версії немає циклу `while` навколо `sem_wait`, а в частині 2 він обов'язковий?
- Чим семафор зі значенням 1 схожий на м'ютекс і чим відрізняється?
