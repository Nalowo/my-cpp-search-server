// #include <vector>

#include "request_queue.h"
#include "search_server.h"

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        const auto result = search_server_.FindTopDocuments(raw_query, status);
        RequestQueue::AddRequest(result.size());
        return result;
    }

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        const auto result = search_server_.FindTopDocuments(raw_query);
        RequestQueue::AddRequest(result.size());
        return result;
    }

    int RequestQueue::GetNoResultRequests() const {
        return no_results_requests_;
    }

    void RequestQueue::AddRequest(int results_num) {
        // новый запрос - новая секунда
        ++current_time_;
        // удаляем все результаты поиска, которые устарели
        while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
            if (0 == requests_.front().results) {
                --no_results_requests_;
            }
            requests_.pop_front();
        }
        // сохраняем новый результат поиска
        requests_.push_back({static_cast<uint64_t>(current_time_), results_num});
        if (0 == results_num) {
            ++no_results_requests_;
        }
}