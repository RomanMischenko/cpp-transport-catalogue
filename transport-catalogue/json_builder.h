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

    class ItemContext {
    public:
        ItemContext();
        ItemContext(Builder& builder);

        KeyItemContext Key(std::string);
        StartDictItemContext StartDict();
        StartArrayItemContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
    protected:
        Builder& builder_;
    };

private:
    json::Node node_;
    std::vector<json::Node *> node_stack_;
    std::string key_;
    bool is_empty_ = true;
    bool have_key_ = false;
};

class KeyItemContext : public Builder::ItemContext {
public:
    KeyItemContext(Builder& builer);
    ValueItemContext Value(Node::Value value);

    Builder& EndDict() = delete;
    Builder& EndArray() = delete;
private:
    Builder& builder_;
};

class StartDictItemContext : public Builder::ItemContext {
public:
    StartDictItemContext(Builder& builder);

    StartDictItemContext StartDict() = delete;
    StartArrayItemContext StartArray() = delete;
    Builder& EndArray() = delete;
private:
    Builder& builder_;
};

class StartArrayItemContext : public Builder::ItemContext {
public:
    StartArrayItemContext(Builder& builder);
    StartArrayItemContext Value(Node::Value value);

    KeyItemContext Key(std::string) = delete;
    Builder& EndDict() = delete;
private:
    Builder& builder_;
};

class ValueItemContext : public Builder::ItemContext {
public:
    ValueItemContext(Builder& builder);
    Builder& EndArray() = delete;
    StartDictItemContext StartDict() = delete;
    StartArrayItemContext StartArray() = delete;
private:
    Builder& builder_;
};
} // namespace json