# Differentiation

Символьный калькулятор: разбирает математические выражения, строит двоичные деревья, умеет упрощать, дифференцировать и визуализировать результат.

## Возможности
- Читает выражения из текстовых файлов в классической математической записи. Первая строка может задавать имя дерева, далее указывается само выражение.
- Поддерживает арифметику, степенной оператор, `log`, `ln`, весь набор тригонометрических и гиперболических функций из `OPERATOR`.
- Строит дерево `EQ_TREE_T` с узлами `NODE_T`, автоматически регистрирует уникальные переменные в `varlist::VarList`.
- Упрощает дерево (`simplify_tree`): свёртка констант, удаление нейтральных элементов, пересчёт размера поддеревьев.
- Выполняет символьное дифференцирование (`differentiate`) с генерацией нового дерева и аккуратной разметкой имени.
- Позволяет вычислить значение выражения в точке (`read_point_data` + `calc_in_point`).
- Формирует дампы Graphviz (полный `full_dump` и компактный `simple_dump`) и HTML-лог с MathJax-разметкой.
- Генерирует LaTeX-строку (`latex_dump`) с расстановкой скобок согласно приоритетам.

## Структура
```
.
├── README.md
├── main.cpp
├── expr/
│   ├── test?.tmp
│   └── ...
├── logs/
│   ├── log.html
│   └── tree_dump_*.svg
└── src/
    ├── base.h
    ├── differentiate.cpp
    ├── differentiator.h
    ├── dump.cpp
    ├── logger.cpp
    ├── logger.h
    ├── parser.cpp
    ├── simplify.cpp
    ├── tree.cpp
    ├── var_list.cpp
    └── var_list.h
```

- `parser.cpp` – рекурсивный спуск, загрузка выражения `load_tree_from_file`, регистрация переменных.
- `tree.cpp` – создание/уничтожение узлов, вычисление выражения, чтение точки.
- `simplify.cpp` – свёртка констант и нейтрализация операций.
- `differentiate.cpp` – символьные производные для всех доступных операторов.
- `dump.cpp` – генерация Graphviz и LaTeX, запись в HTML-лог.
- `logger.*` – минимальный HTML-логгер с поддержкой MathJax.
- `var_list.*` – хэшированный реестр уникальных имен переменных.
- `main.cpp` – пример пайплайна: загрузка, упрощение, дампы, дифференцирование, вычисление значений.


### Требуемая раскладка каталогов
```
./
├── differentiation/          (этот проект)
├── io_utils/                 ([Neburalis/io_utils](https://github.com/Neburalis/io_utils))
└── string_and_thong/         ([Neburalis/string_and_thong](https://github.com/Neburalis/string_and_thong))
```

## Формат входного выражения
- Файл может начинаться с имени дерева в первой строке. Пустая строка означает, что будет использовано имя по умолчанию.
- После имени указывается выражение, например:
  ```
  Test4
  (2^(a+x)-tg(x))/(arccos(2 * x^2))
  ```
- Грамматика парсера в РБНФ:
  ```
  Grammar         ::= Expression'\0'
  Expression      ::= Term{['+-']Term}*
  Term            ::= Power{['*/']Power}*
  Power           ::= Primary{'^'Primary}?
  Primary         ::= '('Expression')' | Number | Variable | UnaryCall | BinaryCall
  UnaryCall       ::= UnaryFunc '('Expression')'
  BinaryCall      ::= BinaryFunc '('Expression','Expression')'
  Variable        ::= ['a'-'z''A'-'Z']['a'-'z''A'-'Z''0'-'9' '_']*
  Number          ::= '-'?['0'-'9']+{'.'['0'-'9']+}?
  BinaryFunc      ::= 'log'
  UnaryFunc       ::= 'ln' | 'sin' | 'cos' | 'tan' | 'tg' | 'ctg' | 'cot' | 'arcsin' | 'arccos'
                    | 'arctan' | 'arctg' | 'arccot' | 'arcctg' | 'sqrt' | 'sinh' | 'sh'
                    | 'cosh' | 'ch' | 'tanh' | 'th' | 'cth'
  ```

## Быстрый старт
### Сборка
Рекомендую использовать мою утилиту [`g+++`](https://github.com/Neburalis/gppp) — она читает `.gppp.cfg` и автоматически применяет оптимальные флаги компилятора. Но можно вручную прописать пути к зависимостям в g++
```bash
g+++        # создаст исполняемый файл a.out
```
Результатом будет `a.out` в текущем каталоге.

### Запуск
```bash
./a.out
```
В результате в каталоге `logs/` появятся HTML и SVG-дампы дерева, а LaTeX-представление будет выведено в stdout.

