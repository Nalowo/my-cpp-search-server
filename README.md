# Учебный проект
# Поисковый сервер
Обрабатывает и хранит строки, выполняет поиск в строках по словам, возвращает список вариантов отсортированный по релевантности в порядке от большего к меньшему.

Инициализация класса поискового сервера:

```C++
SearchServer search_server("and with"s);
```

В аргументе к конструктору передаются слова, которые не будут учитываться при добавлении документов, как правило это слова не несущие конкретного смысла.

Строка в аргументе конструктору это набор стоп слов к

На вход подается:

```C++
search_server.AddDocument(id, text, DocumentStatus::ACTUAL, {1, 2});
```
id - идентификатор добавляемого документа, должен быть уникальным  
text - строка содержащая документ  
DocumentStatus::ACTUAL - статус документа  
{1, 2} - массив оценок пользователей  

Запрос к серверу:

```C++
search_server.FindTopDocuments("curly -nasty cat"s);
```

Аргумент  это строка с запросом слова из которой будут искаться в документах содержащихся в системе. Если слово начинается с "-", это означает что это "минус слово" и документы содержащие его не должны попадать в возвращаемый список.

Вывод:
{ document_id = 2, relevance = 0.866434, rating = 1 }
* document_id = 2 – id документа в системе
* relevance = 0.866434 – TF-IDF релевантность соответствия документа запросу, 
* rating = 1 – средний рейтинг оценки запроса пользователем

# Компилятор:
gcc version 12.2.0 (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders)

# Компиляция:
main.cpp – точка входа
unit_tests.cpp – юнит тесты

# Команда компиляции:
g++ -fdiagnostics-color=always maim.cpp request_queue.cpp search_server.cpp string_processing.cpp process_queries.cpp -o main.exe -std=c++2a -Werror -Wall
