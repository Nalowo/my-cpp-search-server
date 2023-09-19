#pragma once

#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <tuple>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <execution>
#include <mutex>

#include "document.h"
#include "concurrent_map.h"
#include "log_duration.h"

#define MAX_RESULT_DOCUMENT_COUNT 5
#define SCOPE 1e-6

class SearchServer
{

public:
    explicit SearchServer(const std::string &stop_words);
    template <typename ContainerInput>
    explicit SearchServer(const ContainerInput &stop_words);

    void AddDocument(int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &raiting);

    int GetDocumentCount() const;

    template <typename ExecutionPolicy, typename Predicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, Predicate predicat) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, DocumentStatus status_in) const
    {
        return FindTopDocuments(policy, raw_query, [&status_in](int document_id, DocumentStatus status, int rating)
                                { return status == status_in; });
    }
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query) const
    {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }
    template <typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, Predicate predicat) const
    {
        return FindTopDocuments(std::execution::seq, raw_query, predicat);
    }
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status_in) const
    {
        return FindTopDocuments(std::execution::seq, raw_query, status_in);
    }
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const
    {
        return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
    }

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string &raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy &, const std::string &raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy &policy, const std::string &raw_query, int document_id) const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy &, int document_id);
    void RemoveDocument(const std::execution::parallel_policy &policy, int document_id);

private:
    struct Query
    {
        std::vector<std::string_view> plus_words_vec;
        std::vector<std::string_view> minus_words_vec;

        void NormalizeVec();
    };

    struct MetaDataOfDocument
    {
        int raiting;
        DocumentStatus status;
    };
    std::map<std::string_view, std::map<int, double>> documents_;
    std::set<std::string_view> stop_words_;
    std::map<int, MetaDataOfDocument> data_about_documents_;
    std::map<int, std::map<std::string_view, double>> documenis_key_id_;
    std::set<int> document_id_list_;
    std::set<std::string, std::less<>> content_;

    double CountIDF(const std::string_view word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const;

    static int ComputeAverageRating(const std::vector<int> &raitings);

    Query ParseQueryWord(const std::string_view text) const;

    static bool IsValidWord(const std::string_view word);

    template <typename Predicat>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy &policy, const Query &query_words, Predicat predicat) const;
    template <typename Predicat>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy &policy, const Query &query_words, Predicat predicat) const;
};

template <typename ContainerInput>
SearchServer::SearchServer(const ContainerInput &stop_words)
    : documents_(), stop_words_(), data_about_documents_()
{

    for (const std::string &word : stop_words)
    {
        {
            using namespace std::string_literals;
            if (!IsValidWord(word))
                throw std::invalid_argument("Стоп слова содержат недопустимые символы");
        }
        stop_words_.insert(std::string_view(*content_.insert(word).first));
    }
}

template <typename ExecutionPolicy, typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, Predicate predicat) const
{
    if (!IsValidWord(raw_query))
        throw std::invalid_argument("Некорректный запрос");
    std::vector<Document> matched_documents;
    Query query = ParseQueryWord(raw_query);
    query.NormalizeVec();
    matched_documents = std::move(FindAllDocuments(policy, query, predicat));

    std::sort(policy, matched_documents.begin(), matched_documents.end(),
              [](const Document &lhs, const Document &rhs)
              {
                  if ((std::abs(lhs.relevance - rhs.relevance)) < SCOPE)
                  {
                      return lhs.rating > rhs.rating;
                  }
                  return lhs.relevance > rhs.relevance;
              });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename Predicat>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy &policy, const Query &query_words, Predicat predicat) const
{
    std::vector<Document> matched_documents;
    ConcurrentMap<int, double> document_to_relevance(150);
    std::for_each(std::execution::par, query_words.plus_words_vec.begin(), query_words.plus_words_vec.end(), [&](const auto &plus_words)
                  {
        const auto element_of_map_doc = documents_.find(plus_words); 
        if (element_of_map_doc != documents_.end()) { 
            const double idf = CountIDF(plus_words);
            for (const auto& [ID, TF] : element_of_map_doc->second) {
                document_to_relevance[ID].ref_to_value += idf * TF;
            }
        } });
    std::for_each(std::execution::par, query_words.minus_words_vec.begin(), query_words.minus_words_vec.end(), [&](const auto &minus_words)
                  {
        const auto element_of_map_doc = documents_.find(minus_words);
        if (element_of_map_doc != documents_.end()){
            for (const auto& [ID, TF] : element_of_map_doc->second) {
                document_to_relevance.erase(ID);
            }
        } });
    for (const auto &[id, rel] : document_to_relevance.BuildOrdinaryMap())
    {
        const auto meta_data = data_about_documents_.find(id);
        if (predicat(id, meta_data->second.status, meta_data->second.raiting))
        {
            matched_documents.push_back({id, rel, meta_data->second.raiting, meta_data->second.status});
        }
    }
    return matched_documents;
}

template <typename Predicat>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy &policy, const Query &query_words, Predicat predicat) const
{
    std::vector<Document> matched_documents;
    std::map<int, double> document_to_relevance;
    for (const auto &plus_words : query_words.plus_words_vec)
    {
        const auto element_of_map_doc = documents_.find(plus_words);
        if (element_of_map_doc != documents_.end())
        {
            const double idf = CountIDF(plus_words);
            for (const auto &[ID, TF] : element_of_map_doc->second)
            {
                document_to_relevance[ID] += idf * TF;
            }
        }
    }
    for (const auto &minus_words : query_words.minus_words_vec)
    {
        const auto element_of_map_doc = documents_.find(minus_words);
        if (element_of_map_doc != documents_.end())
        {
            for (const auto &[ID, TF] : element_of_map_doc->second)
            {
                document_to_relevance.erase(ID);
            }
        }
    }
    for (const auto &[id, rel] : document_to_relevance)
    {
        const auto meta_data = data_about_documents_.find(id);
        if (predicat(id, meta_data->second.status, meta_data->second.raiting))
        {
            matched_documents.push_back({id, rel, meta_data->second.raiting, meta_data->second.status});
        }
    }
    return matched_documents;
}
