# Учебный проект
# Поисковый сервер
Обрабатывает и хранит строки, выполняет поиск в строках по словам возвращает список вариантов отсортированный по релевантности в порядке от большего к меньшему.

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
