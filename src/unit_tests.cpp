#include "search_server.h"

#include <iostream>
#include <string>
#include <vector>

#include "process_queries.h"

using namespace std;

//###########################-Начало фреймворка для тестов-###################################################
template <typename Type>
ostream& operator<<(ostream& output, vector<Type> v) {
    bool spase = false;
    output << "["s;
    for (const auto& word : v) {
        if (spase) {
            output << ", "s;
        }
        output << word;
        spase = true;
    }
    output << "]"s;
    return output;
}

template <typename Type>
ostream& operator<<(ostream& output, set<Type> v) {
    bool spase = false;
    output << "{"s;
    for (const auto& word : v) {
        if (spase) {
            output << ", "s;
        }
        output << word;
        spase = true;
    }
    output << "}"s;
    return output;
}

template <typename Type, typename Value>
ostream& operator<<(ostream& output, map<Type, Value> v) {
    bool spase = false;
    output << "{"s;
    for (const auto& [key, value] : v) {
        if (spase) {
            output << ", "s;
        }
        output << key << ": "s << value;
        spase = true;
    }
    output << "}"s;
    return output;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename F>
void RunTestImpl(F& f, const string& str) {
    f();
    cerr << str << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

//###########################-Конец фреймворка для тестов-####################################################

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(set<string> {"dog"s, "car"s});
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    { // проверка шататной строки
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents <string>"s);
    }

    { // проверка строки с множестром пробелов
        SearchServer server("  in   the   "s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents <string>"s);
    }

    { // проверка пустой строки
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(!(server.FindTopDocuments("in"s).empty()),
                    "Stop <string> empty"s);
    }

     { // проверка строки с одним пробелом
        SearchServer server(" "s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(!(server.FindTopDocuments("in"s).empty()),
                    "Stop <string> one space"s);
    }


    {
        SearchServer server(set<string> {"in"s, "the"s});
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents <set>"s);
    }

    {
        SearchServer server(set<string> {});
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(!(server.FindTopDocuments("in"s).empty()),
                    "Stop <set> empty"s);
    }

    { // поиск ничего, ничего и не должно быть на выходе
        SearchServer server(set<string> {" "s});
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT((server.FindTopDocuments(" "s).empty()),
                    "Stop <set> empty"s);
    }

    { // проверка со стандартным вектором
        SearchServer server(vector<string> {"in"s, "the"s});
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("i    n "s).empty(),
                    "Stop words must be excluded from documents <vector>"s);
    }

    { // проверка с пустым вектором
        SearchServer server(vector<string> {});
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(!(server.FindTopDocuments("in"s).empty()),
                    "Stop<vector> empty"s);
    }
}

void TestMinusWordInput() { // проверка что не выводится документ в котором есть минус слова
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    { // документ в котором есть минус слова должен вернуть пустой вектор
        SearchServer server("in"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in -city"s);
        ASSERT(found_docs.empty());
    }
}

void TestMatchDocuments() { // проверка вывода запрашиваемых слов в документах
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {// нам должен вернуться кортеж где первым идет вектор с совпадающими словами, а вторым, идет статус документа
        SearchServer server("z"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status_doc] = server.MatchDocument("cat"s, doc_id);
        ASSERT_EQUAL(words[0], "cat"s);
    }

    { // если присутствует мину слово в запросе и оно есть в находимом документе, то вектор возвращается пустым 
        SearchServer server("z"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status_doc] = server.MatchDocument("cat -city"s, doc_id);
        ASSERT(words.empty());
    }

    { // проверка того что если нет совпадений по словам, возвращается пустой вектор для документа
        SearchServer server("z"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status_doc] = server.MatchDocument("owo"s, doc_id);
        ASSERT(words.empty());
    }
}

void TestSortAndRaiting() { // проверка того как отсортирован ввывод и того как считает рейтинг
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id2 = 43;
    const string content2 = "dog in the big city"s;
    const vector<int> ratings2 = {5, 4, 2};

    {
        SearchServer server("z"s); // так как у нас сортировка идет по убыванию релевантности, то отнимаю от наибольшего, первого, наименьшее, последнее
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto output = server.FindTopDocuments("cat in city"s);
        const auto output2 = server.FindTopDocuments("dog big city"s);
        ASSERT(output[0].relevance - output[static_cast<double>(output.size()) - 1].relevance);
    }

    {
        SearchServer server("z"s); // проверка что рейтинг считается правильно, должен равняться среднему арифм от всего рейтинга
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto output = server.FindTopDocuments("cat in city"s);
        const auto output2 = server.FindTopDocuments("dog big city"s);
        ASSERT_EQUAL(output[0].rating, 2);
        ASSERT_EQUAL(output2[0].rating, 3);
    }
}

void TestPredicate() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id2 = 43;
    const string content2 = "dog in the big city"s;
    const vector<int> ratings2 = {5, 4, 2};

    { // проверка случая когда не передается статус
        SearchServer server("z"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs[0].id, 42);
    }

    { // передача статуса
        SearchServer server("z"s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs[0].id, 42);
    }

    { // проверка на корректный пользовательский предикат
        SearchServer server("z"s);;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 42 && status == DocumentStatus::BANNED; });
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs[0].id, 42);
        
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs2 = server.FindTopDocuments("dog"s, [](int document_id, DocumentStatus status, int rating) { return rating == 3 && document_id == 43; });
        ASSERT(!found_docs2.empty());
        ASSERT_EQUAL(found_docs2[0].id, 43);
    }
}

void TestRelevance() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id2 = 43;
    const string content2 = "dog in the big city"s;
    const vector<int> ratings2 = {5, 4, 2};

    const int doc_id3 = 44;
    const string content3 = "ffff xxf the cat city"s;
    const vector<int> ratings3 = {4, 7, 1};

    const double SCOPE1 = 1e-6;
    double xIDF = log(1.0 * 3 / 2);
    double doc1rel = xIDF * (1.0 * 1 / 4);
    double doc3rel = xIDF * (1.0 * 1 / 5);

    { // передача статуса
        SearchServer server("z"s);;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT((abs(found_docs[0].relevance - doc1rel) < SCOPE1));
        ASSERT((abs(found_docs[1].relevance - doc3rel) < SCOPE1));
    }

}

void TestParalMatch() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    const string query = "curly and funny -not"s;
    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        ASSERT_EQUAL(words.size(), 1llu);
        // 1 words for document 1
    }
    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        ASSERT_EQUAL(words.size(), 2llu);
        // 2 words for document 2
    }
    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        ASSERT_EQUAL(words.size(), 0llu);
        // 0 words for document 3
    }
}

void TestParalFind() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    const string query = "funny"s;
    {
        vector<Document> output{search_server.FindTopDocuments(execution::par, query, DocumentStatus::ACTUAL)};
        ASSERT_EQUAL(output[0].id, 1);
        ASSERT(output[0].satus == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(output[1].id, 2);
        ASSERT(output[1].satus == DocumentStatus::ACTUAL);
    }
    {
        vector<Document> output{search_server.FindTopDocuments(execution::par, query)};
        ASSERT_EQUAL(output[0].id, 1);
        ASSERT(output[0].satus == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(output[1].id, 2);
        ASSERT(output[1].satus == DocumentStatus::ACTUAL);
    }
    {
        vector<Document> output{search_server.FindTopDocuments(execution::par, query, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0;})};
        ASSERT_EQUAL(output[0].id, 2);
        ASSERT(output[0].satus == DocumentStatus::ACTUAL);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWordInput);
    RUN_TEST(TestMatchDocuments);
    RUN_TEST(TestSortAndRaiting);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestParalMatch);
    RUN_TEST(TestParalFind);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}