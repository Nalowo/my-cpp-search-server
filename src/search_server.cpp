#include "search_server.h"
#include "string_processing.h"

using namespace std;

SearchServer::SearchServer(const string &stop_words)
    : documents_(), stop_words_(), data_about_documents_()
{

    for (std::string &word : SplitIntoWords(stop_words))
    {
        if (!IsValidWord(word))
            throw invalid_argument("В слове "s + word + " содержатся спецсимволы"s);
        stop_words_.insert(std::string_view(*content_.insert(std::move(word)).first));
    }
}

void SearchServer::AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &raiting)
{
    if (document_id < 0)
        throw invalid_argument("ID документа не должен быть меньше нуля"s);
    if (data_about_documents_.count(document_id) != 0)
        throw invalid_argument("Документ с таким ID уже есть в системе"s);

    vector<string> words = std::move(SplitIntoWordsNoStop(document));
    const double tf_for_word = 1.0 / words.size();
    for (std::string &word : words)
    {
        auto word_iter = content_.insert(std::move(word));
        documents_[*word_iter.first][document_id] += tf_for_word;
        documenis_key_id_[document_id][*word_iter.first] += tf_for_word;
    }

    data_about_documents_.insert({document_id, {ComputeAverageRating(raiting), status}});
    document_id_list_.insert(document_id);
}

int SearchServer::GetDocumentCount() const { return documenis_key_id_.size(); }

tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const string &raw_query, int document_id) const
{
    if (!document_id_list_.count(document_id) || document_id < 0)
        throw std::out_of_range("Документ не найден"s);
    if (!IsValidWord(raw_query))
        throw std::invalid_argument("Некорректный запрос");

    Query query = ParseQueryWord(raw_query);
    query.NormalizeVec();
    DocumentStatus status = data_about_documents_.find(document_id)->second.status;
    vector<std::string_view> output_words;
    for (std::string_view word : query.minus_words_vec)
    {
        const auto id_in_index = documents_.find(word);
        if ((id_in_index != documents_.end()) && (id_in_index->second.count(document_id) > 0))
        {
            return {output_words = {}, status};
        }
    }
    for (std::string_view word : query.plus_words_vec)
    {
        const auto id_in_index = documents_.find(word);
        if ((id_in_index != documents_.end()) && (id_in_index->second.count(document_id) > 0))
        {
            output_words.push_back(word);
        }
    }

    return {output_words, status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy &, const std::string &raw_query, int document_id) const
{
    return this->MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy &policy, const std::string &raw_query, int document_id) const
{
    if (!document_id_list_.count(document_id) || document_id < 0)
        throw std::out_of_range("Документ не найден"s);
    if (!IsValidWord(raw_query))
        throw std::invalid_argument("Некорректный запрос");

    Query query = std::move(ParseQueryWord(raw_query));
    std::vector<std::string_view> output;

    if (std::any_of(query.minus_words_vec.begin(), query.minus_words_vec.end(), [&](const auto &word)
                    {
        const auto id_in_index = documents_.find(word);
        return id_in_index != documents_.end() && id_in_index->second.count(document_id) ? true : false; }))
        return {std::vector<std::string_view>(), data_about_documents_.at(document_id).status};

    output.reserve(documenis_key_id_.at(document_id).size());

    std::copy_if(make_move_iterator(query.plus_words_vec.begin()), make_move_iterator(query.plus_words_vec.end()), std::back_inserter(output), [&](const auto word)
                 {
        const auto id_in_index = documents_.find(word);
        return id_in_index != documents_.end() && id_in_index->second.count(document_id) ? true : false; });

    std::sort(execution::par, output.begin(), output.end());

    output.erase(std::unique(output.begin(), output.end()), output.end());

    return {output, data_about_documents_.at(document_id).status};
}

set<int>::const_iterator SearchServer::begin() const
{
    return document_id_list_.cbegin();
}

set<int>::const_iterator SearchServer::end() const
{
    return document_id_list_.cend();
}

void SearchServer::RemoveDocument(int document_id)
{
    if (!document_id_list_.count(document_id))
        return;
    for (auto &i : documents_)
    {
        i.second.erase(document_id);
    }
    documenis_key_id_.erase(document_id);
    document_id_list_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &, int document_id)
{
    this->RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &policy, int document_id)
{
    if (!document_id_list_.count(document_id))
        return;
    auto &docum_to_renove = documenis_key_id_.at(document_id);
    std::vector<const std::string_view *> words(docum_to_renove.size());

    std::transform(policy, docum_to_renove.begin(), docum_to_renove.end(), words.begin(), [](auto &elem)
                   { return &elem.first; });

    std::for_each(policy, words.begin(), words.end(), [this, document_id](const std::string_view *str)
                  { this->documents_.at(*str).erase(document_id); });
    documenis_key_id_.erase(document_id);
    document_id_list_.erase(document_id);
}

const map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    static const map<string_view, double> a;
    if (document_id_list_.count(document_id))
    {
        return documenis_key_id_.at(document_id);
    }
    return a;
}

double SearchServer::CountIDF(const string_view word) const
{
    return log(1.0 * document_id_list_.size() / documents_.at(word).size());
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const
{
    vector<string> words;
    for (const string &word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
            throw invalid_argument("В слове "s + word + " содержатся спецсимволы"s);
        if (!(stop_words_.count(word) > 0))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int> &raitings)
{
    if (raitings.empty())
    {
        return 0;
    }
    return static_cast<int>(accumulate(raitings.begin(), raitings.end(), 0)) / static_cast<int>(raitings.size());
}

void SearchServer::Query::NormalizeVec()
{
    if (!plus_words_vec.empty())
    {
        std::sort(execution::par, plus_words_vec.begin(), plus_words_vec.end());
        plus_words_vec.erase(std::unique(plus_words_vec.begin(), plus_words_vec.end()), plus_words_vec.end());
    }
    if (!minus_words_vec.empty())
    {
        std::sort(execution::par, minus_words_vec.begin(), minus_words_vec.end());
        minus_words_vec.erase(std::unique(minus_words_vec.begin(), minus_words_vec.end()), minus_words_vec.end());
    }
}

SearchServer::Query SearchServer::ParseQueryWord(const std::string_view text) const
{
    if (text.empty())
    {
        throw invalid_argument("Запрос не может быть пустым"s);
    }
    Query query;
    std::vector<std::string_view> words = SplitIntoWordsView(text);
    query.plus_words_vec.reserve(words.size());
    query.minus_words_vec.reserve((words.size() / 2));
    for (std::string_view &word : words)
    {
        if (word[0] != '-')
        {
            query.plus_words_vec.push_back(word);
        }
        else
        {
            word = word.substr(1);
            if (word[0] == '-' || word.empty())
                throw invalid_argument("В запросе содежатся лишние тире"s);
            if (!stop_words_.count(word))
            {
                query.minus_words_vec.push_back(word);
            }
        }
    }
    return query;
}

bool SearchServer::IsValidWord(const std::string_view word)
{
    return none_of(word.begin(), word.end(), [](char c)
                   { return c >= '\0' && c < ' '; });
}
