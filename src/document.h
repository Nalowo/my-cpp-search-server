#pragma once

enum class DocumentStatus { //перечислимый тип в котором описывается актуальность
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document { // структура в которой хранятся результаты

    Document() = default;

    Document(const int id_in, const double relevanc_in, const int rating_in,  DocumentStatus satus_in = DocumentStatus::IRRELEVANT) :id(id_in), relevance(relevanc_in), rating(rating_in), satus(satus_in) {}

    int id = 0;
    double relevance = 0;
    int rating = 0; // тут хранится рейтинг
    DocumentStatus satus = DocumentStatus::IRRELEVANT; // для хранения актуальности
};