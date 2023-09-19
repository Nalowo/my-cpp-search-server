#include <iostream>
#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) { // Удаляет дубликаты документов. Удаление происходит через множество в котором хранятся множества слов документов, при каждой итерации собирается множество текущего документа и проверяется, хранится ли оно уже в множестве слов, если да, то тогда это дубликат
    set<int> id_to_remove;
    set<set<string>> set_to_compare;
    for(int i : search_server) {
        const map<string, double>& temp1 = search_server.GetWordFrequencies(i);
        set<string> words;
        for (auto i : temp1) {
            words.insert(i.first);
        }
        if(set_to_compare.count(words)) {
            id_to_remove.insert(i);
        } else {
            set_to_compare.insert(words);
        }
    }

    for (int i : id_to_remove) {
        cout << "Found duplicate document id "s << i << '\n';
        search_server.RemoveDocument(i);
    }
}
