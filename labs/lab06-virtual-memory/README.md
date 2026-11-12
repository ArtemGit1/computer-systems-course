# Lab 06: Віртуальна пам'ять, file descriptors та I/O

## Мета роботи

Дослідити virtual address space процесу, memory mappings і pages, а також взаємодію процесу з файлами через file descriptors та POSIX I/O. На прикладах перевірити `open()`, `read()`, `write()`, `close()`, file offset, buffering і копіювання файлів.

## Середовище та компіляція

Роботу виконано в Linux x86-64 у GitHub Codespaces з GCC 13.3, `strace` 6.8 і page size 4096 bytes. Для компіляції прикладів:

```sh
cd labs/lab06-virtual-memory
for source in *.c; do
    gcc -Wall -Wextra "$source" -o "${source%.c}"
done
```

Усі приклади скомпільовано без попереджень. `invalid_memory` навмисно завершується з `SIGSEGV`. Виконувані файли та тестові дані є тимчасовими й не входять до лабораторної.

## Частина I. Virtual Memory

### Адреси об'єктів програми

`memory_layout.c` виводить адреси функції, глобальних і static-змінних, heap allocation та локальної змінної. Три послідовні запуски дали:

| Об'єкт | Run 1 | Run 2 | Run 3 |
| :--- | :--- | :--- | :--- |
| function | `0x5a17669891c9` | `0x5f29038201c9` | `0x5ccf980ca1c9` |
| initialized global | `0x5a176698c010` | `0x5f2903823010` | `0x5ccf980cd010` |
| uninitialized global | `0x5a176698c01c` | `0x5f290382301c` | `0x5ccf980cd01c` |
| static | `0x5a176698c014` | `0x5f2903823014` | `0x5ccf980cd014` |
| heap allocation | `0x5a179282b2a0` | `0x5f29095692a0` | `0x5ccfabc842a0` |
| local variable | `0x7ffe784417cc` | `0x7ffdd170625c` | `0x7fffd91257fc` |

У кожному запуску код і глобальні дані розташовані близько один до одного, heap і stack — в інших областях. Адреси між запусками відрізняються через ASLR. Надруковані адреси є virtual addresses цього процесу, а не physical addresses. Точний порядок областей залежить від системи та executable.

### Memory map і permissions

`memory_maps.c` виводить PID та адреси, а потім чекає 60 секунд. Поки програма працює, її map можна переглянути в іншому терміналі:

```sh
cat /proc/<PID>/maps
```

У перевірочному запуску адреси програми зіставилися з mappings так:

```text
63f6bbda7000-63f6bbda8000 r-xp ... /tmp/lab06-check/memory_maps
63f6bbdaa000-63f6bbdab000 rw-p ... /tmp/lab06-check/memory_maps
63f6d80f0000-63f6d8111000 rw-p ... [heap]
7ffe123c2000-7ffe123e4000 rw-p ... [stack]
```

Адреси `main`, `global`, heap allocation і local variable потрапили відповідно до executable, writable data, `[heap]` і `[stack]`. У permissions `r`, `w`, `x` означають читання, запис і виконання. `p` — private mapping. Розділення permissions захищає пам'ять: code не потрібно дозволяти змінювати, а data не потрібно дозволяти виконувати.

### Page size і heap

`getconf PAGE_SIZE` та `page_size.c` показали **4096 bytes**. Для прикладу, адреса `0x5a176698c010` має:

```text
Virtual page number = address / 4096 = 0x5a176698c
Offset              = address % 4096 = 0x10
```

`heap_growth.c` виділяє 100 MiB, чекає Enter, записує по одному byte на кожній page і знову чекає Enter. В одному вимірюванні `/proc/<PID>/status`:

| Значення | До запису по pages | Після |
| :--- | ---: | ---: |
| `VmSize` | 105088 kB | 105088 kB |
| `VmRSS` | 1704 kB | 104100 kB |
| `VmData` | 102628 kB | 102628 kB |

У цьому запуску virtual allocation збільшив `VmSize`, але більша частина пам'яті стала resident після запису в pages. Точні значення залежать від allocator та ОС.

### Invalid memory access

`invalid_memory.c` записує через `NULL` pointer. Програма надрукувала повідомлення перед доступом і завершилася з `SIGSEGV` (status 139). `strace` показав:

```text
SIGSEGV {si_signo=SIGSEGV, si_code=SEGV_MAPERR, si_addr=NULL}
```

Це invalid virtual address access, який виявляють hardware memory protection та ОС. Це не означає, що physical RAM несправна, і не дозволяє процесу довільно перезаписати пам'ять іншого процесу.

## Частина II. File descriptors та I/O

### Standard descriptors і відкритий файл

`standard_fds.c` вивів:

```text
STDIN  = 0
STDOUT = 1
STDERR = 2
```

Під час перевірки через `/proc/<PID>/fd` descriptors 0, 1 і 2 були pipes, оскільки процес запускався з перенаправленим вводом/виводом. Тому їхні конкретні resources залежать від способу запуску.

`open_file.c` відкрив `example.txt` і повернув descriptor `3`. У цьому процесі `0`, `1` і `2` вже були зайняті, тому `open()` використав наступний вільний descriptor. Програма не повинна покладатися, що це завжди буде саме `3`. Pathname передається до `open()`, а отриманий descriptor використовують подальші I/O виклики.

### `write()`, `read()` і file offset

`write_file.c` створив `output.txt` із вмістом `Hello from write()` і newline: запитано та записано 19 bytes. `read_file.c` прочитав `input.txt` блоками по 16 bytes і вивів його вміст. `read()` повертає кількість bytes, `0` на EOF або `-1` у разі помилки. Успішний `write()` може записати менше за запит, тому приклади читання та копіювання обробляють partial writes.

Для перевірок створювали файл:

```sh
printf 'Computer Systems\nOperating Systems\n' > input.txt
```

`file_offset.c` прочитав перші 5 bytes, наступні 5 bytes, а тоді виконав `lseek(fd, 0, SEEK_SET)` і прочитав початок повторно:

```text
First [Compu]
Second [ter S]
After lseek [Compu]
```

Отже, кожен `read()` просуває поточний file offset, `lseek()` змінює позицію для подальшого послідовного або довільного доступу.

Фрагмент `strace -e trace=openat,read,write,close ./read_file`:

```text
openat(AT_FDCWD, "input.txt", O_RDONLY) = 3
read(3, "Computer Systems", 16) = 16
write(1, "Computer Systems", 16) = 16
read(3, "", 16) = 0
close(3) = 0
```

### stdout, stderr і buffering

`streams.c` записує `Normal output` у descriptor 1 та `Error output` у descriptor 2. Після `./streams > normal.txt` перший текст потрапив у `normal.txt`, а другий залишився на stderr. Shell змінив resource за descriptor 1, програмі не потрібен окремий код для перенаправлення.

`buffering.c` демонструє різницю між stdio buffering (`printf`) і безпосереднім `write()`:

| Запуск | Вивід | Порядок `write()` у `strace -e write` |
| :--- | :--- | :--- |
| `./buffering` | `DABC` | `write(1, "D", 1)`, потім `write(1, "ABC", 3)` |
| `./buffering --flush` | `ABCD` | `write(1, "ABC", 3)`, потім `write(1, "D", 1)` |

Без `fflush(stdout)` символи `A`, `B`, `C` залишаються у stdio buffer до завершення програми, `fflush()` перед `write()` відправляє їх раніше.

### Копіювання файла

`file_copy.c` копіює bytes через `open()`, `read()`, `write()` і `close()` — без stdio-функцій для самого копіювання:

```sh
./file_copy source.txt destination.txt
cmp source.txt destination.txt
```

Текстовий файл і executable було скопійовано, обидві перевірки `cmp` завершилися успішно. Однаковий цикл працює для обох, бо оперує bytes, а не текстовим поданням. Фрагмент `strace`:

```text
openat(AT_FDCWD, "input.txt", O_RDONLY) = 3
openat(AT_FDCWD, "trace-copy.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 4
read(3, ..., 4096) = 35
write(4, ..., 35) = 35
read(3, "", 4096) = 0
close(3) = 0
close(4) = 0
```

Data path для кожного блоку: source file → `read()` через kernel → buffer у virtual memory процесу → `write()` через kernel → destination file. Під час системного виклику descriptor визначає відкритий ресурс, а переданий buffer є адресою в address space процесу, MMU та page tables забезпечують трансляцію адрес і memory protection.

## Контрольні питання

1. **Що таке virtual address?** Адреса, яку процес використовує у своєму virtual address space.
2. **Чим вона відрізняється від physical address?** Virtual address транслюється у physical address апаратурою за таблицями ОС, процес безпосередньо не задає адресу RAM.
3. **Що таке virtual address space?** Віртуальний діапазон адрес, доступний процесу відповідно до mappings і permissions.
4. **Чому два процеси можуть мати однакову virtual address?** Їхні таблиці сторінок можуть відображати цю адресу на різні physical frames.
5. **Що таке page?** Фіксований блок virtual memory, який ОС відображає та захищає як одиницю.
6. **Що таке page frame?** Блок фізичної пам'яті, у який може бути відображена page.
7. **Що таке page table?** Структура відображень virtual pages на physical frames і пов'язаних permissions.
8. **Яку роль виконує MMU?** Транслює адреси та перевіряє права доступу під час memory access.
9. **Що таке TLB?** Кеш нещодавніх трансляцій адрес.
10. **Чим TLB miss відрізняється від page fault?** TLB miss вимагає знайти трансляцію в таблицях а page fault виникає, коли потрібне відображення чи доступ не може бути задоволений без втручання ОС.
11. **Що таке page fault?** Виняток через відсутнє або заборонене відображення сторінки. ОС обробляє його відповідно до причини.
12. **Чи page fault завжди означає помилку програми?** Ні, ОС може завантажити сторінку або створити її, заборонений доступ може завершитися сигналом.
13. **Як virtual memory ізолює процеси?** Кожен процес має власні mappings і permissions, які перевіряються під час доступу.
14. **Що означають `r`, `w`, `x` у mappings?** Дозволи на читання, запис і виконання.
15. **Чому code не обов'язково має бути writable?** Код зазвичай лише виконується та читається. Заборона запису знижує ризик його випадкової чи несанкціонованої зміни.
16. **Чому `malloc()` не означає негайне використання стільки ж physical RAM?** Allocator резервує virtual memory, фізичні сторінки можуть надаватися або ставати resident пізніше, зокрема після доступу до них.
17. **Що таке file descriptor?** Невелике ціле число, яким процес позначає відкритий I/O resource.
18. **Чим pathname відрізняється від descriptor?** Pathname називає ресурс для відкриття, descriptor — handle, повернений `open()` для подальших операцій.
19. **Що зазвичай представляють 0, 1 і 2?** `stdin`, `stdout` і `stderr`.
20. **Що робить `open()`?** Відкриває resource і повертає доступний descriptor або `-1` у разі помилки.
21. **Що повертає `read()`?** Кількість прочитаних bytes, `0` на EOF або `-1` у разі помилки.
22. **Що означає `read() == 0`?** Досягнуто кінця файла для поточної позиції читання.
23. **Що повертає `write()`?** Кількість записаних bytes або `-1` у разі помилки.
24. **Чому треба враховувати partial writes?** Один успішний виклик може записати лише частину переданого buffer, решту треба дозаписати.
25. **Що таке file offset?** Поточна позиція читання або запису у відкритому файлі.
26. **Для чого потрібен `lseek()`?** Для зміни file offset у файлах, що підтримують позиційний доступ.
27. **Чим `FILE *` відрізняється від descriptor?** `FILE *` — stdio stream із власним buffering, descriptor — низькорівневий POSIX handle.
28. **Чому `printf()` не обов'язково одразу викликає `write()`?** Текст може залишатися у stdio buffer до його заповнення, newline у line-buffered режимі або flush.
29. **Що таке buffering?** Тимчасове накопичення даних перед передаванням до наступного I/O рівня.
30. **Що робить `fflush()`?** Передає накопичені вихідні дані stdio stream далі через його I/O layer.
31. **Чому redirection працює без зміни програми?** Shell підміняє ресурс, на який вказує descriptor, перед запуском програми.
32. **Чому `file_copy` копіює text і binary однаково?** Вона читає та записує bytes без текстового перетворення.
33. **Де знаходиться buffer під час `read()`?** У virtual memory процесу. MMU відображає потрібні сторінки на physical memory.
34. **Як взаємодіють descriptor, virtual memory і system call під час I/O?** Системний виклик передає kernel descriptor і адресу buffer, kernel визначає відкритий ресурс, виконує I/O та копіює дані між resource і buffer процесу.

## Короткий висновок

У лабораторній роботі досліджено virtual address space, mappings і вплив доступу до pages на resident memory, а також перевірено ізоляцію через memory protection. I/O експерименти показали роль file descriptors, offset і buffering, `file_copy` поєднала роботу з virtual-memory buffer та системними викликами читання й запису.
