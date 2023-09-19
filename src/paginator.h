#pragma once

#include "document.h"

    template<typename Iterator>
    class IteratorRange { // класс для хранения полуинтервала в виде {начало, конец, размер полуинтервала}
    public:
        IteratorRange(Iterator begin, Iterator end) :begin_(begin), end_(end), size_(0) {
            size_ = distance(begin_, end_); // возвращает кол-во итераторов от итер-парам-1 и до итер-парам-2
        }
    
        Iterator begin() {
            return begin_;
        }
    
        Iterator end() {
            return end_;
        }
    
        size_t size() const {
            return size_;
        }

    private:
        Iterator begin_, end_;
        size_t size_;
    };

    template<typename Iterator>
    class Paginator { // класс для организации постраничного вывода, хранит вектор полуинтервалов, которые являются страницами
    public:
        Paginator(Iterator begin, Iterator end, size_t page_size) {
            while (begin < end) {
                if (static_cast<size_t>(distance(begin, end)) < page_size) {
                    IteratorRange page(begin, end);
                    pages_.push_back(page);
                    break;
                } else {
                    auto start_begin = begin;
                    std::advance(begin, page_size); // увеличиваем переданный первым аргументом итератор на число переданное во втором аргументе
                    IteratorRange page(start_begin, begin);
                    pages_.push_back(page);
                }
            }
        }
    
        auto begin() const {
            return pages_.begin();
        }
    
        auto end() const {
            return pages_.end();
        }

    private:
        std::vector<IteratorRange<Iterator>> pages_;
    };
    
    template<typename Container>
    auto Paginate(const Container &c, size_t page_size) {
        return Paginator(begin(c), end(c), page_size);
    }
    
    std::ostream &operator<<(std::ostream &output, const Document &doc) {// перегрузка для вывода страницы
        return output << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating
                << " }";
    }
    
    template<typename Iterator>
    std::ostream &operator<<(std::ostream &output, IteratorRange<Iterator> iter) { // передаем на вывод хранящиеся итераторы
        for (auto i : iter) { // в iter хранятся интервалы по факту, так что и так работает, но на всякий оставлю явный вариант
            output << i;
        }
        
        return output;
    }
