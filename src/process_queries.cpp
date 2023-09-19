#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> output(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), output.begin(), [&search_server](const std::string& query){
        return search_server.FindTopDocuments(query);
    });
    return output;
}

std::deque<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::deque<Document> output;
    const auto& rez =  ProcessQueries(search_server, queries);
    for(const auto& elem : rez) {
        for(const auto& elem2 : elem) {
            output.push_back(std::move(elem2));
        }
    }
    return output;
}