/*Files:
ds.h - pure data structure header (Array, LinkedList, DoublyLinkedList, Deque, Stack, Queue, Graph, BST, MaxHeap, Sorter, boolean search evaluator)
ui.h - terminal UI helpers (ANSI, termios, prompts)
family.cpp - domain structs, app state, persistence, logic
*/

#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <sys/stat.h>
#include "ds.h"
#include "ui.h"

//Domain structs
struct Comment {
    int authorId;
    string text;
    string timestamp;
    Comment() : authorId(-1) {}
    Comment(int a, const string& t, const string& ts) : authorId(a), text(t), timestamp(ts) {}
};

struct Post {
    int id;
    int authorId;
    string title;
    string body;
    int likes;
    string timestamp;
    bool deleted;
    int privacy;
    LinkedList<int>     likedBy;
    LinkedList<Comment> comments;
    DoublyLinkedList<Post*>::Node* feedNode;
    long long timeVal;

    Post() : id(-1), authorId(-1), likes(0), deleted(false), privacy(0),
             feedNode(nullptr), timeVal(0) {}
};

struct FriendRequest {
    int fromId;
    string fromName;
    FriendRequest() : fromId(-1) {}
    FriendRequest(int id, const string& n) : fromId(id), fromName(n) {}
};

struct User {
    int id;
    string username;
    string displayName;
    string password;
    string bio;
    Queue<FriendRequest> friendRequests;
    Queue<string> notifications;
    User() : id(-1) {}
};

//App state
Array<User> users;
DoublyLinkedList<Post*> feed;
Graph social;
BST userDir;
Deque<string> actLog;

int postCounter = 0;
int currentUser = -1;

//Persistence helpers
static string esc(const string& s) {
    string r;
    for (char c : s) { if (c=='|') r+="\\|"; else if (c=='\\') r+="\\\\"; else r+=c; }
    return r;
}
static string unesc(const string& s) {
    string r; bool e=false;
    for (char c : s) { if (e) { r+=c; e=false; } else if (c=='\\') e=true; else r+=c; }
    return r;
}
static Array<string> splitPipe(const string& s, int mx=99) {
    Array<string> p; string cur; bool e=false;
    for (size_t i=0;i<s.size();i++) {
        char c=s[i];
        if (e) { cur+=c; e=false; }
        else if (c=='\\') e=true;
        else if (c=='|' && (int)p.size()<mx-1) { p.pushBack(cur); cur.clear(); }
        else cur+=c;
    }
    p.pushBack(cur); return p;
}

static void ensureDir() { mkdir("data", 0755); }

static void saveUsers() {
    ensureDir(); ofstream f("data/users.txt");
    for (int i=0;i<users.size();i++) {
        User& u=users[i];
        f << u.id << "|" << esc(u.username) << "|"
          << esc(u.displayName) << "|" << esc(u.password) << "|" << esc(u.bio) << "\n";
    }
}
static void loadUsers() {
    ifstream f("data/users.txt"); if (!f) return;
    string line;
    while (getline(f,line)) {
        if (line.empty()) continue;
        Array<string> p=splitPipe(line,5); if (p.size()<5) continue;
        User u;
        u.id=stoi(p[0]); u.username=unesc(p[1]);
        u.displayName=unesc(p[2]); u.password=unesc(p[3]); u.bio=unesc(p[4]);
        while (users.size()<=u.id) { User e; users.pushBack(e); }
        users[u.id]=u;
        userDir.insert(u.username, u.id);
    }
}
static void savePosts() {
    ensureDir(); ofstream f("data/posts.txt");
    DoublyLinkedList<Post*>::Node* cur=feed.getHead();
    while (cur) {
        Post* p=cur->data;
        if (!p->deleted) {
            f << p->id << "|" << p->authorId << "|" << p->likes << "|"
              << p->privacy << "|" << p->timeVal << "|"
              << esc(p->timestamp) << "|" << esc(p->title) << "|" << esc(p->body) << "\n";
            LinkedList<int>::Node* lb=p->likedBy.getHead();
            while (lb) { f<<lb->data<<" "; lb=lb->next; } f<<"\n";
            f<<p->comments.size()<<"\n";
            LinkedList<Comment>::Node* cc=p->comments.getHead();
            while (cc) {
                f<<cc->data.authorId<<"|"<<esc(cc->data.timestamp)<<"|"<<esc(cc->data.text)<<"\n";
                cc=cc->next;
            }
        }
        cur=cur->next;
    }
}
static void loadPosts() {
    ifstream f("data/posts.txt"); if (!f) return;
    string line;
    while (getline(f,line)) {
        if (line.empty()) continue;
        Array<string> p=splitPipe(line,8); if (p.size()<8) continue;
        Post* post=new Post();
        post->id=stoi(p[0]); post->authorId=stoi(p[1]);
        post->likes=stoi(p[2]); post->privacy=stoi(p[3]);
        post->timeVal=atoll(p[4].c_str());
        post->timestamp=unesc(p[5]); post->title=unesc(p[6]); post->body=unesc(p[7]);
        if (post->id>=postCounter) postCounter=post->id+1;
        string lbLine; getline(f,lbLine);
        istringstream lbss(lbLine); int uid;
        while (lbss>>uid) post->likedBy.pushBack(uid);
        string ccLine; getline(f,ccLine);
        int cc=0; try{ cc=stoi(ccLine); }catch(...) {}
        for (int i=0;i<cc;i++) {
            string cl; getline(f,cl);
            Array<string> cp=splitPipe(cl,3); if (cp.size()<3) continue;
            Comment c(stoi(cp[0]),unesc(cp[2]),unesc(cp[1]));
            post->comments.pushBack(c);
        }
        post->feedNode=feed.pushBack(post);
    }
}
static void saveFriends() {
    ensureDir(); ofstream f("data/friends.txt");
    for (int i=0;i<users.size();i++) { f<<i<<":"; social.saveEdges(i,f); }
}
static void loadFriends() {
    ifstream f("data/friends.txt"); if (!f) return;
    string line;
    while (getline(f,line)) {
        if (line.empty()) continue;
        size_t c=line.find(':'); if (c==string::npos) continue;
        int uid=stoi(line.substr(0,c));
        social.ensureVertex(uid);
        istringstream ss(line.substr(c+1)); int fid;
        while (ss>>fid) { if (fid>uid) social.addEdge(uid,fid); }
    }
}
static void saveAll() { saveUsers(); savePosts(); saveFriends(); }


//App helpers
const string& nameOf(int id) {
    static const string uk="?";
    return (id>=0&&id<users.size()) ? users[id].username : uk;
}
const string& dispOf(int id) {
    static const string uk="?";
    return (id>=0&&id<users.size()) ? users[id].displayName : uk;
}
bool hasLiked(Post* p, int uid) {
    LinkedList<int>::Node* c=p->likedBy.getHead();
    while (c) { if (c->data==uid) return true; c=c->next; }
    return false;
}
bool canSee(Post* p) {
    if (p->privacy==0) return true;
    if (currentUser==-1) return false;
    if (p->authorId==currentUser) return true;
    return social.areFriends(currentUser, p->authorId);
}
void pushNotif(int toUser, const string& msg) {
    if (toUser>=0&&toUser<users.size())
        users[toUser].notifications.enqueue(msg);
}


//Feed builders
struct PostTimeKey {
    long long timeVal; int id; Post* ptr;
    bool operator<(const PostTimeKey& o) const {
        return timeVal != o.timeVal ? timeVal < o.timeVal : id < o.id;
    }
    bool operator==(const PostTimeKey& o) const { return id==o.id; }
};

Array<Post*> buildFeedByTime() {
    Array<PostTimeKey> keys;
    DoublyLinkedList<Post*>::Node* cur=feed.getHead();
    while (cur) {
        Post* p=cur->data;
        if (!p->deleted && canSee(p)) {
            PostTimeKey k; k.timeVal=p->timeVal; k.id=p->id; k.ptr=p;
            keys.pushBack(k);
        }
        cur=cur->next;
    }
    Sorter<PostTimeKey>::quickSort(keys, true);
    Array<Post*> res;
    for (int i=0;i<keys.size();i++) res.pushBack(keys[i].ptr);
    return res;
}

Array<Post*> buildFeedByLikes() {
    MaxHeap heap;
    DoublyLinkedList<Post*>::Node* cur=feed.getHead();
    while (cur) {
        Post* p=cur->data;
        if (!p->deleted && canSee(p)) {
            RankedPost rp; rp.likes=p->likes; rp.postId=p->id;
            heap.insert(rp);
        }
        cur=cur->next;
    }
    Array<int> orderedIds;
    while (!heap.empty()) orderedIds.pushBack(heap.extractMax().postId);
    Array<Post*> res;
    for (int i=0;i<orderedIds.size();i++) {
        DoublyLinkedList<Post*>::Node* c=feed.getHead();
        while (c) {
            if (c->data->id==orderedIds[i]) { res.pushBack(c->data); break; }
            c=c->next;
        }
    }
    return res;
}


//UI sections
void header(const string& title) {
    cout << "\n" << C::B << C::CYAN;
    rule('='); centred(title); rule('=');
    cout << C::R;
}
void sub(const string& t) {
    cout << "\n" << C::B << C::BLUE << "  ─── " << t << " ───" << C::R << "\n";
}

void printPost(Post* p, int idx=-1, bool full=false) {
    if (!p) return;
    cout << "\n";
    if (idx>=0) cout << C::D << "  [" << idx << "] " << C::R;
    cout << C::B << C::MAG << dispOf(p->authorId) << C::R
         << C::D << " @" << nameOf(p->authorId)
         << "  " << p->timestamp << C::R
         << (p->privacy==1 ? (string)" " + C::YELLOW + "[friends]" + C::R : "") << "\n";
    cout << C::B << "      " << p->title << C::R << "\n";
    if (!p->body.empty() && (full || p->body.size()<=120))
        cout << "      " << p->body << "\n";
    else if (!p->body.empty() && !full)
        cout << "      " << p->body.substr(0,117) << "...\n";
    cout << "      "
         << C::RED << p->likes << " ♥" << C::R << "  "
         << C::BLUE << p->comments.size() << " 💬" << C::R
         << C::D << "  #" << p->id << C::R << "\n";
    if (full && !p->comments.empty()) {
        sub("Comments");
        LinkedList<Comment>::Node* c=p->comments.getHead();
        while (c) {
            cout << C::D << "    " << c->data.timestamp << "  " << C::R
                 << C::B << nameOf(c->data.authorId) << C::R << ": " << c->data.text << "\n";
            c=c->next;
        }
    }
}


//Persistent navbar
void printNavbar() {
    User& u=users[currentUser];
    int notifCount=u.notifications.size()+u.friendRequests.size();

    cout << "\n" << C::B << C::CYAN;
    rule('='); centred("F  A  M  I  L  Y"); rule('=');
    cout << C::R;
    cout << "  " << C::B << C::GREEN << u.displayName << C::R
         << C::D << " @" << u.username << C::R;
    if (notifCount>0) cout << "    " << C::YELLOW << "🔔 " << notifCount << C::R;
    cout << "\n";
    rule('-');
    cout << C::B << C::YELLOW << "  tab1" << C::R << " Feed\n"
         << C::B << C::YELLOW << "  tab2" << C::R << " Friends\n"
         << C::B << C::YELLOW << "  tab3" << C::R << " Notifications"
         << (notifCount>0 ? string(" ") + C::YELLOW + "(" + to_string(notifCount) + " new)" + C::R : "") << "\n"
         << C::B << C::YELLOW << "  tab4" << C::R << " Settings\n";
    rule('=');
}

//Forward declarations for tabs
void feedPage();
void friendsPage();
void notificationsPage();
void settingsPage();
void postDetail(Post* p);

static string toLowerStr(const string& s) {
    string r=s; for (auto& c : r) c = tolower(c); return r;
}


bool handleNavbarChoice(const string& chRaw) {
    string ch=toLowerStr(chRaw);
    if (ch=="tab1") { feedPage(); return true; }
    if (ch=="tab2") { friendsPage(); return true; }
    if (ch=="tab3") { notificationsPage(); return true; }
    if (ch=="tab4") { settingsPage(); return true; }
    return false;
}


//Post detail
void postDetail(Post* p) {
    while (true) {
        clearScreen();
        header("POST");
        printPost(p,-1,true);
        rule();
        bool liked = currentUser!=-1 && hasLiked(p,currentUser);
        bool own   = currentUser==p->authorId;
        cout << C::B << C::YELLOW << "  1" << C::R << ". " << (liked?"Unlike ♥":"Like ♥")
             << "    "
             << C::B << C::YELLOW << "  2" << C::R << ". Comment"
             << "    ";
        if (own) cout << C::B << C::RED << "  3" << C::R << ". Delete    ";
        cout << C::B << C::RED << "  0" << C::R << ". Back\n";
        rule();

        int ch=promptInt("Action: ");
        if (ch==0) break;
        if (ch==1) {
            if (currentUser==-1) { err("Log in first."); waitEnter(); continue; }
            if (liked) {
                LinkedList<int> tmp;
                LinkedList<int>::Node* c=p->likedBy.getHead();
                while (c) { if (c->data!=currentUser) tmp.pushBack(c->data); c=c->next; }
                p->likedBy.clear();
                LinkedList<int>::Node* c2=tmp.getHead();
                while (c2) { p->likedBy.pushBack(c2->data); c2=c2->next; }
                p->likes--; ok("Unliked.");
            } else {
                p->likedBy.pushBack(currentUser);
                p->likes++;
                pushNotif(p->authorId, nameOf(currentUser)+" liked your post: "+p->title);
                actLog.pushFront(nameOf(currentUser)+" liked #"+to_string(p->id));
                ok("Liked! ("+to_string(p->likes)+" likes)");
            }
            savePosts(); waitEnter();
        } else if (ch==2) {
            if (currentUser==-1) { err("Log in first."); waitEnter(); continue; }
            string text=promptLine("Comment: ");
            if (text.empty()) { err("Empty."); waitEnter(); continue; }
            Comment c(currentUser,text,nowStamp());
            p->comments.pushBack(c);
            pushNotif(p->authorId, nameOf(currentUser)+" commented on: "+p->title);
            actLog.pushFront(nameOf(currentUser)+" commented on #"+to_string(p->id));
            savePosts(); ok("Comment added."); waitEnter();
        } else if (ch==3 && own) {
            p->deleted=true;
            feed.remove(p->feedNode);
            savePosts(); ok("Post deleted."); waitEnter(); break;
        } else { err("Invalid."); waitEnter(); }
    }
}


//Feed Page
void feedPage() {
    bool byLikes=false;
    const int PAGE=5;
    int offset=0;

    while (currentUser!=-1) {
        clearScreen();
        printNavbar();
        cout << "\n" << C::B << C::CYAN;
        rule('=');
        centred("F E E D  —  " + string(byLikes?"Most Liked":"Most Recent"));
        rule('=');
        cout << C::R;

        if (currentUser!=-1)
            cout << C::B << C::GREEN << "  [W]" << C::R << " Write a post    ";
        cout << C::B << C::YELLOW << "  [S]" << C::R
             << " Sort: " << (byLikes?"switch to Recent":"switch to Most Liked")
             << "    " << C::B << C::YELLOW << "[Q]" << C::R << " Search posts\n";
        rule('-');

        Array<Post*> posts = byLikes ? buildFeedByLikes() : buildFeedByTime();

        if (posts.empty()) { info("No posts yet."); }
        else {
            int shown=0;
            for (int i=offset; i<posts.size()&&shown<PAGE; i++,shown++)
                printPost(posts[i], i+1);
            rule('-');
            cout << C::D << "  " << (offset+1) << "-"
                 << (offset+PAGE<posts.size()?offset+PAGE:posts.size())
                 << " of " << posts.size() << C::R << "\n";
            if (offset>0) cout << C::B << C::YELLOW << "  [P]" << C::R << " Prev    ";
            if (offset+PAGE<posts.size()) cout << C::B << C::YELLOW << "  [N]" << C::R << " Next    ";
            cout << C::B << C::YELLOW << "  [#]" << C::R << " Open by number\n";
        }
        rule('=');

        string ch=promptLine("Choice: ");
        if (handleNavbarChoice(ch)) { offset=0; continue; }
        if ((ch=="w"||ch=="W") && currentUser!=-1) {
            clearScreen(); header("NEW POST");
            string title=promptLine("Title: ");
            if (title.empty()) { err("Title required."); waitEnter(); continue; }
            string body=promptLine("Body (optional): ");
            cout << "  Privacy:  0 = Public  |  1 = Friends only\n";
            int priv=promptInt("Privacy [0]: ");
            if (priv!=1) priv=0;
            Post* p=new Post();
            p->id=postCounter++;
            p->authorId=currentUser;
            p->title=title; p->body=body;
            p->privacy=priv;
            p->timestamp=nowStamp();
            p->timeVal=(long long)time(nullptr);
            p->feedNode=feed.pushBack(p);
            savePosts();
            actLog.pushFront(nameOf(currentUser)+" posted: "+title);
            ok("Posted! (#"+to_string(p->id)+")");
            waitEnter(); offset=0; continue;
        }
        if (ch=="s"||ch=="S") { byLikes=!byLikes; offset=0; continue; }
        if (ch=="n"||ch=="N") { if (offset+PAGE<posts.size()) offset+=PAGE; continue; }
        if (ch=="p"||ch=="P") { if (offset>0) offset-=PAGE; continue; }
        if (ch=="q"||ch=="Q") {
            clearScreen(); header("SEARCH POSTS");
            cout << C::D << "  Tip: Use AND / OR / () — e.g.  food AND (travel OR trip)\n" << C::R;
            string query=promptLine("Search query: ");
            if (query.empty()) continue;
            Array<Token> toks=tokenise(query);
            Array<Token> pf=infixToPostfix(toks);
            Array<Post*> results;
            DoublyLinkedList<Post*>::Node* cur=feed.getHead();
            while (cur) {
                Post* pp=cur->data;
                if (!pp->deleted && canSee(pp)) {
                    string combined=pp->title+" "+pp->body;
                    if (evalPostfix(pf, combined)) results.pushBack(pp);
                }
                cur=cur->next;
            }
            clearScreen(); header("SEARCH RESULTS: "+query);
            if (results.empty()) info("No matching posts.");
            else for (int i=0;i<results.size();i++) printPost(results[i],i+1);
            rule();
            string sel=promptLine("Open by number or [0] to go back: ");
            if (sel!="0"&&!sel.empty()) {
                try {
                    int idx=stoi(sel)-1;
                    if (idx>=0&&idx<results.size()) {
                        postDetail(results[idx]);
                    }
                } catch (...) {}
            }
            continue;
        }
        try {
            int sel=stoi(ch)-1;
            if (sel<0||sel>=posts.size()) { err("Invalid number."); waitEnter(); continue; }
            actLog.pushFront(nameOf(currentUser==-1?0:currentUser)
                             +" viewed post #"+to_string(posts[sel]->id));
            postDetail(posts[sel]);
            offset=0;
        } catch (...) { err("Invalid input."); waitEnter(); }
    }
}


//Profile Page
void viewProfile(int uid) {
    if (uid<0||uid>=users.size()) { err("Invalid user."); waitEnter(); return; }
    while (true) {
        clearScreen();
        User& u=users[uid];
        header("PROFILE: @"+u.username);
        cout << C::B << C::MAG << "  " << u.displayName << C::R
             << C::D << " @" << u.username << C::R << "\n";
        if (!u.bio.empty()) cout << C::I << "  " << u.bio << C::R << "\n";
        Array<int> friends=social.getFriends(uid);
        cout << C::D << "  " << friends.size() << " friends" << C::R << "\n";

        sub("Posts");
        Array<Post*> theirPosts;
        DoublyLinkedList<Post*>::Node* cur=feed.getTail();
        while (cur) {
            Post* p=cur->data;
            if (!p->deleted && p->authorId==uid && canSee(p))
                theirPosts.pushBack(p);
            cur=cur->prev;
        }
        if (theirPosts.empty()) cout << C::D << "  (no visible posts)\n" << C::R;
        else for (int i=0;i<theirPosts.size()&&i<5;i++) printPost(theirPosts[i],i+1);

        rule();
        bool isFriend=currentUser!=-1&&social.areFriends(currentUser,uid);
        bool isMe    =currentUser==uid;
        if (!isMe && currentUser!=-1) {
            if (isFriend)
                cout << C::B << C::YELLOW << "  1" << C::R << ". Remove Friend    ";
            else
                cout << C::B << C::YELLOW << "  1" << C::R << ". Send Friend Request    ";
        }
        cout << C::B << C::RED << "  0" << C::R << ". Back\n";
        rule('=');

        int ch=promptInt("Action: ");
        if (ch==0) break;
        if (ch==1 && !isMe && currentUser!=-1) {
            if (isFriend) {
                social.removeEdge(currentUser,uid);
                saveFriends();
                ok("Removed "+u.username+" from friends.");
            } else {
                FriendRequest req(currentUser, nameOf(currentUser));
                users[uid].friendRequests.enqueue(req);
                pushNotif(uid, nameOf(currentUser)+" sent you a friend request!");
                actLog.pushFront(nameOf(currentUser)+" sent friend request to "+u.username);
                ok("Friend request sent!");
            }
            waitEnter();
        }
    }
}


//Friends Page
void friendsPage() {
    if (currentUser==-1) { err("Log in first."); waitEnter(); return; }
    while (currentUser!=-1) {
        clearScreen();
        printNavbar();
        header("CONNECTIONS");
        Array<int> myFriends=social.getFriends(currentUser);
        sub("My Friends ("+to_string(myFriends.size())+")");
        if (myFriends.empty()) cout << C::D << "  (none yet)\n" << C::R;
        else {
            int show=myFriends.size()<5?myFriends.size():5;
            for (int i=0;i<show;i++)
                cout << "  [" << (i+1) << "] " << C::B << C::MAG << dispOf(myFriends[i])
                     << C::R << C::D << " @" << nameOf(myFriends[i]) << C::R << "\n";
            if (myFriends.size()>5) {
                string m=promptLine("  View more? [y/N]: ");
                if (m=="y"||m=="Y") {
                    for (int i=5;i<myFriends.size();i++)
                        cout << "  [" << (i+1) << "] " << C::B << C::MAG << dispOf(myFriends[i])
                             << C::R << C::D << " @" << nameOf(myFriends[i]) << C::R << "\n";
                    waitEnter();
                }
            }
        }

        int mutuals[256]={};
        int resCount=0;
        Array<int> sugg=social.suggestFriendsBFS(currentUser, mutuals, resCount);
        sub("People You May Know");
        if (sugg.empty()) cout << C::D << "  (none right now)\n" << C::R;
        else {
            int show=sugg.size()<5?sugg.size():5;
            for (int i=0;i<show;i++)
                cout << "  [S" << (i+1) << "] " << C::B << C::CYAN << dispOf(sugg[i])
                     << C::R << C::D << " @" << nameOf(sugg[i])
                     << "  (" << mutuals[i] << " mutual)" << C::R << "\n";
        }

        Array<int> reachable=social.reachableDFS(currentUser);
        cout << "\n" << C::D << "  Your network reaches " << reachable.size()
             << " people (DFS connected component)." << C::R << "\n";

        rule();
        cout << C::B << C::YELLOW << "  0" << C::R << ". Search user    "
             << C::B << C::YELLOW << "  #" << C::R << ". View my friend by number    "
             << C::B << C::YELLOW << "  S#" << C::R << ". View suggestion\n";
        rule('=');

        string ch=promptLine("Choice: ");
        if (handleNavbarChoice(ch)) continue;
        if (ch.empty()) { err("Invalid."); waitEnter(); continue; }
        if (ch=="0") {
            clearScreen(); header("SEARCH USER");
            string name=promptLine("Username: ");
            int found=userDir.search(name);
            if (found==-1) { err("User not found."); waitEnter(); }
            else viewProfile(found);
            continue;
        }
        if ((ch[0]=='s'||ch[0]=='S') && ch.size()>1) {
            try {
                int idx=stoi(ch.substr(1))-1;
                if (idx>=0&&idx<sugg.size()) viewProfile(sugg[idx]);
                else { err("Invalid."); waitEnter(); }
            } catch (...) { err("Invalid."); waitEnter(); }
            continue;
        }
        try {
            int idx=stoi(ch)-1;
            if (idx>=0&&idx<myFriends.size()) viewProfile(myFriends[idx]);
            else { err("Invalid."); waitEnter(); }
        } catch (...) { err("Invalid."); waitEnter(); }
    }
}


//Notifications Page
void notificationsPage() {
    if (currentUser==-1) { err("Log in first."); waitEnter(); return; }
    clearScreen();
    printNavbar();
    header("NOTIFICATIONS");

    bool hadReq=false;
    Queue<FriendRequest> kept;
    while (!users[currentUser].friendRequests.empty()) {
        FriendRequest req=users[currentUser].friendRequests.dequeue();
        hadReq=true;
        cout << C::B << C::CYAN << "  Friend Request from " << req.fromName << C::R << "\n";
        string ans=promptLine("  Accept? [y/N]: ");
        if (ans=="y"||ans=="Y") {
            social.addEdge(currentUser, req.fromId);
            saveFriends();
            pushNotif(req.fromId, nameOf(currentUser)+" accepted your friend request!");
            ok("You are now friends with "+req.fromName+".");
        } else {
            ok("Request from "+req.fromName+" declined.");
        }
    }
    if (!hadReq) info("No pending friend requests.");

    sub("Notifications");
    if (users[currentUser].notifications.empty())
        cout << C::D << "  (none)\n" << C::R;
    else {
        while (!users[currentUser].notifications.empty())
            cout << "  " << C::GREEN << "▶ " << C::R
                 << users[currentUser].notifications.dequeue() << "\n";
    }

    sub("Recent Activity");
    actLog.printAll(users[currentUser].username);

    rule();
    waitEnter();
}


//Settings Page

void settingsPage() {
    if (currentUser==-1) { err("Log in first."); waitEnter(); return; }
    while (currentUser!=-1) {
        clearScreen();
        printNavbar();
        header("SETTINGS");
        User& u=users[currentUser];
        cout << "  " << C::B << C::MAG << u.displayName << C::R
             << C::D << " @" << u.username << C::R << "\n";
        if (!u.bio.empty()) cout << C::I << "  " << u.bio << C::R << "\n\n";
        cout << C::B << C::YELLOW << "  1" << C::R << ". Edit Profile (name & bio)\n"
             << C::B << C::YELLOW << "  2" << C::R << ". Change Password\n"
             << C::B << C::RED    << "  3" << C::R << ". Logout\n";
        rule();

        string chStr=promptLine("Choice: ");
        if (handleNavbarChoice(chStr)) continue;
        int ch; try { ch=stoi(chStr); } catch (...) { ch=-9999; }
        if (ch==1) {
            clearScreen(); header("EDIT PROFILE");
            cout << C::D << "  Leave blank to keep current.\n" << C::R;
            string nd=promptLine("Display name ["+u.displayName+"]: ");
            if (!nd.empty()) u.displayName=nd;
            string nb=promptLine("Bio ["+u.bio+"]: ");
            if (!nb.empty()) u.bio=nb;
            saveUsers(); ok("Profile updated."); waitEnter();
        } else if (ch==2) {
            clearScreen(); header("CHANGE PASSWORD");
            string old=readPassword("Current password: ");
            if (old!=u.password) { err("Incorrect password."); waitEnter(); continue; }
            string np=readPassword("New password: ");
            if (np.empty()) { err("Password cannot be empty."); waitEnter(); continue; }
            string nc=readPassword("Confirm: ");
            if (nc!=np) { err("Passwords don't match."); waitEnter(); continue; }
            u.password=np; saveUsers(); ok("Password changed."); waitEnter();
        } else if (ch==3) {
            actLog.pushFront(nameOf(currentUser)+" logged out");
            ok("Goodbye, "+u.displayName+"!");
            currentUser=-1; waitEnter(); break;
        }
    }
}


//Pre-login Menu
void preLoginMenu() {
    while (currentUser==-1) {
        clearScreen();
        cout << "\n" << C::B << C::CYAN;
        rule('=');
        centred("F  A  M  I  L  Y");
        centred("offline social network");
        rule('=');
        cout << C::R;
        cout << C::B << C::YELLOW << "  1" << C::R << ". Register\n"
             << C::B << C::YELLOW << "  2" << C::R << ". Login\n"
             << C::B << C::RED    << "  0" << C::R << ". Exit\n";
        rule('=');

        int ch=promptInt("Choice: ");
        if (ch==0) { saveAll(); cout << C::B << C::CYAN << "\nGoodbye!\n" << C::R; exit(0); }
        if (ch==1) {
            clearScreen(); header("REGISTER");
            User u;
            u.username=promptLine("Username (no spaces): ");
            if (u.username.empty()||u.username.find(' ')!=string::npos)
                { err("Invalid username."); waitEnter(); continue; }
            if (userDir.search(u.username)!=-1)
                { err("Username already taken."); waitEnter(); continue; }
            u.displayName=promptLine("Display name: ");
            if (u.displayName.empty()) u.displayName=u.username;
            u.bio=promptLine("Bio (optional): ");
            u.password=readPassword("Password: ");
            if (u.password.empty()) { err("Empty password."); waitEnter(); continue; }
            string conf=readPassword("Confirm password: ");
            if (conf!=u.password) { err("Passwords don't match."); waitEnter(); continue; }
            u.id=users.size();
            users.pushBack(u);
            userDir.insert(u.username, u.id);
            social.ensureVertex(u.id);
            saveUsers();
            actLog.pushFront(u.username+" joined FAMILY");
            ok("Welcome to FAMILY, "+u.displayName+"!");
            currentUser=u.id;
            waitEnter();
        } else if (ch==2) {
            clearScreen(); header("LOGIN");
            string name=promptLine("Username: ");
            int id=userDir.search(name);
            if (id==-1) { err("No such user."); waitEnter(); continue; }
            string pass=readPassword("Password: ");
            if (users[id].password!=pass) { err("Incorrect password."); waitEnter(); continue; }
            currentUser=id;
            actLog.pushFront(nameOf(id)+" logged in");
            ok("Welcome back, "+dispOf(id)+"!");
            if (!users[id].notifications.empty()||!users[id].friendRequests.empty())
                warn("You have pending notifications!");
            waitEnter();
        }
    }
}


void mainApp() {
    while (currentUser!=-1) {
        feedPage();
    }
}


//main
int main() {
    ensureDir();
    loadUsers();
    loadPosts();
    loadFriends();

    while (true) {
        if (currentUser==-1) preLoginMenu();
        if (currentUser!=-1) mainApp();
    }
    return 0;
}
