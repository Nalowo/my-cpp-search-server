#include "string_processing.h"


std::vector<std::string> SplitIntoWords(const std::string& text) { // разбивает строки на слова и возвращает вектор
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
    if (str.empty()) return {};
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.size(), str.find_first_not_of(" ")));
    int64_t space = 0;
    while (!str.empty()) {
        space = str.find(' ');
        result.push_back(str.substr(0, space));
        str.remove_prefix(std::min(str.size(),str.find_first_not_of(" ", space)));
    }
    return result;
}
