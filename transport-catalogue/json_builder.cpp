#include "json_builder.h"

namespace json {

// __________ BaseContext __________

Builder::BaseContext::BaseContext(Builder& builder)
: builder_(builder) {
}
Builder::DictValueContext Builder::BaseContext::Key(const std::string& key){
    return builder_.Key(key);
}
Builder::DictItemContext Builder::BaseContext::StartDict() {
    return builder_.StartDict();
}
Builder::ArrayItemContext Builder::BaseContext::StartArray() {
    return builder_.StartArray();
}
Builder& Builder::BaseContext::EndDict() {
    return builder_.EndDict();
}
Builder& Builder::BaseContext::EndArray() {
    return builder_.EndArray();
}
Builder& Builder::BaseContext::GetBuilder() {
    return builder_;
}

// __________ DictValueContext __________

Builder::DictValueContext::DictValueContext(Builder& builder)
: BaseContext(builder) {
}
Builder::DictItemContext Builder::DictValueContext::Value(const Node::Value& node) {
    return GetBuilder().Value(node);
}

// __________ DictItemContext __________

Builder::DictItemContext::DictItemContext(Builder& builder)
: BaseContext(builder) {
}

// __________ ArrayItemContext __________

Builder::ArrayItemContext::ArrayItemContext(Builder& builder)
: BaseContext(builder) {
}
Builder::ArrayItemContext Builder::ArrayItemContext::Value(const Node::Value& node) {
    return GetBuilder().Value(node);
}

// __________ Builder __________

Builder::DictValueContext Builder::Key(const std::string& key) {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Try create \"Key\" without Dict.");
    }

    nodes_stack_.back()->AsDict()[key] = Node(key);
    nodes_stack_.push_back(&(nodes_stack_.back()->AsDict().at(key)));

    return DictValueContext{*this};
}
Builder& Builder::Value(const Node::Value& node) {
    if (nodes_stack_.empty()) { // JSON представляет из себя Value-объект
        if (root_) { // объект готов
            throw std::logic_error("Try create \"Value\" for completed object.");
        }
        root_ = std::move(node);
    } else {
        Node* back_node = nodes_stack_.back();
        if (back_node->IsArray()) {
            back_node->AsArray().emplace_back(std::move(node));
        } else if (back_node->IsString()) { // последняя нода - ключ словаря
            nodes_stack_[nodes_stack_.size() - 2]->AsDict().at(back_node->AsString()) = std::move(node);
            nodes_stack_.pop_back();
        } else {
            throw std::logic_error("Expected \"Array\" or \"Dict\" on stack nodes.");
        }

    }

    return *this;
}
Builder::DictItemContext Builder::StartDict() {
    if (nodes_stack_.empty()) { // JSON представляет из себя словарь Dict
        if (root_) { // объект готов
            throw std::logic_error("Try create \"Dict\" for completed object.");
        }
        root_ = Dict{};
        nodes_stack_.push_back(&(*root_));
    } else {
        Node *back_node = nodes_stack_.back();
        if (back_node->IsArray()) {
            back_node->AsArray().emplace_back(Dict{});
            nodes_stack_.push_back(&back_node->AsArray().back());
        } else if (back_node->IsString()) { // последняя нода - ключ словаря
            auto key = back_node->AsString();
            nodes_stack_.pop_back();
            nodes_stack_.back()->AsDict().at(key) = Dict{};
            nodes_stack_.push_back(&(nodes_stack_.back()->AsDict().at(key)));
        } else {
            throw std::logic_error("Expected \"Array\" or \"Key\" when creating Dict.");
        }

    }
    return DictItemContext{*this};
}
Builder::ArrayItemContext Builder::StartArray() {
    if (nodes_stack_.empty()) { // JSON представляет из себя массив Array
        if (root_) { // объект готов
            throw std::logic_error("Try create \"Array\" for completed object.");
        }
        root_ = Array{};
        nodes_stack_.push_back(&(*root_));
    } else {
        Node* back_node = nodes_stack_.back();
        if (back_node->IsArray()) {
            back_node->AsArray().emplace_back(Array{});
            nodes_stack_.push_back(&back_node->AsArray().back());
        } else if (back_node->IsString()) { // последняя нода - ключ словаря
            auto key = back_node->AsString();
            nodes_stack_.pop_back();
            nodes_stack_.back()->AsDict().at(key) = Array{};
            nodes_stack_.push_back(&(nodes_stack_.back()->AsDict().at(key)));

        } else {
            throw std::logic_error("Expected \"Array\" or \"Dict\" when creating Array.");
        }
    }

    return ArrayItemContext{*this};
}
Builder& Builder::EndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Dict can be End if Dict there is.");
    }
    nodes_stack_.pop_back();
    return *this;
}
Builder& Builder::EndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("Array can be End if Array there is.");
    }
    nodes_stack_.pop_back();
    return *this;
}
json::Node Builder::Build() const {
    if (!root_ || !nodes_stack_.empty()) {
        throw std::logic_error("Try build empty object or non finish Array or Dict.");
    }

    return *root_;
}

} //  namespace json