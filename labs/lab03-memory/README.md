# Lab 03: Побудова програми, ELF та організація пам'яті

## Мета роботи

Дослідити шлях від C source code до ELF executable, роль preprocessor, compiler, assembler і linker, структуру ELF та відмінність між executable file і процесом. Експериментально зіставити змінні з областями `.text`, `.rodata`, `.data`, `.bss`, stack і heap.

## Середовище та відтворення

Робота виконувалась у Linux x86-64 з GCC. Для перевірки використовувалися `gcc`, `file`, `readelf`, `objdump`, `nm`, `ldd` і `size`. Точні адреси, розміри ELF, назви loader та вивід утиліт можуть відрізнятися на іншій системі.

Перейдіть у каталог лабораторної та перевірте інструменти:

```sh
cd labs/lab03-memory
gcc --version
readelf --version
objdump --version
```

## Завдання 1-5. Build pipeline

Базова програма зберігається у [program.c](program.c). Усі етапи можна відтворити окремо:

```sh
gcc -Wall -Wextra program.c -o program
./program

gcc -E program.c -o program.i
gcc -S -O0 -masm=intel program.i -o program.s
gcc -c program.s -o program.o
gcc program.o -o program
```

Результат запуску: `Result: 30`.

| Файл | Етап | Що містить | CPU виконує безпосередньо? |
| :--- | :--- | :--- | :--- |
| `.c` | Source | Код мовою C | Ні |
| `.i` | Preprocessing | C-код після розгортання директив і заголовків | Ні |
| `.s` | Compilation | Текстовий Assembly для цільової ISA | Ні |
| `.o` | Assembly | ELF relocatable object із machine code, даними та symbols | Ні, потрібне компонування/завантаження |
| executable | Linking | ELF executable, готовий для запуску ОС | Так, після завантаження ОС у пам'ять |

`program.i` значно більший за `program.c`, оскільки `#include <stdio.h>` замінюється вмістом заголовків і preprocessing обробляє директиви. Файл усе ще є C source, а не машинним кодом. `program.s` є текстом Assembly для x86-64; у ньому видно мітки `add` та `main`. `program.o` є бінарним ELF object. `objdump -d -M intel program.o` декодує його машинні байти назад у читабельне Assembly.

`nm program.o` показує визначені symbols `add`, `main`, `global_value`, `global_empty` і невизначений зовнішній symbol `printf` (`U`). Отже, реалізація `add` і `main` присутня в object file, а реалізацію `printf` треба знайти під час linking, зазвичай у C library.

## Завдання 6. Linker error

У [main.c](main.c) оголошено `calculate`, але його реалізацію відкладено в [calculate.c](calculate.c). Перевірка:

```sh
gcc -c main.c -o main.o
gcc main.o -o app
```

Перша команда успішна: компілятор бачить оголошення та може згенерувати виклик. Linking завершується повідомленням `undefined reference to 'calculate'`, бо жоден переданий object file не визначає цей symbol. Виправлення:

```sh
gcc -c calculate.c -o calculate.o
nm main.o
nm calculate.o
gcc main.o calculate.o -o app
./app
```

`nm main.o` показує `U calculate`, а `nm calculate.o` — визначений `T calculate`. Після додавання обох object files linker створює executable, що друкує `30`.

## Завдання 7-8. ELF executable та sections

Перевірка:

```sh
file program
readelf -h program
readelf -S program
```

У перевіреному середовищі отримано такі властивості:

| ELF property | Результат |
| :--- | :--- |
| Class | ELF64 |
| Data | 2's complement, little endian |
| Type | DYN (Position-Independent Executable, PIE) |
| Machine | Advanced Micro Devices X86-64 |
| Entry point | `0x1060` у цьому конкретному build |

`ELF64` означає 64-бітний формат ELF; поле `Machine` вказує на x86-64. Сучасний GCC у цьому середовищі за замовчуванням створює PIE executable, тому його ELF type — `DYN`. Це виконуваний PIE, а не звичайна shared library.

| Section | Вміст і приклади з цієї роботи |
| :--- | :--- |
| `.text` | Машинний код функцій `main`, `add` та інших функцій програми |
| `.rodata` | Read-only дані: `message` (`Computer Systems`), string literals і форматні рядки `printf` |
| `.data` | Ініціалізовані глобальні та static змінні: `global_value`, `initialized`, `static_initialized` |
| `.bss` | Неініціалізовані або нульові глобальні та static змінні: `global_empty`, `uninitialized`, `static_uninitialized`; також `large_array` у відповідному експерименті |

В `nm` типові літери `T`/`t` позначають код, `D`/`d` — ініціалізовані дані, `B`/`b` — BSS, `R`/`r` — read-only дані. Регістр літери відрізняє global від local symbols.

## Завдання 9-10. `.data`, `.bss` та розмір executable

У [sections.c](sections.c) символи `initialized` і `static_initialized` знаходяться в `.data`; `uninitialized` і `static_uninitialized` — у `.bss`; масив `message` — у `.rodata`. Це можна перевірити командами:

```sh
gcc -Wall -Wextra sections.c -o sections
nm sections
readelf -S sections
objdump -s -j .data sections
objdump -s -j .rodata sections
```

Для контрольованого порівняння масив увімкнено макросом `LARGE_ARRAY`, без редагування вихідника:

```sh
gcc -Wall -Wextra sections.c -o sections-small
gcc -Wall -Wextra -DLARGE_ARRAY sections.c -o sections-large
size sections-small sections-large
ls -lh sections-small sections-large
```

Заміри в середовищі роботи:

| Варіант | text | data | bss | Розмір файла |
| :--- | ---: | ---: | ---: | ---: |
| Звичайний | 1450 | 608 | 16 | 16144 bytes |
| З `large_array[100000]` | 1450 | 608 | 400072 | 16176 bytes |

Значення можуть трохи відрізнятися залежно від версій компілятора та linker. `int` у цьому середовищі займає 4 bytes, отже масив потребує 400000 bytes у пам'яті. `.bss` у ELF описується як область типу `NOBITS`: нульові байти не записуються в executable, а loader виділяє пам'ять і заповнює її нулями. Тому BSS зросла приблизно на 400 KB, а файл — лише на 32 bytes у цьому вимірюванні.

## Завдання 11. Dynamic libraries

```sh
ldd program
```

У цьому середовищі серед залежностей є `libc.so.6` та ELF loader `/lib64/ld-linux-x86-64.so.2`. Повна реалізація `printf` не копіюється у наш executable: dynamic linker знаходить відповідний symbol у shared library під час запуску. `.so` означає shared object. Спільні бібліотеки дають змогу багатьом програмам використовувати одну бібліотечну реалізацію, зменшуючи дублювання коду у виконуваних файлах і дозволяючи оновлювати бібліотеку окремо.

## Завдання 12. Executable та process

[process.c](process.c) виводить PID і чекає 60 секунд:

```sh
gcc -Wall -Wextra process.c -o process
./process
```

Поки програма чекає, в іншому terminal виконайте `ps -p PID`, підставивши надрукований PID. `process` до запуску — файл з ELF-кодом і даними на диску. Після запуску ОС створює процес: екземпляр виконання з PID, віртуальним address space, станом регістрів та іншими ресурсами. PID і адреси належать процесу, а не незмінному executable file.

## Завдання 13-16. Розміщення даних, stack і heap

У [memory.c](memory.c) та [memory_map.c](memory_map.c) передбачення й фактична класифікація такі:

| Об'єкт | Очікувана область | Спостереження |
| :--- | :--- | :--- |
| Функція `main` / `marker` | code | Адреси функцій близькі одна до одної в `.text` |
| `global_initialized` | data | Групується зі статичними даними |
| `global_uninitialized` | bss | Групується зі статичними даними, окремо від stack/heap |
| `static_initialized` | data | Зберігається разом з initialized global |
| `static_uninitialized` | bss | Зберігається разом з uninitialized global |
| String literal | rodata | Розташований у read-only ділянці поблизу коду й інших констант |
| `local` | stack | Адреса належить області stack процесу |
| Результат `malloc` | heap | Адреса належить dynamic allocation area, окремо від local |

Один з запусків `memory_map` показав, наприклад, code за адресою `0x5a880f5421a9`, global/static data біля `0x5a880f545010`, string literal за `0x5a880f543095`, heap за `0x5a8834d472a0` і local за `0x7ffc4854b24c`. Це лише приклад одного запуску, не сталі адреси. У двох запусках `memory` адреса `main` змінилася з `0x5fcb478221a9` на `0x596ac47cb1a9`; отже, абсолютні адреси рандомізуються (ASLR), але об'єкти в межах однієї області лишаються згрупованими.

Для stack-експерименту:

```sh
gcc -Wall -Wextra -O0 stack.c -o stack
./stack
```

У замірі адреси `&a`, `&b`, `&c` відповідно були `0x7fff6d9303c4`, `0x7fff6d9303a4`, `0x7fff6d930384`. При вкладених викликах адреси зменшувалися, що узгоджується з типовою моделлю stack для x86-64 у цьому середовищі. Напрямок не є універсальною властивістю всіх ISA та ABI.

Для heap-експерименту:

```sh
gcc -Wall -Wextra -O0 heap.c -o heap
./heap
```

Один запуск повернув `a = 0x6418a407a2a0`, `b = 0x6418a407a2c0`, `c = 0x6418a407a2e0`. Ці allocations випадково виявилися близькими й регулярно вирівняними; це не гарантує, що `malloc` завжди повертає послідовні адреси. Результат залежить від allocator і розміру запитів.

Концептуальна карта, від вищих адрес до нижчих для цього типового Linux процесу:

```text
Higher addresses
┌────────────────────────┐
│ Stack                  │ local, кадри викликів функцій
├────────────────────────┤
│ ...                    │ інші mapping-и процесу
├────────────────────────┤
│ Heap                   │ dynamic allocation від malloc
├────────────────────────┤
│ .bss                   │ global_empty, uninitialized
├────────────────────────┤
│ .data                  │ global_value, initialized
├────────────────────────┤
│ .rodata                │ message, string literals
├────────────────────────┤
│ .text                  │ main, add та інший код
└────────────────────────┘
Lower addresses
```

Це концептуальна схема, не точна карта всього address space. PIE, libraries, mmap та ASLR впливають на реальне розташування областей.

## Фінальний build pipeline

Для [memory_map.c](memory_map.c) повторіть повний pipeline та класифікуйте отримані файли:

```sh
gcc -E memory_map.c -o memory_map.i
gcc -S -O0 -masm=intel memory_map.i -o memory_map.s
gcc -c memory_map.s -o memory_map.o
gcc memory_map.o -o memory_map

file memory_map.c memory_map.i memory_map.s memory_map.o memory_map
```

## Контрольні питання

1. **Які основні етапи проходить C-програма до executable?** Preprocessing, compilation, assembling і linking; ОС завантажує executable для створення процесу.
2. **Що робить preprocessor?** Обробляє директиви, розгортає `#include` і macros та готує translation unit для компілятора.
3. **Що створює compiler?** Assembly-код для цільової ISA (або, залежно від параметрів, передає його наступному етапу).
4. **Що робить assembler?** Перетворює Assembly на object file з machine code, даними та таблицями symbols/relocations.
5. **Що містить object file?** Machine code і дані окремих секцій, symbols та інформацію для подальшого linking.
6. **Навіщо потрібен linker?** Об'єднує object files і бібліотеки, зіставляє зовнішні symbols та створює executable або library.
7. **Що означає `undefined reference`?** Під час linking не знайдено визначення symbol, на який посилається програма.
8. **Що таке symbol?** Іменована сутність object file, наприклад функція або змінна, з адресою/секцією чи зовнішнім посиланням.
9. **Чим header file відрізняється від library?** Header містить оголошення та макроси для компіляції; library містить скомпільовану реалізацію функцій і даних.
10. **Що таке ELF?** Executable and Linkable Format — формат object files, executable і shared libraries у Linux та інших системах.
11. **Що зазвичай містить `.text`?** Машинний код програми.
12. **Що зазвичай містить `.rodata`?** Константні дані, зокрема string literals та format strings.
13. **У чому різниця між `.data` та `.bss`?** `.data` зберігає initialized writable static data; `.bss` описує нульові або неініціалізовані static objects і зазвичай не зберігає їх нулі в executable.
14. **Чому велика `.bss` не додає такий самий розмір до executable?** Секція має тип `NOBITS`; loader виділяє пам'ять і обнуляє її під час завантаження.
15. **Що показує `ldd`?** Shared library dependencies executable, які dynamic linker має знайти.
16. **Чим executable відрізняється від process?** Executable — файл; process — запущений екземпляр програми з власним станом та address space.
17. **Де концептуально розташовуються local variables?** Зазвичай у stack frame, якщо компілятор не оптимізував їх у регістри чи інше місце.
18. **Для чого використовується heap?** Для dynamic allocation, коли розмір або час життя даних визначаються під час виконання; пам'яттю керують через allocator, наприклад `malloc`/`free`.
19. **Яка різниця між stack та heap?** Stack автоматично обслуговує кадри викликів із вкладеним часом життя; heap надає dynamic allocations, що існують до звільнення або завершення процесу.
20. **Чому адреси можуть змінюватися між запусками?** Через ASLR та інші рішення loader/allocator щодо розміщення областей.
21. **Що таке temporal і spatial locality?** Temporal locality — нещодавно використані дані й інструкції часто використовуються знову; spatial locality — після доступу до адреси часто використовуються близькі адреси.

## Короткий висновок

Під час лабораторної роботи розкладено build pipeline від C source до ELF executable та перевірено роль linker і symbols на прикладі відтворюваної помилки. За допомогою `readelf`, `nm`, `objdump`, `size` і `ldd` досліджено ELF sections, бібліотечні залежності та вплив великої BSS на розмір файла. Запуски програм показали відмінність між code, global/static data, stack і heap, а також те, що ASLR змінює абсолютні адреси між запусками.