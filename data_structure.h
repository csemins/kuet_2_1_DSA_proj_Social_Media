#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include <iostream>
using namespace std;

template <typename T>
class Array {
    T* data;
    int length;
    int capacity;

    void grow() {
        capacity = capacity * 2;
        T* fresh = new T[capacity];
        for (int i = 0; i < length; i++) {
            fresh[i] = data[i];
        }
        delete[] data;
        data = fresh;
    }

public:
    Array(int initial = 16) {
        capacity = initial;
        length = 0;
        data = new T[capacity];
    }

    ~Array() {
        delete[] data;
    }

    int size() {
        return length;
    }

    T& at(int index) {
        return data[index];
    }

    T& operator[](int index) {
        return data[index];
    }

    void pushBack(T value) {
        if (length == capacity) grow();
        data[length] = value;
        length++;
    }

    void insert(int index, T value) {
        if (length == capacity) grow();
        for (int i = length; i > index; i--) {
            data[i] = data[i - 1];
        }
        data[index] = value;
        length++;
    }

    void update(int index, T value) {
        data[index] = value;
    }

    void remove(int index) {
        for (int i = index; i < length - 1; i++) {
            data[i] = data[i + 1];
        }
        length--;
    }

    int search(T value) {
        for (int i = 0; i < length; i++) {
            if (data[i] == value) return i;
        }
        return -1;
    }

    void traverse() {
        for (int i = 0; i < length; i++) {
            cout << data[i] << " ";
        }
        cout << endl;
    }
};

template <typename T>
class Stack {
    T* data;
    int top;
    int capacity;

public:
    Stack(int initial = 100) {
        capacity = initial;
        data = new T[capacity];
        top = -1;
    }

    ~Stack() {
        delete[] data;
    }

    void push(T value) {
        if (top == capacity - 1) {
            capacity = capacity * 2;
            T* fresh = new T[capacity];
            for (int i = 0; i <= top; i++) fresh[i] = data[i];
            delete[] data;
            data = fresh;
        }
        top = top + 1;
        data[top] = value;
    }

    T pop() {
        T value = data[top];
        top = top - 1;
        return value;
    }

    T peek() {
        return data[top];
    }

    bool isEmpty() {
        if (top == -1) return true;
        else return false;
    }

    int size() {
        return top + 1;
    }
};

template <typename T>
class Queue {
    T* data;
    int front;
    int rear;
    int capacity;

public:
    Queue(int initial = 100) {
        capacity = initial;
        data = new T[capacity];
        front = -1;
        rear = -1;
    }

    ~Queue() {
        delete[] data;
    }

    void enqueue(T value) {
        if (rear == capacity - 1) {
            capacity = capacity * 2;
            T* fresh = new T[capacity];
            for (int i = 0; i <= rear; i++) fresh[i] = data[i];
            delete[] data;
            data = fresh;
        }
        if (front == -1) front = 0;
        rear = rear + 1;
        data[rear] = value;
    }

    T dequeue() {
        T value = data[front];
        if (front == rear) {
            front = -1;
            rear = -1;
        } else {
            front = front + 1;
        }
        return value;
    }

    T peek() {
        return data[front];
    }

    bool isEmpty() {
        if (front == -1) return true;
        else return false;
    }

    int size() {
        if (front == -1) return 0;
        return rear - front + 1;
    }
};

template <typename T>
class Deque {
    T* data;
    int front;
    int rear;
    int capacity;

public:
    Deque(int initial = 100) {
        capacity = initial;
        data = new T[capacity];
        front = -1;
        rear = -1;
    }

    ~Deque() {
        delete[] data;
    }

    bool isEmpty() {
        if (front == -1) return true;
        else return false;
    }

    bool isFull() {
        return (rear == capacity - 1) && (front == 0);
    }

    void pushBack(T value) {
        if (isFull()) return;
        if (front == -1) front = 0;
        rear = rear + 1;
        data[rear] = value;
    }

    void pushFront(T value) {
        if (isFull()) return;
        if (front == -1) {
            front = 0;
            rear = 0;
        } else if (front == 0) {
            return;
        } else {
            front = front - 1;
        }
        data[front] = value;
    }

    T popFront() {
        T value = data[front];
        if (front == rear) {
            front = -1;
            rear = -1;
        } else {
            front = front + 1;
        }
        return value;
    }

    T popBack() {
        T value = data[rear];
        if (front == rear) {
            front = -1;
            rear = -1;
        } else {
            rear = rear - 1;
        }
        return value;
    }

    T peekFront() {
        return data[front];
    }

    T peekBack() {
        return data[rear];
    }
};

template <typename T>
class LinkedList {
public:
    class Node {
    public:
        T data;
        Node* next;
        Node() {
            next = NULL;
        }
        Node(T value) {
            data = value;
            next = NULL;
        }
    };

private:
    Node* head;

public:
    LinkedList() {
        head = NULL;
    }

    ~LinkedList() {
        Node* current = head;
        while (current != NULL) {
            Node* upcoming = current->next;
            delete current;
            current = upcoming;
        }
    }

    Node* getHead() {
        return head;
    }

    bool isEmpty() {
        return head == NULL;
    }

    void insertNode(T value) {
        Node* newNode = new Node(value);
        if (head == NULL) {
            head = newNode;
            return;
        }
        Node* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }

    void insertFirst(T value) {
        Node* newNode = new Node(value);
        newNode->next = head;
        head = newNode;
    }

    void insertAfter(T key, T value) {
        Node* current = head;
        while (current != NULL && current->data != key) {
            current = current->next;
        }
        if (current == NULL) {
            cout << "Value not found" << endl;
            return;
        }
        Node* newNode = new Node(value);
        newNode->next = current->next;
        current->next = newNode;
    }

    void printList() {
        if (head == NULL) {
            cout << "List empty" << endl;
            return;
        }
        Node* current = head;
        while (current != NULL) {
            cout << current->data << " ";
            current = current->next;
        }
        cout << endl;
    }

    int searchValue(T value) {
        int position = 1;
        Node* current = head;
        while (current != NULL) {
            if (current->data == value) return position;
            current = current->next;
            position++;
        }
        return -1;
    }

    int getSize() {
        int count = 0;
        Node* current = head;
        while (current != NULL) {
            count++;
            current = current->next;
        }
        return count;
    }

    void deleteFirst() {
        if (head == NULL) return;
        Node* temp = head;
        head = head->next;
        delete temp;
    }

    void deleteLast() {
        if (head == NULL) return;
        if (head->next == NULL) {
            delete head;
            head = NULL;
            return;
        }
        Node* current = head;
        while (current->next->next != NULL) {
            current = current->next;
        }
        delete current->next;
        current->next = NULL;
    }

    void deleteNode(int position) {
        if (head == NULL) {
            cout << "List empty." << endl;
            return;
        }
        if (position > getSize()) {
            cout << "Index out of range" << endl;
            return;
        }
        if (position == 1) {
            deleteFirst();
            return;
        }
        Node* current = head;
        Node* previous = NULL;
        int i = 1;
        while (i < position) {
            previous = current;
            current = current->next;
            i++;
        }
        previous->next = current->next;
        delete current;
    }

    void deleteValue(T value) {
        if (head == NULL) return;
        if (head->data == value) {
            deleteFirst();
            return;
        }
        Node* current = head;
        while (current->next != NULL && current->next->data != value) {
            current = current->next;
        }
        if (current->next == NULL) {
            cout << "Value not found" << endl;
            return;
        }
        Node* toRemove = current->next;
        current->next = toRemove->next;
        delete toRemove;
    }
};

template <typename T>
class DoublyLinkedList {
public:
    class Node {
    public:
        T info;
        Node* prev;
        Node* next;
        Node() {
            prev = NULL;
            next = NULL;
        }
        Node(T value) {
            info = value;
            prev = NULL;
            next = NULL;
        }
    };

private:
    Node* head;

public:
    DoublyLinkedList() {
        head = NULL;
    }

    ~DoublyLinkedList() {
        Node* current = head;
        while (current != NULL) {
            Node* upcoming = current->next;
            delete current;
            current = upcoming;
        }
    }

    Node* getHead() {
        return head;
    }

    bool isEmpty() {
        return head == NULL;
    }

    void insertFirst(T value) {
        Node* newNode = new Node(value);
        if (head == NULL) {
            head = newNode;
            return;
        }
        head->prev = newNode;
        newNode->next = head;
        head = newNode;
    }

    void insertLast(T value) {
        Node* newNode = new Node(value);
        if (head == NULL) {
            head = newNode;
            return;
        }
        Node* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
        newNode->prev = current;
    }

    void insertBefore(T key, T value) {
        if (head == NULL) return;
        if (head->info == key) {
            insertFirst(value);
            return;
        }
        Node* current = head;
        while (current != NULL && current->info != key) {
            current = current->next;
        }
        if (current == NULL) {
            cout << "Value not found" << endl;
            return;
        }
        Node* newNode = new Node(value);
        newNode->prev = current->prev;
        newNode->next = current;
        current->prev->next = newNode;
        current->prev = newNode;
    }

    void insertAfter(T key, T value) {
        Node* current = head;
        while (current != NULL && current->info != key) {
            current = current->next;
        }
        if (current == NULL) {
            cout << "Value not found" << endl;
            return;
        }
        if (current->next == NULL) {
            insertLast(value);
            return;
        }
        Node* newNode = new Node(value);
        newNode->prev = current;
        newNode->next = current->next;
        current->next->prev = newNode;
        current->next = newNode;
    }

    void traverse() {
        Node* current = head;
        while (current != NULL) {
            cout << current->info << " ";
            current = current->next;
        }
        cout << endl;
    }

    void deleteFirst() {
        if (head == NULL) return;
        Node* temp = head;
        head = head->next;
        if (head != NULL) head->prev = NULL;
        delete temp;
    }

    void deleteLast() {
        if (head == NULL) return;
        if (head->next == NULL) {
            delete head;
            head = NULL;
            return;
        }
        Node* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->prev->next = NULL;
        delete current;
    }

    void deleteValue(T value) {
        if (head == NULL) return;
        if (head->info == value) {
            deleteFirst();
            return;
        }
        Node* current = head;
        while (current != NULL && current->info != value) {
            current = current->next;
        }
        if (current == NULL) return;
        if (current->next == NULL) {
            deleteLast();
            return;
        }
        current->prev->next = current->next;
        current->next->prev = current->prev;
        delete current;
    }

    void deleteBefore(T key) {
        if (head == NULL || head->next == NULL) return;
        Node* current = head;
        while (current != NULL && current->info != key) {
            current = current->next;
        }
        if (current == NULL || current->prev == NULL) return;
        if (current->prev == head) {
            deleteFirst();
            return;
        }
        Node* toRemove = current->prev;
        toRemove->prev->next = current;
        current->prev = toRemove->prev;
        delete toRemove;
    }

    void deleteAfter(T key) {
        Node* current = head;
        while (current != NULL && current->info != key) {
            current = current->next;
        }
        if (current == NULL || current->next == NULL) return;
        Node* toRemove = current->next;
        if (toRemove->next == NULL) {
            deleteLast();
            return;
        }
        current->next = toRemove->next;
        toRemove->next->prev = current;
        delete toRemove;
    }
};

template <typename T>
class BST {
public:
    class Node {
    public:
        T value;
        Node* left;
        Node* right;
        Node(T v) {
            value = v;
            left = NULL;
            right = NULL;
        }
    };

private:
    Node* root;

    Node* insertHelper(Node* node, T value) {
        if (node == NULL) return new Node(value);
        if (value < node->value) {
            node->left = insertHelper(node->left, value);
        } else if (value > node->value) {
            node->right = insertHelper(node->right, value);
        }
        return node;
    }

    Node* findMin(Node* node) {
        while (node->left != NULL) {
            node = node->left;
        }
        return node;
    }

    Node* deleteHelper(Node* node, T value) {
        if (node == NULL) return node;
        if (value < node->value) {
            node->left = deleteHelper(node->left, value);
        } else if (value > node->value) {
            node->right = deleteHelper(node->right, value);
        } else {
            if (node->left == NULL && node->right == NULL) {
                delete node;
                return NULL;
            } else if (node->left == NULL) {
                Node* temp = node->right;
                delete node;
                return temp;
            } else if (node->right == NULL) {
                Node* temp = node->left;
                delete node;
                return temp;
            } else {
                Node* successor = findMin(node->right);
                node->value = successor->value;
                node->right = deleteHelper(node->right, successor->value);
            }
        }
        return node;
    }

    bool searchHelper(Node* node, T value) {
        if (node == NULL) return false;
        if (node->value == value) return true;
        if (value < node->value) return searchHelper(node->left, value);
        return searchHelper(node->right, value);
    }

    void inorderHelper(Node* node) {
        if (node == NULL) return;
        inorderHelper(node->left);
        cout << node->value << " ";
        inorderHelper(node->right);
    }

    void preorderHelper(Node* node) {
        if (node == NULL) return;
        cout << node->value << " ";
        preorderHelper(node->left);
        preorderHelper(node->right);
    }

    void postorderHelper(Node* node) {
        if (node == NULL) return;
        postorderHelper(node->left);
        postorderHelper(node->right);
        cout << node->value << " ";
    }

    void destroyHelper(Node* node) {
        if (node == NULL) return;
        destroyHelper(node->left);
        destroyHelper(node->right);
        delete node;
    }

public:
    BST() {
        root = NULL;
    }

    ~BST() {
        destroyHelper(root);
    }

    Node* getRoot() {
        return root;
    }

    void insert(T value) {
        root = insertHelper(root, value);
    }

    void remove(T value) {
        root = deleteHelper(root, value);
    }

    bool search(T value) {
        return searchHelper(root, value);
    }

    void inorder() {
        inorderHelper(root);
        cout << endl;
    }

    void preorder() {
        preorderHelper(root);
        cout << endl;
    }

    void postorder() {
        postorderHelper(root);
        cout << endl;
    }
};

class Graph {
    int vertices;
    LinkedList<int>* adjacency;

    void DFSHelper(int node, bool* visited) {
        visited[node] = true;
        cout << node << " ";
        LinkedList<int>::Node* current = adjacency[node].getHead();
        while (current != NULL) {
            int neighbor = current->data;
            if (!visited[neighbor]) {
                DFSHelper(neighbor, visited);
            }
            current = current->next;
        }
    }

public:
    Graph(int n) {
        vertices = n;
        adjacency = new LinkedList<int>[vertices];
    }

    ~Graph() {
        delete[] adjacency;
    }

    int getVertices() {
        return vertices;
    }

    void addEdge(int u, int v) {
        adjacency[u].insertNode(v);
        adjacency[v].insertNode(u);
    }

    void addDirectedEdge(int u, int v) {
        adjacency[u].insertNode(v);
    }

    LinkedList<int>& neighbors(int u) {
        return adjacency[u];
    }

    void BFS(int start) {
        bool* visited = new bool[vertices];
        for (int i = 0; i < vertices; i++) visited[i] = false;
        Queue<int> q;
        visited[start] = true;
        q.enqueue(start);
        while (!q.isEmpty()) {
            int node = q.dequeue();
            cout << node << " ";
            LinkedList<int>::Node* current = adjacency[node].getHead();
            while (current != NULL) {
                int neighbor = current->data;
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    q.enqueue(neighbor);
                }
                current = current->next;
            }
        }
        cout << endl;
        delete[] visited;
    }

    void DFS(int start) {
        bool* visited = new bool[vertices];
        for (int i = 0; i < vertices; i++) visited[i] = false;
        DFSHelper(start, visited);
        cout << endl;
        delete[] visited;
    }
};

template <typename T>
class MaxHeap {
    T* data;
    int capacity;
    int count;

    void heapify(int i) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int largest = i;
        if (left < count && data[left] > data[largest]) largest = left;
        if (right < count && data[right] > data[largest]) largest = right;
        if (largest != i) {
            T temp = data[i];
            data[i] = data[largest];
            data[largest] = temp;
            heapify(largest);
        }
    }

public:
    MaxHeap(int initial = 100) {
        capacity = initial;
        data = new T[capacity];
        count = 0;
    }

    ~MaxHeap() {
        delete[] data;
    }

    void build(T* array, int n) {
        for (int i = 0; i < n; i++) data[i] = array[i];
        count = n;
        for (int i = n / 2 - 1; i >= 0; i--) {
            heapify(i);
        }
    }

    void insert(T value) {
        if (count == capacity) {
            capacity = capacity * 2;
            T* fresh = new T[capacity];
            for (int i = 0; i < count; i++) fresh[i] = data[i];
            delete[] data;
            data = fresh;
        }
        data[count] = value;
        int i = count;
        count++;
        while (i > 0 && data[(i - 1) / 2] < data[i]) {
            T temp = data[i];
            data[i] = data[(i - 1) / 2];
            data[(i - 1) / 2] = temp;
            i = (i - 1) / 2;
        }
    }

    T extractMax() {
        T maxValue = data[0];
        data[0] = data[count - 1];
        count--;
        heapify(0);
        return maxValue;
    }

    T peek() {
        return data[0];
    }

    bool isEmpty() {
        return count == 0;
    }

    int size() {
        return count;
    }

    void heapSort(T* array, int n) {
        build(array, n);
        for (int i = n - 1; i > 0; i--) {
            T temp = data[0];
            data[0] = data[i];
            data[i] = temp;
            count--;
            heapify(0);
        }
        for (int i = 0; i < n; i++) array[i] = data[i];
        count = n;
    }
};

template <typename T>
class PriorityQueue {
    MaxHeap<T> heap;

public:
    void enqueue(T value) {
        heap.insert(value);
    }

    T dequeue() {
        return heap.extractMax();
    }

    T peek() {
        return heap.peek();
    }

    bool isEmpty() {
        return heap.isEmpty();
    }

    int size() {
        return heap.size();
    }
};

#endif
