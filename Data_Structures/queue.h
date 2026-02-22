#ifndef QUEUE_H
#define QUEUE_H

#include <string>

class Queue {
private:
    struct Node {
        std::string url;
        Node* next;

        explicit Node(const std::string& u) : url(u), next(nullptr) {}
    };

    Node* front;
    Node* rear;

public:
    Queue() : front(nullptr), rear(nullptr) {}

    // Destructor to prevent memory leaks
    ~Queue() {
        while (!isEmpty()) {
            dequeue();
        }
    }

    void enqueue(const std::string& u) {
        Node* temp = new Node(u);

        if (rear == nullptr) {
            front = rear = temp;
            return;
        }

        rear->next = temp;
        rear = temp;
    }

    std::string dequeue() {
        if (isEmpty()) {
            return "";  // Convention: empty string means no element
        }

        Node* temp = front;
        std::string url = temp->url;

        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }

        delete temp;
        return url;
    }

    bool isEmpty() const {
        return front == nullptr;
    }

    // Optional: peek at front without removing
    std::string peek() const {
        if (isEmpty()) return "";
        return front->url;
    }

    // Optional: get current size (O(n) – acceptable for small queues)
    size_t size() const {
        size_t count = 0;
        for (Node* curr = front; curr != nullptr; curr = curr->next) {
            ++count;
        }
        return count;
    }
};

#endif