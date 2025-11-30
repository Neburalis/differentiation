# Differentiation

Проект для разбора символьных выражений, хранения их в виде двоичных деревьев и генерации наглядных представлений (Graphviz, HTML-лог, LaTeX).

## Что делает система
- Читает выражения из текстового файла в формате s-expression: `(token left right)`, где `nil` обозначает отсутствующего ребенка.
- Строит дерево узлов `NODE_T`, которые бывают числовыми, операторными и именными (переменные).
- Поддерживает арифметику, степени, логарифмы, тригонометрические и гиперболические функции, перечисленные в `OPERATOR`.
- Ведет реестр уникальных переменных (`varlist::VarList`) и возвращает индексы при повторном использовании имен.
- Создает детализированные дампы: полный (`full_dump`) с внутренними указателями и компактный (`simple_dump`) только по значениям.
- Формирует LaTeX-строку (`latex_dump`), сохраняя корректные приоритеты и расстановку скобок.

## Структура проекта
```
.
├── README.md
├── a.out
├── main.cpp
├── logs/
│   ├── log.html
│   ├── tree_dump_0.svg
│   └── ...
├── expr
│   ├── test.tmp
│   ├── test2.txt
│   └── test_expression.txt
└── src
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

- `src/base.h` — набор утилитных макросов, аллокаторы и проверки.
- `src/differentiator.h` — публичные типы (`NODE_T`, `EQ_TREE_T`, перечисления) и прототипы функций.
- `src/parser.cpp` — парсер s-expression, точка входа `load_tree_from_file`, управление ошибками и привязка переменных.
- `src/tree.cpp` — создание/уничтожение узлов, утилиты определения типов токенов и операторов.
- `src/dump.cpp` — генерация DOT-файлов, HTML-записей лога и LaTeX-представления выражения.
- `src/logger.h` — интерфейс логгера: инициализация каталога, получение текущего файла и освобождение ресурсов.
- `src/simplify.cpp` — упрощение дерева: свёртка констант, нейтральные элементы, пересчёт размеров поддеревьев.
- `src/var_list.*` — список переменных, обеспечивающий уникальность и быстрый поиск по именам.
- `logs/` — HTML- и SVG-дампы, которые появляются после запуска `a.out`.
- `main.cpp` — пример: читает `test_expression.txt`, вызывает дампы, печатает LaTeX и корректно освобождает ресурсы.

## Грамматика
Правила текущий РБНФ-грамматики:
```
Grammar         ::= Expression'\0'
Expression      ::= Term{['+-']Term}*
Term            ::= Power{['*/']Power}*
Power           ::= Primary['^'Power]?
Primary         ::= '('Expression')' | Number | Variable | UnaryCall | BinaryCall
UnaryCall       ::= UnaryFunc '('Expression')'
BinaryCall      ::= BinaryFunc '('Expression','Expression')'
Variable        ::= ['a'-'z''A'-'Z']['a'-'z''A'-'Z''0'-'9' '_']*
Number          ::= '-'?['0'-'9']+
BinaryFunc      ::= 'log'
UnaryFunc       ::= 'ln' | 'sin' | 'cos' | 'tan' | 'tg' | 'ctg' | 'cot' | 'arcsin' | 'arccos'
                  | 'arctan' | 'arctg' | 'arccot' | 'arcctg' | 'sqrt' | 'sinh' | 'sh'
                  | 'cosh' | 'ch' | 'tanh' | 'th' | 'cth'
```

## Требуемая раскладка каталогов
```
./
├── differentiation/          (этот проект)
├── io_utils/                 ([Neburalis/io_utils](https://github.com/Neburalis/io_utils))
└── string_and_thong/         ([Neburalis/string_and_thong](https://github.com/Neburalis/string_and_thong))
```

## Сборка
Используйте мою утилиту [`g+++`](https://github.com/Neburalis/gppp) — она читает `.gppp.cfg` и автоматически применяет оптимальные флаги компилятора.
```bash
g+++        # создаст исполняемый файл a.out
```
Результатом будет `a.out` в текущем каталоге.

## Запуск
```bash
./a.out
```
В результате в каталоге `logs/` появятся HTML и SVG-дампы дерева, а LaTeX-представление будет выведено в stdout.

