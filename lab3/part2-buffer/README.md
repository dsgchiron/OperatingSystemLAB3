# Частина 2. Producer—consumer

Тема: умовні змінні. Інформація: OSTEP [30. Condition Variables](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf) (розділ про producer/consumer — саме ця задача).

## Мета

Реалізувати обмежений буфер (bounded buffer), у якому producers чекають на вільне місце, а consumers — на елемент.

## Дано

`bbuf.c`:

```sh
./part2-buffer/bbuf PRODUCERS CONSUMERS ITEMS CAPACITY
```

- Буфер — кільцевий масив на CAPACITY елементів: `head` — звідки брати, `tail` — куди класти, `count` — скільки елементів зараз. Його захищає м'ютекс `lock`; на `not_full` чекають producers, на `not_empty` — consumers.
- Producers разом кладуть у буфер числа від 0 до ITEMS−1, кожне рівно один раз. Наприклад, з 2 producers і ITEMS = 6 перший кладе 0, 2, 4, а другий — 1, 3, 5.
- Consumer бере елементи, доки не отримає елемент зі значенням `STOP`.
- Наприкінці програма друкує, скільки елементів узяв кожен consumer і скільки разів потоки чекали, і перевіряє, що кожне число отримано рівно один раз (`check: ok`).

## Завдання

1. Заповніть **TODO 1** (`buffer_put`) і **TODO 2** (`buffer_take`). Обидві функції мають однакову будову:

   ```c
   pthread_mutex_lock(&m);
   while (!умова)
       pthread_cond_wait(&cv, &m);
   /* тут умова виконується, і м'ютекс у нас */
   ...
   pthread_cond_signal(&інша_cv);
   pthread_mutex_unlock(&m);
   ```

   `pthread_cond_wait` атомарно відпускає м'ютекс і засинає, а прокинувшись, знову захоплює м'ютекс і лише тоді повертається.

2. Заповніть **TODO 3** у `main`: зробіть так, щоб consumers завершилися, коли producers поклали всі елементи. Без цього програма зависає: producers закінчили, а consumers чекають на елементи, яких уже не буде, тож `pthread_join` для consumers ніколи не повертається.

3. Перевірте:

   ```sh
   make
   timeout 10 ./part2-buffer/bbuf 1 1 100000 4
   timeout 10 ./part2-buffer/bbuf 4 4 100000 4
   timeout 10 ./part2-buffer/bbuf-tsan 3 3 100000 2
   for i in $(seq 20); do timeout 30 ./part2-buffer/bbuf 4 4 100000 1 > /dev/null || echo "FAIL $i"; done
   ```

   Щоразу має бути `check: ok`, TSan — без попереджень, цикл нічого не друкує.

4. Вплив розміру буфера. Запустіть з 2 producers, 2 consumers і 100 000 елементів для розмірів буфера (CAPACITY) 1, 4, 64, 1024, а для 1 і 64 ще й на одному ядрі:

   ```sh
   for c in 1 4 64 1024; do ./part2-buffer/bbuf 2 2 100000 $c; done
   taskset -c 0 ./part2-buffer/bbuf 2 2 100000 1
   taskset -c 0 ./part2-buffer/bbuf 2 2 100000 64
   ```

   Поясніть, як від розміру буфера залежать кількість очікувань і час, а також що швидше для буфера на 1 елемент: одне ядро чи кілька, і чому.

## Запитання для самоперевірки

- Навіщо `pthread_cond_wait` приймає м'ютекс? Що піде не так, якщо перевірити умову, відпустити м'ютекс і лише потім викликати `wait`?
- З яких двох причин умова після пробудження може знову бути хибною?
- Чому `buffer_put` будить `not_empty`, а не `not_full`?
- Скільки STOP потрібно і в який момент їх класти? Чому саме стільки і саме тоді? Що буде, якщо кожен producer покладе один STOP, коли закінчить?
- Чи можна обійтися без `count`, лише з `head` і `tail`?
