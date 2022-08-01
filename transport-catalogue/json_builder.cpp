#include "json_builder.h"

#include <algorithm>

using std::literals::string_literals::operator""s;

namespace json {

Builder::KeyItemContext Builder::Key(std::string value) {
    // вызов метода key при наличии другого ключа
    if (have_key_ == true) {
        throw std::logic_error("key is already ready"s);
    }
    // вызов метода key снаружи словаря
    if (node_stack_.empty() == true) {
        throw std::logic_error("dist nod started"s);
    }
    if (node_stack_.back()->IsMap() == false) {
        throw std::logic_error("can't use non-dict key"s);
    }
    key_ = std::move(value);
    have_key_ = true;
    return *this;
}

Builder& Builder::Value(Node::Value value) {
    // после предыдущего value
    if (node_stack_.empty() && is_empty_ == false) {
        throw std::logic_error("can't use value"s);
    }
    // если не после конструктора
    // если не после key
    // если не после предыдущего эл-та массива
    if (node_stack_.empty() == false &&
            have_key_ == false &&
            node_stack_.back()->IsArray() == false) {
        throw std::logic_error("can't use value"s);
    }
    if (is_empty_ == true) {
        is_empty_ = false;
    }
    // новое
    if (node_stack_.empty() == true) {
        node_ = std::move(value);
        return *this;
    }
    // из массива
    if (node_stack_.back()->IsArray() == true) {
        Array& ptr = const_cast<Array&>(node_stack_.back()->AsArray());
        ptr.push_back(std::move(value));
        return *this;
    }
    // из словаря
    if (node_stack_.back()->IsMap() == true) {
        Dict& ptr = const_cast<Dict&>(node_stack_.back()->AsMap());
        ptr.emplace(key_, std::move(value));
        have_key_ = false;
        return *this;
    }
    throw std::logic_error("can't start value"s);
    return *this;
}

Builder::StartDictItemContext Builder::StartDict() {
    // если не после конструктора
    // если не после key
    // если не после предыдущего эл-та массива
    if (node_stack_.empty() == false &&
            have_key_ == false &&
            node_stack_.back()->IsArray() == false) {
        throw std::logic_error("can't use value"s);
    }
    // новое
    if (node_stack_.empty() == true) {
        Dict tmp;
        node_ = {std::move(tmp)};
        node_stack_.push_back(&node_);
        return *this;
    }
    // из массива
    if (node_stack_.back()->IsArray() == true) {
        Array& ptr = const_cast<Array&>(node_stack_.back()->AsArray());
        ptr.push_back({Dict{}});
        Node* new_dict = const_cast<Node *>(&(node_stack_.back()->AsArray().back()));
        node_stack_.push_back(new_dict);
        return *this;
    }
    // из словаря
    if (node_stack_.back()->IsMap() == true) {
        Dict& ptr = const_cast<Dict&>(node_stack_.back()->AsMap());
        ptr.emplace(key_, Dict{});
        have_key_ = false;
        Node* new_dict = const_cast<Node *>(&(ptr.at(key_)));
        node_stack_.push_back(new_dict);
        return *this;
    }
    throw std::logic_error("can't start dict"s);
    return *this;
}

Builder::StartArrayItemContext Builder::StartArray() {
    // если не после конструктора
    // если не после key
    // если не после предыдущего эл-та массива
    if (node_stack_.empty() == false &&
            have_key_ == false &&
            node_stack_.back()->IsArray() == false) {
        throw std::logic_error("can't use value"s);
    }
    // новое
    if (node_stack_.empty() == true) {
        Array tmp;
        node_ = {std::move(tmp)};
        node_stack_.push_back(&node_);
        return *this;
    }
    // из массива
    if (node_stack_.back()->IsArray() == true) {
        Array& ptr = const_cast<Array&>(node_stack_.back()->AsArray());
        ptr.push_back({Array{}});
        Node* new_array = const_cast<Node *>(&(node_stack_.back()->AsArray().back()));
        node_stack_.push_back(new_array);
        return *this;
    }
    // из словаря
    if (node_stack_.back()->IsMap() == true) {
        Dict& ptr = const_cast<Dict&>(node_stack_.back()->AsMap());
        ptr.emplace(key_, Array{});
        have_key_ = false;
        Node* new_array = const_cast<Node *>(&(ptr.at(key_)));
        node_stack_.push_back(new_array);
        return *this;
    }
    throw std::logic_error("can't start array"s);
    return *this;
}

Builder& Builder::EndDict() {
    if (node_stack_.empty() == true) {
        throw std::logic_error("can't finish for dict"s);
    }
    if (node_stack_.back()->IsMap() == false) {
        throw std::logic_error("can't finish for dict"s);
    }
    if (is_empty_ == true) {
        is_empty_ = false;
    }
    node_stack_.pop_back();
    return *this;
}

Builder& Builder::EndArray() {
    if (node_stack_.empty() == true) {
        throw std::logic_error("can't finish for array"s);
    }
    if (node_stack_.back()->IsArray() == false) {
        throw std::logic_error("can't finish for array"s);
    }
    if (is_empty_ == true) {
        is_empty_ = false;
    }
    node_stack_.pop_back();
    return *this;
}

json::Node Builder::Build() {
    if (is_empty_ == true || node_stack_.empty() == false) {
        throw std::logic_error("object not ready"s);
    }
    return node_;
}

// ---- ItemContext ----
Builder::ItemContext::ItemContext(Builder& builder) 
: builder_(builder)
{}
Builder::KeyItemContext Builder::ItemContext::Key(std::string key) {
    builder_.Key(key);
    return builder_;
}
Builder::StartDictItemContext Builder::ItemContext::StartDict() {
    builder_.StartDict();
    return builder_;
}
Builder::StartArrayItemContext Builder::ItemContext::StartArray() {
    builder_.StartArray();
    return builder_;
}
Builder& Builder::ItemContext::EndDict() {
    builder_.EndDict();
    return builder_;
}
Builder& Builder::ItemContext::EndArray() {
    builder_.EndArray();
    return builder_;
}

// ---- KeyItemContext ----
Builder::KeyItemContext::KeyItemContext(Builder& builder)
: ItemContext::ItemContext(builder)
{}
Builder::ValueItemContext Builder::KeyItemContext::Value(Node::Value value) {
    builder_.Value(value);
    return builder_;
}

// ---- StartDictItemContext ----
Builder::StartDictItemContext::StartDictItemContext(Builder& builder)
: ItemContext::ItemContext(builder)
{}

// ---- StartArrayItemContext ----
Builder::StartArrayItemContext::StartArrayItemContext(Builder& builder)
: ItemContext::ItemContext(builder)
{}

// ---- StartArrayItemContext ----
Builder::StartArrayItemContext Builder::StartArrayItemContext::Value(Node::Value value) {
    builder_.Value(value);
    return builder_;
}

// ---- ValueItemContext ----
Builder::ValueItemContext::ValueItemContext(Builder& builder)
: ItemContext::ItemContext(builder)
{}

} // namespace json