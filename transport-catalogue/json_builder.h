#pragma once
#include "json.h"

#include <vector>

namespace json {

class KeyItemContext;
class StartDictItemContext;
class StartArrayItemContext;
class ValueItemContext;

class Builder {
public:
    KeyItemContext Key(std::string);
    Builder& Value(Node::Value);
    StartDictItemContext StartDict();
    StartArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    json::Node Build();
private:
    json::Node node_;
    std::vector<json::Node *> node_stack_;
    std::string key_;
    bool is_empty_ = true;
    bool have_key_ = false;
};

class KeyItemContext {
public:
    KeyItemContext(Builder& builder);
    ValueItemContext Value(Node::Value value);
    StartDictItemContext StartDict();
    StartArrayItemContext StartArray();
private:
    Builder& builder_;
};

class StartDictItemContext {
public:
    StartDictItemContext(Builder& builder);
    KeyItemContext Key(std::string key);
    Builder& EndDict();
private:
    Builder& builder_;
};

class StartArrayItemContext {
public:
    StartArrayItemContext(Builder& builder);
    StartArrayItemContext Value(Node::Value value);
    StartDictItemContext StartDict();
    StartArrayItemContext StartArray();
    Builder& EndArray();
private:
    Builder& builder_;
};

class ValueItemContext {
public:
    ValueItemContext(Builder& builder);
    KeyItemContext Key(std::string key);
    Builder& EndDict();
private:
    Builder& builder_;
};
} // namespace json