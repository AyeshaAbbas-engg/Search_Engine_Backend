#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <initializer_list>
#include <cstddef>

template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& val) : data(val), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    size_t _size;

public:
    LinkedList() : head(nullptr), tail(nullptr), _size(0) {}

    // Deep Copy Constructor
    LinkedList(const LinkedList& other) : head(nullptr), tail(nullptr), _size(0) {
        for (const auto& item : other) push_back(item);
    }

    // Move Constructor
    LinkedList(LinkedList&& other) noexcept : head(other.head), tail(other.tail), _size(other._size) {
        other.head = other.tail = nullptr;
        other._size = 0;
    }

    // Copy Assignment
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (const auto& item : other) push_back(item);
        }
        return *this;
    }

    // Move Assignment
    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head = other.head;
            tail = other.tail;
            _size = other._size;
            other.head = other.tail = nullptr;
            other._size = 0;
        }
        return *this;
    }

    ~LinkedList() { clear(); }

    void push_back(const T& val) {
        Node* newNode = new Node(val);
        if (!head) { head = tail = newNode; }
        else { tail->next = newNode; tail = newNode; }
        _size++;
    }

    void clear() {
        Node* current = head;
        while (current) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head = tail = nullptr;
        _size = 0;
    }

    struct Iterator {
        Node* current;
        Iterator(Node* node) : current(node) {}
        T& operator*() { return current->data; }
        Iterator& operator++() { current = current->next; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };

    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }

    struct ConstIterator {
        const Node* current;
        ConstIterator(const Node* node) : current(node) {}
        const T& operator*() const { return current->data; }
        ConstIterator& operator++() { current = current->next; return *this; }
        bool operator!=(const ConstIterator& other) const { return current != other.current; }
    };

    ConstIterator begin() const { return ConstIterator(head); }
    ConstIterator end() const { return ConstIterator(nullptr); }

    size_t size() const { return _size; }
    bool empty() const { return head == nullptr; }
    T& back() { return tail->data; }
};

#endif