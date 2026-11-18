# Частина 1. Race condition і м'ютекс

Тема: потоки, race condition, м'ютекси. Інформація: OSTEP [26. Concurrency: An Introduction](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-intro.pdf), [27. Interlude: Thread API](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf), [28. Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf), [29. Lock-based Concurrent Data Structures](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks-usage.pdf) (розділ про лічильники).

## Мета

Побачити, як втрачаються оновлення спільної змінної, знайти data race за допомогою ThreadSanitizer (TSan), усунути м'ютексом і порівняти «ціну» м'ютекса в режимах `locked` (захоплення на кожен інкремент) і `local` (один раз на потік).

## Дано

`counter.c`: програма запускає THREADS потоків, кожен з них ITERATIONS разів збільшує глобальну змінну `counter`, і друкує результат, очікуване значення, кількість втрачених оновлень змінної і час:

```sh
./part1-race/counter racy|locked|local THREADS ITERATIONS
```

Режим `racy` вже імплементований: потоки збільшують `counter` без синхронізації. Режими `locked` і `local` потрібно реалізувати самостійно.

## Завдання

1. Зберіть (`make` у каталозі `lab3`) і запустіть `racy` на 1, 2, 4 і 8 потоках, кожен по кілька разів:

   ```sh
   for t in 1 2 4 8; do ./part1-race/counter racy $t 1000000; done
   ```

   Чи однакові результати між запусками? Скільки оновлень змінної втрачено?

2. Запустіть ту саму програму, але використовуючи лише 1 ядро процесору:

   ```sh
   taskset -c 0 ./part1-race/counter racy 4 1000000
   taskset -c 0 ./part1-race/counter racy 4 100000000
   ```

   Порівняйте кількість втрат з кроком 1. Як ви думаєте, чому так?

3. Запустіть версію з TSan:

   ```sh
   ./part1-race/counter-tsan racy 2 1000
   ```

   Розберіть попередження TSan. Подивіться також, що надрукувала програма. Що це означає?

4. Заповніть **TODO 1** (`increment_locked`) і **TODO 2** (`increment_local`). Перевірте, що результат правильний і TSan мовчить:

   ```sh
   make
   ./part1-race/counter-tsan locked 4 100000
   ./part1-race/counter-tsan local 4 100000
   ```

5. Виміряйте час трьох режимів на 1, 2 і 4 потоках по 10 000 000 інкрементів, а також `locked` на одному ядрі:

   ```sh
   for m in racy locked local; do for t in 1 2 4; do ./part1-race/counter $m $t 10000000; done; done
   taskset -c 0 ./part1-race/counter locked 4 10000000
   ```

   Порівняйте `locked` з 4 потоками на кількох ядрах і ті самі 4 потоки на одному ядрі: що швидше і чому? Як від кількості потоків залежить час `local`?

6. Зберіть `racy` з увімкненими оптимізаціями компілятору (-O2) і запустіть кілька разів:

   ```sh
   gcc -O2 -pthread part1-race/counter.c -o /tmp/counter-o2
   /tmp/counter-o2 racy 4 1000000
   ```

   Порівняйте кількість втрат з кроком 1. З `-O2` компілятор замінив увесь цикл одним `counter += iterations`. Поясніть, як це вплинуло на втрати і чи є в цій програмі data race (це можна перевірити, скомпілювавши програму з TSan):

   ```sh
   gcc -O2 -pthread -fsanitize=thread part1-race/counter.c -o /tmp/counter-o2-tsan
   /tmp/counter-o2-tsan racy 4 1000000
   ```

## Запитання для самоперевірки

- З яких операцій складається `counter++`? За якого порядку виконання цих операцій двома потоками один інкремент втрачається?
- Чим конкурентність відрізняється від паралелізму
- Як може втратитися інкремент, якщо всі потоки працюють на одному ядрі?
- Кому належать регістри — потоку чи ядру процесора? Де лежить прочитане значення `counter`, поки потік витіснений між читанням і записом?
- Чому `local` у `increment_local` не потребує м'ютекса, а `counter` потребує?
- Що робить потік, коли м'ютекс зайнятий іншим? Що у ваших вимірах відрізняє `locked` на кількох ядрах від `locked` на одному і чим це пояснюється?
- Як змінюється час `racy` з кількістю потоків, хоча м'ютекса там немає? Яка з причин сповільнення `locked` пояснює і це?
