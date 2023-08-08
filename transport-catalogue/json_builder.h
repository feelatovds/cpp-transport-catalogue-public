#pragma once

#include <vector>
#include <optional>
#include <variant>

#include "json.h"

namespace json {

class Builder {
public:
    class BaseContext;
    class DictValueContext;
    class DictItemContext;
    class ArrayItemContext;

    DictValueContext Key(const std::string& key);
    Builder& Value(const Node::Value& node);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    json::Node Build() const;

private:
    std::optional<Node> root_;
    std::vector<Node*> nodes_stack_;
};

class Builder::BaseContext {
public:
    BaseContext(Builder& builder);

    DictValueContext Key(const std::string& key);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();

    Builder& GetBuilder();

private:
    Builder& builder_;
};

class Builder::DictValueContext : public BaseContext {
public:
    DictValueContext(Builder& builder);

    DictItemContext Value(const Node::Value& node);
    DictValueContext Key(const std::string& key) =delete;
    Builder& EndDict() =delete;
    Builder& EndArray() =delete;
};

class Builder::DictItemContext : public BaseContext {
public:
    DictItemContext(Builder& builder);

    DictItemContext StartDict() =delete;
    ArrayItemContext StartArray() =delete;
    Builder& EndArray() =delete;
};

class Builder::ArrayItemContext : public BaseContext {
public:
    ArrayItemContext(Builder& builder);

    ArrayItemContext Value(const Node::Value& node);
    DictValueContext Key(const std::string& key) =delete;
    Builder& EndDict() =delete;
};

} //  namespace json