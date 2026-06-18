#ifndef DS_H
#define DS_H

/*Pure Data Structure Header
Structures:
Array<T> - dynamic array
LinkedList<T> - singly linked,
DoublyLinkedList<T> - bi-directional
Deque<T> - double-ended queue(uses DLL)
Stack<T> - uses LinkedList
Queue<T> -  uses LinkedList
Graph - undirected adjacency list, BFS & DFS
BST - binary search tree, string to int map
RankedPost, MaxHeap - max-heap for priority ranking
Sorter<T> - QuickSort & HeapSort on Array
Token, tokenise, infixToPostfix, evalPostfix - boolean search expression evaluator
*/

#include <iostream>
#include <string>
using namespace std;

//Array
template <typename T>
class Array {
    T* d; int len; int cap;
    void grow() {
        cap *= 2; T* f = new T[cap];
        for (int i = 0; i < len; i++) f[i] = d[i];
        delete[] d; d = f;
    }
public:
    Array(int init = 8) : len(0), cap(init) { d = new T[cap]; }
    ~Array() { delete[] d; }
    int  size() const { return len; }
    bool empty() const { return len == 0; }
    T&   operator[](int i) { return d[i]; }
    const T& operator[](int i) const { return d[i]; }
    void pushBack(T v) { if (len == cap) grow(); d[len++] = v; }
    void clear() { len = 0; }
    void removeAt(int i) {
        for (int j = i; j < len - 1; j++) d[j] = d[j+1];
        len--;
    }
};


//LinkedList
template <typename T>
class LinkedList {
public:
    struct Node { T data; Node* next; Node(T v) : data(v), next(nullptr) {} };
private:
    Node* head; Node* tail; int cnt;
public:
    LinkedList() : head(nullptr), tail(nullptr), cnt(0) {}
    ~LinkedList() { clear(); }
    Node* getHead() const { return head; }
    bool  empty() const { return head == nullptr; }
    int   size() const { return cnt; }
    void  clear() { while (head) { Node* t = head; head = head->next; delete t; } tail = nullptr; cnt = 0; }
    void  pushBack(T v) {
        Node* n = new Node(v);
        if (!head) head = tail = n; else { tail->next = n; tail = n; }
        cnt++;
    }
    void  pushFront(T v) {
        Node* n = new Node(v); n->next = head; head = n;
        if (!tail) tail = n; cnt++;
    }
    T     front() { return head->data; }
    void  popFront() {
        if (!head) return;
        Node* t = head; head = head->next;
        if (!head) tail = nullptr;
        delete t; cnt--;
    }
};


//DoublyLinkedList
template <typename T>
class DoublyLinkedList {
public:
    struct Node { T data; Node* prev; Node* next;
                  Node(T v) : data(v), prev(nullptr), next(nullptr) {} };
private:
    Node* head; Node* tail; int cnt;
public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), cnt(0) {}
    ~DoublyLinkedList() {
        Node* c = head;
        while (c) { Node* nx = c->next; delete c; c = nx; }
    }
    Node* getHead() const { return head; }
    Node* getTail() const { return tail; }
    bool  empty() const { return head == nullptr; }
    int   size() const { return cnt; }
    Node* pushBack(T v) {
        Node* n = new Node(v);
        if (!head) head = tail = n; else { n->prev = tail; tail->next = n; tail = n; }
        cnt++; return n;
    }
    void remove(Node* n) {
        if (!n) return;
        if (n->prev) n->prev->next = n->next; else head = n->next;
        if (n->next) n->next->prev = n->prev; else tail = n->prev;
        delete n; cnt--;
    }
};


//Deque
template <typename T>
class Deque {
    DoublyLinkedList<T> list;
    int maxSz;
public:
    Deque(int mx = 20) : maxSz(mx) {}
    void pushFront(T v) {
        list.pushBack(v);
        while (list.size() > maxSz)
            list.remove(list.getHead());
    }
    bool empty() const { return list.empty(); }
    int  size() const { return list.size(); }
    void printAll(const string& user = "") const {
        typename DoublyLinkedList<T>::Node* c = list.getTail();
        int n = 0;
        while (c) {
            // first string of each entry is the username; show only this user's activity
            if (user.empty() || c->data.substr(0, c->data.find(' ')) == user) {
                cout << "  › " << c->data << "\n";
                n++;
            }
            c = c->prev;
        }
        if (n == 0) cout << "  (empty)\n";
    }
};


//Stack
template <typename T>
class Stack {
    LinkedList<T> list;
public:
    void push(T v) { list.pushFront(v); }
    T    pop() { T v = list.front(); list.popFront(); return v; }
    T    peek() { return list.front(); }
    bool empty() { return list.empty(); }
    int  size() { return list.size(); }
};


//Queue
template <typename T>
class Queue {
    LinkedList<T> list;
public:
    void enqueue(T v) { list.pushBack(v); }
    T    dequeue() { T v = list.front(); list.popFront(); return v; }
    T    peek() { return list.front(); }
    bool empty() { return list.empty(); }
    int  size() { return list.size(); }
};


//Graph
class Graph {
    LinkedList<int>* adj;
    int cap, verts;
    void grow(int needed) {
        int nc = cap; while (nc <= needed) nc *= 2;
        LinkedList<int>* f = new LinkedList<int>[nc];
        for (int i = 0; i < verts; i++) {
            LinkedList<int>::Node* c = adj[i].getHead();
            while (c) { f[i].pushBack(c->data); c = c->next; }
        }
        delete[] adj; adj = f; cap = nc;
    }
public:
    Graph(int init = 16) : cap(init), verts(0) { adj = new LinkedList<int>[cap]; }
    ~Graph() { delete[] adj; }

    void ensureVertex(int v) {
        if (v >= cap) grow(v);
        if (v + 1 > verts) verts = v + 1;
    }
    bool areFriends(int u, int v) {
        if (u >= verts) return false;
        LinkedList<int>::Node* c = adj[u].getHead();
        while (c) { if (c->data == v) return true; c = c->next; }
        return false;
    }
    void addEdge(int u, int v) {
        if (areFriends(u, v)) return;
        ensureVertex(u); ensureVertex(v);
        adj[u].pushBack(v); adj[v].pushBack(u);
    }
    void removeEdge(int u, int v) {
        if (u >= verts || v >= verts) return;
        LinkedList<int> nu, nv;
        LinkedList<int>::Node* c = adj[u].getHead();
        while (c) { if (c->data != v) nu.pushBack(c->data); c = c->next; }
        c = adj[v].getHead();
        while (c) { if (c->data != u) nv.pushBack(c->data); c = c->next; }
        adj[u].clear(); adj[v].clear();
        c = nu.getHead(); while (c) { adj[u].pushBack(c->data); c = c->next; }
        c = nv.getHead(); while (c) { adj[v].pushBack(c->data); c = c->next; }
    }
    Array<int> getFriends(int u) {
        Array<int> r;
        if (u >= verts) return r;
        LinkedList<int>::Node* c = adj[u].getHead();
        while (c) { r.pushBack(c->data); c = c->next; }
        return r;
    }
    //BFS: friend-of-friend suggestions with mutual count
    Array<int> suggestFriendsBFS(int start, int* mutuals, int& resCount) {
        Array<int> res; resCount = 0;
        if (start >= verts) return res;
        bool* visited = new bool[verts]();
        int*  dist = new int[verts]();
        visited[start] = true;
        Queue<int> q; q.enqueue(start);
        while (!q.empty()) {
            int node = q.dequeue();
            LinkedList<int>::Node* c = adj[node].getHead();
            while (c) {
                int nb = c->data;
                if (!visited[nb]) {
                    visited[nb] = true; dist[nb] = dist[node] + 1;
                    if (dist[nb] == 2) {
                        int mutual = 0;
                        LinkedList<int>::Node* a = adj[start].getHead();
                        while (a) { if (areFriends(nb, a->data)) mutual++; a = a->next; }
                        res.pushBack(nb); mutuals[resCount++] = mutual;
                    }
                    if (dist[nb] < 2) q.enqueue(nb);
                }
                c = c->next;
            }
        }
        delete[] visited; delete[] dist; return res;
    }
    //DFS: all users reachable from start (connected component)
    Array<int> reachableDFS(int start) {
        Array<int> res;
        if (start >= verts) return res;
        bool* visited = new bool[verts]();
        Stack<int> stk; stk.push(start); visited[start] = true;
        while (!stk.empty()) {
            int node = stk.pop();
            if (node != start) res.pushBack(node);
            LinkedList<int>::Node* c = adj[node].getHead();
            while (c) {
                if (!visited[c->data]) { visited[c->data] = true; stk.push(c->data); }
                c = c->next;
            }
        }
        delete[] visited; return res;
    }
    void saveEdges(int u, ofstream& f) {
        if (u >= verts) { f << "\n"; return; }
        LinkedList<int>::Node* c = adj[u].getHead();
        while (c) { f << c->data << " "; c = c->next; }
        f << "\n";
    }
};


//BST(username -> uid directory; sorted listing
class BST {
    struct Node { string v; int uid; Node* l; Node* r;
                  Node(string x, int i) : v(x), uid(i), l(nullptr), r(nullptr) {} };
    Node* root;
    Node* ins(Node* n, const string& x, int id) {
        if (!n) return new Node(x, id);
        if (x < n->v) n->l = ins(n->l, x, id);
        else if (x > n->v) n->r = ins(n->r, x, id);
        return n;
    }
    int srch(Node* n, const string& x) {
        if (!n) return -1;
        if (n->v == x) return n->uid;
        return x < n->v ? srch(n->l, x) : srch(n->r, x);
    }
    void inord(Node* n, Array<string>& out) {
        if (!n) return;
        inord(n->l, out); out.pushBack(n->v); inord(n->r, out);
    }
    void destroy(Node* n) { if (!n) return; destroy(n->l); destroy(n->r); delete n; }
public:
    BST() : root(nullptr) {}
    ~BST() { destroy(root); }
    void insert(const string& x, int id) { root = ins(root, x, id); }
    int  search(const string& x) { return srch(root, x); }
    Array<string> sorted() { Array<string> a; inord(root, a); return a; }
};


//RankedPost + MaxHeap(top-posts priority queue by likes)
struct RankedPost {
    int likes; int postId;
    bool operator<(const RankedPost& o) const {
        return likes != o.likes ? likes < o.likes : postId < o.postId;
    }
};

class MaxHeap {
    RankedPost* d; int cap; int cnt;
    void siftUp(int i) {
        while (i > 0 && d[(i-1)/2] < d[i]) {
            RankedPost t = d[i]; d[i] = d[(i-1)/2]; d[(i-1)/2] = t; i = (i-1)/2;
        }
    }
    void siftDown(int i) {
        int lg = i, l = 2*i+1, r = 2*i+2;
        if (l < cnt && d[lg] < d[l]) lg = l;
        if (r < cnt && d[lg] < d[r]) lg = r;
        if (lg != i) { RankedPost t = d[i]; d[i] = d[lg]; d[lg] = t; siftDown(lg); }
    }
    void grow() { cap *= 2; RankedPost* f = new RankedPost[cap]; for (int i=0;i<cnt;i++) f[i]=d[i]; delete[] d; d=f; }
public:
    MaxHeap(int init=16) : cap(init), cnt(0) { d = new RankedPost[cap]; }
    ~MaxHeap() { delete[] d; }
    void insert(RankedPost v) { if (cnt==cap) grow(); d[cnt]=v; siftUp(cnt++); }
    RankedPost extractMax() { RankedPost t=d[0]; d[0]=d[--cnt]; siftDown(0); return t; }
    bool empty() { return cnt==0; }
    int size() { return cnt; }
};


//Sorter (QuickSort + HeapSort)
template <typename T>
class Sorter {
    static int partition(Array<T>& a, int lo, int hi) {
        T pivot = a[hi]; int i = lo - 1;
        for (int j = lo; j < hi; j++)
            if (!(pivot < a[j])) { i++; T t=a[i]; a[i]=a[j]; a[j]=t; }
        T t=a[i+1]; a[i+1]=a[hi]; a[hi]=t; return i+1;
    }
    static void qs(Array<T>& a, int lo, int hi) {
        if (lo >= hi) return;
        int p = partition(a, lo, hi); qs(a, lo, p-1); qs(a, p+1, hi);
    }
    static void heapify(Array<T>& a, int n, int i) {
        int lg=i, l=2*i+1, r=2*i+2;
        if (l<n && a[lg]<a[l]) lg=l;
        if (r<n && a[lg]<a[r]) lg=r;
        if (lg!=i) { T t=a[i]; a[i]=a[lg]; a[lg]=t; heapify(a,n,lg); }
    }
    static void hs(Array<T>& a, int n) {
        for (int i=n/2-1;i>=0;i--) heapify(a,n,i);
        for (int i=n-1;i>0;i--) { T t=a[0]; a[0]=a[i]; a[i]=t; heapify(a,i,0); }
    }
public:
    static void quickSort(Array<T>& a, bool desc=false) {
        if (a.size()>1) qs(a,0,a.size()-1);
        if (desc) { int n=a.size(); for(int i=0;i<n/2;i++){T t=a[i];a[i]=a[n-1-i];a[n-1-i]=t;} }
    }
    static void heapSort(Array<T>& a, bool desc=false) {
        hs(a, a.size());
        if (desc) { int n=a.size(); for(int i=0;i<n/2;i++){T t=a[i];a[i]=a[n-1-i];a[n-1-i]=t;} }
    }
};


//Boolean Search Expression Evaluator Infix -> Postfix -> evaluate per text.
struct Token {
    enum Kind { WORD, AND_OP, OR_OP, LPAREN, RPAREN } kind;
    string word;
    Token() : kind(WORD) {}
    Token(Kind k) : kind(k) {}
    Token(Kind k, const string& w) : kind(k), word(w) {}
};

inline Array<Token> tokenise(const string& query) {
    Array<Token> toks; string cur;
    auto flush = [&]() {
        if (cur.empty()) return;
        string up = cur; for (auto& c : up) c = toupper(c);
        if (up == "AND") toks.pushBack(Token(Token::AND_OP));
        else if (up == "OR") toks.pushBack(Token(Token::OR_OP));
        else toks.pushBack(Token(Token::WORD, cur));
        cur.clear();
    };
    for (char c : query) {
        if (c == '(') { flush(); toks.pushBack(Token(Token::LPAREN)); }
        else if (c == ')') { flush(); toks.pushBack(Token(Token::RPAREN)); }
        else if (c == ' ') flush();
        else cur += c;
    }
    flush(); return toks;
}

inline Array<Token> infixToPostfix(const Array<Token>& toks) {
    Array<Token> out; Stack<Token> ops;
    auto prec = [](Token::Kind k) -> int {
        if (k == Token::AND_OP) return 2;
        if (k == Token::OR_OP)  return 1;
        return 0;
    };
    for (int i = 0; i < toks.size(); i++) {
        const Token& t = toks[i];
        if (t.kind == Token::WORD) {
            out.pushBack(t);
        } else if (t.kind == Token::AND_OP || t.kind == Token::OR_OP) {
            while (!ops.empty() &&
                   (ops.peek().kind == Token::AND_OP || ops.peek().kind == Token::OR_OP) &&
                   prec(ops.peek().kind) >= prec(t.kind))
                out.pushBack(ops.pop());
            ops.push(t);
        } else if (t.kind == Token::LPAREN) {
            ops.push(t);
        } else if (t.kind == Token::RPAREN) {
            while (!ops.empty() && ops.peek().kind != Token::LPAREN)
                out.pushBack(ops.pop());
            if (!ops.empty()) ops.pop();
        }
    }
    while (!ops.empty()) out.pushBack(ops.pop());
    return out;
}

inline bool evalPostfix(const Array<Token>& postfix, const string& text) {
    string lo = text; for (auto& c : lo) c = tolower(c);
    Stack<bool> stk;
    for (int i = 0; i < postfix.size(); i++) {
        const Token& t = postfix[i];
        if (t.kind == Token::WORD) {
            string w = t.word; for (auto& c : w) c = tolower(c);
            stk.push(lo.find(w) != string::npos);
        } else if (t.kind == Token::AND_OP) {
            bool b = stk.pop(), a = stk.pop(); stk.push(a && b);
        } else if (t.kind == Token::OR_OP) {
            bool b = stk.pop(), a = stk.pop(); stk.push(a || b);
        }
    }
    return stk.empty() ? false : stk.pop();
}

#endif // DS_H
