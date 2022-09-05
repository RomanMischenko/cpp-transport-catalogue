#pragma once

#include "json.h"

#include <iostream>

class Serializator {
public:
    Serializator(std::istream& input);
    void Pack() const;

private:
    json::Document document_;
};

class DeSerializator {
public:
    DeSerializator(std::istream& input);
    json::Document UnPack() const;
    const json::Document& GetProcessRequests() const;
private:
    json::Document process_requests_;
};