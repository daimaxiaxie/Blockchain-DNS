//
// Created by lenovo on 2022/1/21.
//

#ifndef BLOCKCHAIN_DNS_LIST_H
#define BLOCKCHAIN_DNS_LIST_H

#include <jemalloc/jemalloc.h>
#include <stdexcept>

template<typename T>
struct Node {
    Node *prev, *next;
    T data;
};

template<typename T>
struct Endpoint {
    Node<T> *prev, *next;
};

template<typename T>
void nothing(T &t) {
}

template<typename T>
class List {
public:
    List();

    ~List();

    void clear(void (func)(T &t));

    Node<T> *push_back(T &&data);

    Node<T> *push_front(T &&data);

    void pop_back();

    void pop_back(void (func)(T &t));

    void pop_front(void (func)(T &t));

    T &back();

    T &front();

    bool empty();

    bool erase(Node<T> *node);

    bool erase(Node<T> *node, void (func)(T &t));

    bool move_to_front(Node<T> *node);

    bool move_to_back(Node<T> *node);

    size_t size();

private:
    Endpoint<T> *head, *tail;
    size_t count;
};

template<typename T>
List<T>::List(): count(0) {
    head = (Endpoint<T> *) malloc(sizeof(Endpoint<T>));
    tail = (Endpoint<T> *) malloc(sizeof(Endpoint<T>));

    if (head == nullptr || tail == nullptr) {
        printf("jemalloc malloc List error\n");
        exit(0);
    }
    head->prev = nullptr;
    head->next = (Node<T> *) tail;
    tail->prev = (Node<T> *) head;
    tail->next = nullptr;
}

template<typename T>
List<T>::~List() {
    clear(nothing<T>);
    free(head);
    free(tail);
}

template<typename T>
void List<T>::clear(void (func)(T &t)) {
    Node<T> *ptr = head->next;
    while (ptr != (Node<T> *) tail) {
        func(ptr->data);
        ptr = ptr->next;
        //free(ptr->prev);
        delete ptr->prev;
    }
    head->prev = nullptr;
    head->next = (Node<T> *) tail;
    tail->prev = (Node<T> *) head;
    tail->next = nullptr;
}

template<typename T>
Node<T> *List<T>::push_back(T &&data) {
    /*
    Node<T> *node = malloc(sizeof(Node<T>));
    if (node == nullptr) {
        return nullptr;
    }
    new(node)Node<T>();*/
    Node<T> *node = new Node<T>();

    node->data = std::move(data);
    tail->prev->next = node;
    node->prev = tail->prev;
    node->next = (Node<T> *) tail;
    tail->prev = node;
    return node;
}

template<typename T>
Node<T> *List<T>::push_front(T &&data) {
    Node<T> *node = new Node<T>();

    node->data = std::move(data);
    head->next->prev = node;
    node->prev = (Node<T> *) head;
    node->next = head->next;
    head->next = node;
    return node;
}

template<typename T>
void List<T>::pop_back() {
    if (head->next == (Node<T> *) tail || tail->prev == (Node<T> *) head) {
        return;
    }
    Node<T> *trash = tail->prev;
    tail->prev->prev->next = (Node<T> *) tail;
    tail->prev = tail->prev->prev;
    //free(trash);
    delete trash;
    --count;
}

template<typename T>
void List<T>::pop_back(void (*func)(T &)) {
    if (head->next == (Node<T> *) tail || tail->prev == (Node<T> *) head) {
        return;
    }
    func(tail->prev->data);
    pop_back();
}

template<typename T>
void List<T>::pop_front(void (*func)(T &)) {
    if (head->next == (Node<T> *) tail || tail->prev == (Node<T> *) head) {
        return;
    }
    Node<T> *trash = head->next;
    head->next->next->prev = head;
    head->next = head->next->next;
    func(trash->data);
    //free(trash);
    delete trash;
    --count;
}

template<typename T>
T &List<T>::back() {
    if (tail->prev == (Node<T> *) head) {
        throw std::out_of_range("List back");
    }
    return tail->prev->data;
}

template<typename T>
T &List<T>::front() {
    if (head->next == tail) {
        throw std::out_of_range("List front");
    }
    return head->next->data;
}

template<typename T>
bool List<T>::empty() {
    if (head->next == tail || tail->prev == head) {
        return true;
    }
    return false;
}

template<typename T>
bool List<T>::erase(Node<T> *node) {
    if (node == nullptr) {
        return false;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    //free(node);
    delete node;
    --count;
    return true;
}

template<typename T>
bool List<T>::erase(Node<T> *node, void (*func)(T &)) {
    if (node == nullptr) {
        return false;
    }
    func(node->data);
    erase(node);
    return true;
}

template<typename T>
bool List<T>::move_to_front(Node<T> *node) {
    if (node == nullptr) {
        return false;
    } else if (head->next == node) {
        return true;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;

    head->next->prev = node;
    node->prev = (Node<T> *) head;
    node->next = head->next;
    head->next = node;
    return true;
}

template<typename T>
bool List<T>::move_to_back(Node<T> *node) {
    if (node == nullptr) {
        return false;
    } else if (tail->prev == node) {
        return true;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;

    tail->prev->next = node;
    node->prev = tail->prev;
    node->next = tail;
    tail->prev = node;
    return true;
}

template<typename T>
size_t List<T>::size() {
    return count;
}


#endif //BLOCKCHAIN_DNS_LIST_H
