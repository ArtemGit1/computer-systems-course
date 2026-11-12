# Lab 05: Потоки, race condition та mutex

## Мета роботи

Метою лабораторної роботи було вивчити модель потоків, зрозуміти різницю між process та thread, дослідити shared memory між потоками, з'ясувати причини race condition на прикладі `counter++` і показати, як `pthread_mutex_t` усуває некоректні інтервали.
Робота була виконана в Linux-середовищі GitHub Codespaces з використанням GCC 13.x та опції `-pthread`. Для компіляції використовувався стандартний цикл:

```sh
cd labs/lab05-threads
for source in *.c; do
    gcc -Wall -Wextra -pthread "$source" -o "${source%.c}"
done
```

Всі приклади були відтворені локально в цьому середовищі.

## 1. Перший потік

Файл `first_thread.c` демонструє базову схему `pthread_create()` і `pthread_join()`.

```c
void *worker(void *arg)
{
    (void)arg;
    printf("Hello from worker thread!\n");
    return NULL;
}
```

Компіляція та запуск:

```sh
gcc -Wall -Wextra -pthread first_thread.c -o first_thread
./first_thread
```

Результат виконання:

```text
Main: creating thread
Hello from worker thread!
Main: worker finished
```

### Висновки

- Одна програма має один initial process.
- Після `pthread_create()` в процесі з'являється новий thread.
- `pthread_join()` гарантує, що main thread дочекається завершення worker.
- Після успішного `pthread_create()` існує 2 threads в одному process.
- Новий thread виконує функцію `worker()`.
- `pthread_create()` не створює нового address space; thread працює в тому ж адресному просторі, що й parent.

## 2. Декілька threads

Файл `multiple_threads.c` запускає 4 worker threads. Програма була запущена 5 разів, щоб продемонструвати nondeterministic scheduling.

```text
Run 1
Thread 1 is running
Thread 2 is running
Thread 3 is running
Thread 4 is running
All threads finished

Run 2
Thread 3 is running
Thread 4 is running
Thread 1 is running
Thread 2 is running
All threads finished

Run 3
Thread 1 is running
Thread 2 is running
Thread 3 is running
Thread 4 is running
All threads finished

Run 4
Thread 1 is running
Thread 3 is running
Thread 4 is running
Thread 2 is running
All threads finished

Run 5
Thread 1 is running
Thread 2 is running
Thread 3 is running
Thread 4 is running
All threads finished
```

### Відповіді на запитання

- Порядок виконання не гарантується; `pthread_create()` не гарантує, що thread 1 завершиться першим.
- Scheduler Linux може перемикати потоки у довільному порядку.
- Операції виконуються на CPU у рамках одного process, але порядок запуску визначається планувальником.

## 3. Shared global variable

Файл `shared_data.c` демонструє, що глобальна змінна `value` є shared між усіма threads одного process.

```text
Thread 1: value=100, address=0x5ccb15c65014
Thread 2: value=200, address=0x5ccb15c65014
Main: value=200, address=0x5ccb15c65014
```

### Аналіз

- Усі три виводи показали одну й ту саму адресу `0x5ccb15c65014`.
- Це означає, що `value` лежить у shared memory відповідної області process.
- Threads одного process мають спільний address space.
- Фінальне значення залежить від порядку виконання, тому що `value` змінюється конкурентно.
- Для процесів у Lab 4 кожен process мав окремий address space; для threads — спільний.

## 4. Чи мають threads власний stack?

Файл `thread_stacks.c` порівнює локальні змінні `local` для двох threads.

```text
Thread 1: local=10, address=0x758e4282ceb0
Thread 2: local=20, address=0x758e4202beb0
```

### Висновок

- Глобальна змінна `value` була спільною.
- Локальна змінна `local` була розташована в стеку кожного thread окремо.
- Тому адреси відрізняються.
- Стек є thread-local; heap і дані процесу є shared.

Схема була такою:

```text
Process
│
├── Code       ← shared
├── Data       ← shared
├── Heap       ← shared
│
├── Thread 1
│     └── Stack 1
│
└── Thread 2
      └── Stack 2
```

## 5. Race condition

Файл `race.c` виконує `counter++` у двох threads. Очікуване значення для `2 * 1000000` дорівнює `2000000`.

```text
Run 1
Expected: 2000000
Actual:   1724285

Run 2
Expected: 2000000
Actual:   1452902

Run 3
Expected: 2000000
Actual:   1739419

Run 4
Expected: 2000000
Actual:   1643161

Run 5
Expected: 2000000
Actual:   1887782
```

### Чому `counter++` створює проблему?

Теоретично `counter++` еквівалентно трьом операціям:

```text
READ counter
ADD 1
WRITE counter
```

Приклад interleaving:

```text
Thread A      Thread B
READ 100      READ 100
ADD -> 101    ADD -> 101
WRITE 101     WRITE 101
```

Обидва потоки прочитали те саме значення, обидва додали 1, і результат був 101 замість 102.

Таке поведінка виникає через race condition: декілька threads одночасно змінюють shared state без синхронізації.

### Machine instructions та суть проблеми

Коли у C є вираз `counter++`, компілятор перетворює його на послідовність машинних інструкцій, а не в одну атомарну операцію. У `race.c` це означає, що між `read` і `write` може статися перемикання потоків. Тому `counter++` не атомарний на рівні виконання.

### Визначення

- Shared state: `counter`
- Concurrent writers: `thread1`, `thread2`
- Critical operation: `counter++`

## 6. Mutex як виправлення

Файл `mutex_counter.c` захищає інкремент м'ютексом.

Результати п'яти запусків:

```text
Run 1
Expected: 2000000
Actual:   2000000

Run 2
Expected: 2000000
Actual:   2000000

Run 3
Expected: 2000000
Actual:   2000000

Run 4
Expected: 2000000
Actual:   2000000

Run 5
Expected: 2000000
Actual:   2000000
```

### Чому mutex змінив поведінку?

М'ютекс забезпечує mutual exclusion:

```text
Thread A        Thread B
  LOCK           LOCK (waits)
  counter++
  UNLOCK
```

Тільки один thread одночасно може перебувати в критичній секції. Це дозволяє виконати `READ → MODIFY → WRITE` без interleaving іншого thread. У результаті немає `lost update`.

## 7. Де саме повинен бути lock?

Для correctness достатньо, щоб `counter++` був усередині критичної секції:

```c
for (...) {
    pthread_mutex_lock(&mutex);
    counter++;
    pthread_mutex_unlock(&mutex);
}
```

Альтернативний варіант:

```c
pthread_mutex_lock(&mutex);
for (...) {
    counter++;
}
pthread_mutex_unlock(&mutex);
```

Це теж виправляє помилку, але зменшує паралелізм: один потік тримає mutex під час всієї циклічної роботи, а інший поток блокуватиметься довше. Критична секція має бути достатньо короткою, але не занадто малою.

## 8. Synchronization має вартість

Порівнювальний запуск в нашому середовищі показав:

```text
race.c -> incorrect, but often faster
mutex_counter.c -> correct, with synchronization overhead
```

Чому? Тому що локація mutex додає cost на lock/unlock і може викликати contention між потоками. У правильній програмі потрібно знайти баланс між correctness і продуктивністю.

## 9. Більше threads: 2 vs 4

Додаткові експерименти з 4 потоками:

```text
--- race_four ---
Expected: 2000000
Actual:   1081085

--- mutex_four ---
Expected: 2000000
Actual:   2000000
```

Це показує, що "більше threads" не означає автоматично "швидше". Чим більше потоків, тим більше шансів на interleaving і contention. Без синхронізації помилка зростає, а при м'ютексі додаткові накладні витрати теж зростають.

## 10. Shared heap memory

Файл `shared_heap.c` перевіряє, що `data` знаходиться в heap, а не на stack.

```text
Worker: value=110 address=0x55e7c95b11f0
Main: value=110 address=0x55e7c95b11f0
```

### Висновки

- `data` виділено через `malloc()`, тобто в heap.
- Worker і main бачать один і той самий об'єкт.
- Якщо декілька потоків одночасно роблять `data->value += 10`, результат буде надто залежати від interleaving і також потребуватиме mutex.

## 11. Thread return value

Файл `thread_result.c` повертає результат через `malloc()`, а не через локальний стек потоку.

```c
int *result = malloc(sizeof(*result));
*result = value * value;
return result;
```

Якщо зробити так:

```c
int result = value * value;
return &result;
```

це некоректно, бо `result` є локальною змінною потоку і знищується після завершення функції. Пам'ять на stack не є безпечною для повернення за межі потоку.

## 12. Deadlock

Файл `deadlock.c` демонструє класичну схему deadlock:

```text
Thread 1: locking mutex1
Thread 2: locking mutex2
Thread 1: locking mutex2
Thread 2: locking mutex1
```

Після запуску програма зависала. У нашому перевірочному запуску:

```sh
timeout 3 ./deadlock
```

виконався з кодом завершення `124`: програма не завершилася в межах таймауту.

### Чому це deadlock?

```text
Thread 1 owns mutex1, waits for mutex2
Thread 2 owns mutex2, waits for mutex1
```

Маємо circular wait:

```text
Thread 1 → mutex2 → Thread 2 → mutex1 → Thread 1
```

Це deadlock: програма перестає робити прогрес.

### Виправлення

У другому потоці необхідно брати locks у тому ж порядку, наприклад:

```c
pthread_mutex_lock(&mutex1);
pthread_mutex_lock(&mutex2);
```

Після цього усувається circular wait. Це ключова стратегія безпеки від deadlock: один порядок захоплення locks для всіх потоків.

## 13. Race condition ≠ deadlock

| Характеристика | Race condition | Deadlock |
| :--- | :--- | :--- |
| Threads продовжують execution? | Так, але результат може бути неправильним | Ні, програма зупиняється |
| Результат неправильний? | Так, через lost update | Не обов'язково, а програма стоїть на місці |
| Прогрес зупинено? | Ні | Так |
| Пов'язано з timing/order? | Так | Так |
| Mutex може допомогти? | Так | Так, якщо використовувати правильно |
| Неправильний mutex може бути причиною? | Ні | Так, якщо створюється circular wait |

Ключова ідея: без синхронізації — race condition, з поганою синхронізацією — deadlock.

## 14. Parallel sum

Файл `parallel_sum.c` виконує суму `1 + 2 + ... + N` у 4 потоках.

```text
Thread 1: 31250125000
Thread 2: 93750125000
Thread 3: 156250125000
Thread 4: 218750125000
Parallel result: 500000500000
Expected result: 500000500000
```

Формула перевірки:

```text
N * (N + 1)
──────────
     2
```

Для `N = 1000000` результат рівний `500000500000`.

### Чому дизайн краще за один global counter?

Кожен thread обраховує свій `local sum` і повертає результат у `task->result`. Тільки у `main` відбувається підсумовування partial results після `pthread_join()`. Таким чином:

- shared mutable state мінімізовано;
- mutex потрібен лише для завершення main після join, а не для кожної ітерації;
- перекладається менша кількість synchronization на критичні секції;
- зменшується contention;
- кожен thread має локальний accumulator.

Це хороший architectural lesson: іноді краще не додавати більше locks, а зменшувати спільну змінну state.

## 15. Додатковий experiment: sequential vs parallel

Файл `sequential_sum.c` перевіряє послідовне обчислення:

```text
Sequential result: 500000500000
Expected result: 500000500000
```

Паралельна версія дає той самий результат, але для маленьких задач часто не буде швидшою через накладні витрати на thread creation, scheduling і синхронізацію. Parallelization має сенс, коли задача велика, а всі підзадачі більш-менш незалежні.

## 16. Контрольні питання

1. Що таке thread?  
   Це окремий потік виконання в межах одного process. У потоку є свій stack і instruction pointer, але є спільний код, heap, data і address space.

2. Чим thread відрізняється від process?  
   Process має окремий address space, а thread працює в тому ж address space, що й інші threads процесу.

3. Що threads одного process поділяють?  
   Код, data, heap, open files, process-wide state.

4. Що кожен thread має власне?  
   Stack, регістри, instruction pointer, thread-local data, execution context.

5. Чи має кожен thread власний stack?  
   Так.

6. Чи мають threads спільний heap?  
   Так.

7. Для чого використовується `pthread_create()`?  
   Для створення нового потоку.

8. Для чого використовується `pthread_join()`?  
   Для очікування завершення потоку.

9. Чи гарантується порядок виконання threads за порядком `pthread_create()`?  
   Ні.

10. Чим concurrency відрізняється від parallelism?  
    Concurrency — логічне перекривання виконання; parallelism — реальне одночасне виконання на кількох ядрах/потоках.

11. Що таке shared state?  
    Змінна або об'єкт, до якого мають доступ кілька потоків.

12. Що таке race condition?  
    Коли результат залежить від timing і interleaving виконання потоків.

13. Що таке lost update?  
    Коли два потоки читають одне й те саме значення, змінюють його незалежно і записують назад, втрачаючи одну з модифікацій.

14. Чому `counter++` не слід вважати atomic?  
    Для нього потрібні кілька машинних операцій: read, add, write, а між ними можливе перемикання потоків.

15. Що таке critical section?  
    Частина коду, що працює зі спільними даними і повина виконуватися не більше одного потоку одночасно.

16. Що таке mutual exclusion?  
    Умови, коли лише один потік може входити в критичну секцію.

17. Що таке mutex?  
    Механізм синхронізації для захисту критичних секцій.

18. Що робить `pthread_mutex_lock()`?  
    Захоплює mutex; якщо він вже заблокований іншим потоком, поточний потік чекає.

19. Що відбувається, якщо mutex уже належить іншому потоку?  
    Поток блокується до того моменту, поки mutex не буде вивільнений.

20. Навіщо викликати `pthread_mutex_unlock()`?  
    Для зняття блокування і дозволення іншим потокам увійти.

21. Чому надто велика critical section може погіршити concurrency?  
    Тому що інші потоки довше чекають на mutex.

22. Що таке lock contention?  
    Конкуренція потоків за один і той самий mutex.

23. Чому більше threads не обов'язково означає вищу performance?  
    Через overhead на планування і contention за resources.

24. Що таке deadlock?  
    Стан, коли потоки заблоковані один на одного і не можуть продовжити виконання.

25. Чим deadlock відрізняється від race condition?  
    Race condition — неправильний результат; deadlock — повна зупинка програми.

26. Що таке circular wait?  
    Коли потік A чекає на ресурс, який утримує потік B, а B чекає на ресурс, який утримує A.

27. Як однаковий порядок lock acquisition допомагає уникнути deadlock?  
    Усуває circular wait.

28. Що таке starvation?  
    Стан, коли потік довго не отримує доступ до ресурсів через безперервну перевагу інших потоків.

29. Чому local accumulation у `parallel_sum` кращий за shared counter з mutex на кожній ітерації?  
    Тому що зменшується shared mutable state і synchronization overhead.

30. Чому 100 успішних запусків concurrent program не доводять відсутність race condition?  
    Через nondeterministic scheduling; помилка може проявитися не на кожному запуску.

## Короткий висновок

У лабораторній роботі досліджено спільний address space потоків і їхні окремі stacks, а також показано, що race condition у спільних даних можна усунути синхронізацією через mutex. Також розглянуто deadlock і накладні витрати паралельного виконання; коректність програми залежить від продуманого shared state та порядку захоплення locks.
