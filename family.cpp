#include <fstream>      // reading/writing files
#include <sstream>      // splitting strings (istringstream)
#include <cstring>
#include <cmath>
#include <sys/stat.h>   // mkdir (make the data folder)
#include "ds.h"         // our data structures
#include "ui.h"         // our screen/input helpers

// A single comment on a post.
class Comment {
public:
    int authorId;       // who wrote it
    string text;        // the comment text
    string timestamp;   // when it was written
    Comment() : authorId(-1) {}
    Comment(int a, const string& t, const string& ts) : authorId(a), text(t), timestamp(ts) {}
};

// A single post.
class Post {
public:
    int id;             // unique number for this post
    int authorId;       // who posted it
    string title;
    string body;
    int likes;          // how many likes
    string timestamp;   // readable date/time
    bool deleted;       // true = hidden/removed
    int privacy;        // 0 = public, 1 = friends only
    LinkedList<int>     likedBy;    // list of user ids who liked it
    LinkedList<Comment> comments;   // the comment thread
    DoublyLinkedList<Post*>::Node* feedNode;  // this post's spot in the feed (for instant delete)
    long long timeVal;  // raw time number, used for sorting newest-first

    Post() : id(-1), authorId(-1), likes(0), deleted(false), privacy(0),
             feedNode(nullptr), timeVal(0) {}
};

// A pending friend request.
class FriendRequest {
public:
    int fromId;          // who sent it
    string fromName;
    FriendRequest() : fromId(-1) {}
    FriendRequest(int id, const string& n) : fromId(id), fromName(n) {}
};

// A user account.
class User {
public:
    int id;
    string username;    // unique login name (no spaces)
    string displayName; // shown name
    string password;
    string bio;
    Queue<FriendRequest> friendRequests;    // incoming requests, handled oldest-first
    Queue<string> notifications;            // messages like "X liked your post"
    User() : id(-1) {}
};

// Main Global Classess to hold the program's states..
Array<User> users;              // all users, found by their id
DoublyLinkedList<Post*> feed;   // all posts, in the order they were made
Graph social;                   // the friendship network
BST userDir;                    // username -> id lookup (fast + sorted)
Deque<string> actLog;           // recent activity log (keeps only the latest few)

int postCounter = 0;            // next post id to hand out
int currentUser = -1;           // who is logged in (-1 means nobody)

// escaping the | character for safety
static string esc(const string& s) {
    string r;
    for (char c : s) { if (c=='|') r+="\\|"; else if (c=='\\') r+="\\\\"; else r+=c; }
    return r;
}
// ...and unesc removes them when reading back.
static string unesc(const string& s) {
    string r; bool e=false;
    for (char c : s) { if (e) { r+=c; e=false; } else if (c=='\\') e=true; else r+=c; }
    return r;
}

// Split a line by '|' into pieces (up to mx pieces)
static Array<string> splitPipe(const string& s, int mx=99) {
    Array<string> p; string cur; bool e=false;
    for (size_t i=0;i<s.size();i++) {
        char c=s[i];
        if (e) { cur+=c; e=false; }                 // previous char was '\', take this one literally
        else if (c=='\\') e=true;                    // start an escape
        else if (c=='|' && (int)p.size()<mx-1) { p.pushBack(cur); cur.clear(); }  // field separator
        else cur+=c;
    }
    p.pushBack(cur); return p;                        // last piece
}

// Make the "data" folder if it doesn't exist yet
static void ensureDir() { mkdir("data", 0755); }

// Save all users to data/users.txt (one line each)
static void saveUsers() {
    ensureDir(); ofstream f("data/users.txt");
    for (int i=0;i<users.size();i++) {
        User& u=users[i];
        f << u.id << "|" << esc(u.username) << "|"
          << esc(u.displayName) << "|" << esc(u.password) << "|" << esc(u.bio) << "\n";
    }
}
// Load users from the file back into memory and rebuild the username BST
static void loadUsers() {
    ifstream f("data/users.txt"); if (!f) return;
    string line;
    while (getline(f,line)) {
        if (line.empty()) continue;
        Array<string> p=splitPipe(line,5); if (p.size()<5) continue;  // expect 5 fields
        User u;
        u.id=stoi(p[0]); u.username=unesc(p[1]);
        u.displayName=unesc(p[2]); u.password=unesc(p[3]); u.bio=unesc(p[4]);
        while (users.size()<=u.id) { User e; users.pushBack(e); }      // grow array so index == id
        users[u.id]=u;
        userDir.insert(u.username, u.id);                              // add to the lookup tree
    }
}

// Save all posts with their likes and comments to data/posts.txt
static void savePosts() {
    ensureDir(); ofstream f("data/posts.txt");
    DoublyLinkedList<Post*>::Node* cur=feed.getHead();
    while (cur) {
        Post* p=cur->data;
        if (!p->deleted) {
            // line 1: the post's main info
            f << p->id << "|" << p->authorId << "|" << p->likes << "|"
              << p->privacy << "|" << p->timeVal << "|"
              << esc(p->timestamp) << "|" << esc(p->title) << "|" << esc(p->body) << "\n";
            // line 2: ids of everyone who liked it
            LinkedList<int>::Node* lb=p->likedBy.getHead();
            while (lb) { f<<lb->data<<" "; lb=lb->next; } f<<"\n";
            // line 3: number of comments, then one line per comment
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
// Load posts back from the file and rebuild the feed.
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
        if (post->id>=postCounter) postCounter=post->id+1;   // keep new ids unique
        // next line: the likes
        string lbLine; getline(f,lbLine);
        istringstream lbss(lbLine); int uid;
        while (lbss>>uid) post->likedBy.pushBack(uid);
        // next line: comment count, then that many comment lines
        string ccLine; getline(f,ccLine);
        int cc=0; try{ cc=stoi(ccLine); }catch(...) {}
        for (int i=0;i<cc;i++) {
            string cl; getline(f,cl);
            Array<string> cp=splitPipe(cl,3); if (cp.size()<3) continue;
            Comment c(stoi(cp[0]),unesc(cp[2]),unesc(cp[1]));
            post->comments.pushBack(c);
        }
        post->feedNode=feed.pushBack(post);   // add to the feed, remember its node
    }
}

// Save the friendship graph
static void saveFriends() {
    ensureDir(); ofstream f("data/friends.txt");
    for (int i=0;i<users.size();i++) { f<<i<<":"; social.saveEdges(i,f); }
}
// Load friendships back into the graph
static void loadFriends() {
    ifstream f("data/friends.txt"); if (!f) return;
    string line;
    while (getline(f,line)) {
        if (line.empty()) continue;
        size_t c=line.find(':'); if (c==string::npos) continue;
        int uid=stoi(line.substr(0,c));
        social.ensureVertex(uid);
        istringstream ss(line.substr(c+1)); int fid;
        // each friendship is written twice (from both sides); only add it once (fid>uid)
        while (ss>>fid) { if (fid>uid) social.addEdge(uid,fid); }
    }
}
// Save everything at once
static void saveAll() { saveUsers(); savePosts(); saveFriends(); }

// Get a user's username from their id
const string& nameOf(int id) {
    static const string uk="?"; // return ? if unknown
    return (id>=0&&id<users.size()) ? users[id].username : uk;
}
// Get a user's display name from their id
const string& dispOf(int id) {
    static const string uk="?";
    return (id>=0&&id<users.size()) ? users[id].displayName : uk;
}
// Has this user already liked this post??
bool hasLiked(Post* p, int uid) {
    LinkedList<int>::Node* c=p->likedBy.getHead();
    while (c) { if (c->data==uid) return true; c=c->next; }
    return false;
}
// Can the current user SEE this post? (privacy check, used everywhere)
bool canSee(Post* p) {
    if (p->privacy==0) return true;                 // public: everyone
    if (currentUser==-1) return false;              // friends-only + nobody logged in: no
    if (p->authorId==currentUser) return true;      // it's your own post: yes
    return social.areFriends(currentUser, p->authorId);  // otherwise: only if you're friends
}
// Add a notification message to a user's queue.
void pushNotif(int toUser, const string& msg) {
    if (toUser>=0&&toUser<users.size())
        users[toUser].notifications.enqueue(msg);
}

// A sortable key used to order the feed by time (newest first).
class PostTimeKey {
public:
    long long timeVal; int id; Post* ptr;
    bool operator<(const PostTimeKey& o) const {
        return timeVal != o.timeVal ? timeVal < o.timeVal : id < o.id;
    }
    bool operator==(const PostTimeKey& o) const { return id==o.id; }
};

// Build the feed ordered newest-first, using QuickSort.
Array<Post*> buildFeedByTime() {
    Array<PostTimeKey> keys;
    DoublyLinkedList<Post*>::Node* cur=feed.getHead();
    while (cur) {
        Post* p=cur->data;
        if (!p->deleted && canSee(p)) {            // only visible posts
            PostTimeKey k; k.timeVal=p->timeVal; k.id=p->id; k.ptr=p;
            keys.pushBack(k);
        }
        cur=cur->next;
    }
    Sorter<PostTimeKey>::quickSort(keys, true);    // true = biggest time first (newest)
    Array<Post*> res;
    for (int i=0;i<keys.size();i++) res.pushBack(keys[i].ptr);
    return res;
}

// Build the feed ordered most-liked-first, using a MaxHeap.
Array<Post*> buildFeedByLikes() {
    MaxHeap heap;
    DoublyLinkedList<Post*>::Node* cur=feed.getHead();
    while (cur) {
        Post* p=cur->data;
        if (!p->deleted && canSee(p)) {
            RankedPost rp; rp.likes=p->likes; rp.postId=p->id;
            heap.insert(rp);                       // put every visible post in the heap
        }
        cur=cur->next;
    }
    // pull posts out of the heap biggest-first -> gives ids in like order
    Array<int> orderedIds;
    while (!heap.empty()) orderedIds.pushBack(heap.extractMax().postId);
    // turn those ids back into actual post pointers
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

// Print a big title banner.
void header(const string& title) {
    cout << "\n" << C::B << C::CYAN;
    rule('='); centred(title); rule('=');
    cout << C::R;
}
// Print a small sub-heading.
void sub(const string& t) {
    cout << "\n" << C::B << C::BLUE << "  ─── " << t << " ───" << C::R << "\n";
}

// Print one post nicely. idx = number to show. full = show body+comments in detail
void printPost(Post* p, int idx=-1, bool full=false) {
    if (!p) return;
    cout << "\n";
    if (idx>=0) cout << C::D << "  [" << idx << "] " << C::R;       // the [number]
    // author line: display name, @username, time, and a [friends] tag if private
    cout << C::B << C::MAG << dispOf(p->authorId) << C::R
         << C::D << " @" << nameOf(p->authorId)
         << "  " << p->timestamp << C::R
         << (p->privacy==1 ? (string)" " + C::YELLOW + "[friends]" + C::R : "") << "\n";
    cout << C::B << "      " << p->title << C::R << "\n";            // title
    // body: show fully if "full" or short; otherwise cut it short with "..."
    if (!p->body.empty() && (full || p->body.size()<=120))
        cout << "      " << p->body << "\n";
    else if (!p->body.empty() && !full)
        cout << "      " << p->body.substr(0,117) << "...\n";
    // like count + comment count + post id
    cout << "      "
         << C::RED << p->likes << " ♥" << C::R << "  "
         << C::BLUE << p->comments.size() << " 💬" << C::R
         << C::D << "  #" << p->id << C::R << "\n";
    // in full view, also list all the comments
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

// The top bar shown on every logged-in page (name + notification count + tabs).
void printNavbar() {
    User& u=users[currentUser];
    int notifCount=u.notifications.size()+u.friendRequests.size();  // total "new" things

    cout << "\n" << C::B << C::CYAN;
    rule('='); centred("F  A  M  I  L  Y"); rule('=');
    cout << C::R;
    cout << "  " << C::B << C::GREEN << u.displayName << C::R
         << C::D << " @" << u.username << C::R;
    if (notifCount>0) cout << "    " << C::YELLOW << "🔔 " << notifCount << C::R;  // bell badge
    cout << "\n";
    rule('-');
    // the four navigation tabs
    cout << C::B << C::YELLOW << "  tab1" << C::R << " Feed\n"
         << C::B << C::YELLOW << "  tab2" << C::R << " Friends\n"
         << C::B << C::YELLOW << "  tab3" << C::R << " Notifications"
         << (notifCount>0 ? string(" ") + C::YELLOW + "(" + to_string(notifCount) + " new)" + C::R : "") << "\n"
         << C::B << C::YELLOW << "  tab4" << C::R << " Settings\n";
    rule('=');
}

// Declared here so functions can call each other (definitions are below).
void feedPage();
void friendsPage();
void notificationsPage();
void settingsPage();
void postDetail(Post* p);

// Make a lowercase copy of a string (for case-insensitive comparison).
static string toLowerStr(const string& s) {
    string r=s; for (auto& c : r) c = tolower(c); return r;
}

// If the user typed tab1-tab4, jump to that page. Returns true if it handled it.
bool handleNavbarChoice(const string& chRaw) {
    string ch=toLowerStr(chRaw);
    if (ch=="tab1") { feedPage(); return true; }
    if (ch=="tab2") { friendsPage(); return true; }
    if (ch=="tab3") { notificationsPage(); return true; }
    if (ch=="tab4") { settingsPage(); return true; }
    return false;
}

// The single-post screen: like, comment, or delete.
void postDetail(Post* p) {
    while (true) {
        clearScreen();
        header("POST");
        printPost(p,-1,true);                          // show post in full
        rule();
        bool liked = currentUser!=-1 && hasLiked(p,currentUser);  // did I already like it?
        bool own   = currentUser==p->authorId;                    // is it my post?
        cout << C::B << C::YELLOW << "  1" << C::R << ". " << (liked?"Unlike ♥":"Like ♥")
             << "    "
             << C::B << C::YELLOW << "  2" << C::R << ". Comment"
             << "    ";
        if (own) cout << C::B << C::RED << "  3" << C::R << ". Delete    ";  // delete only on own post
        cout << C::B << C::RED << "  0" << C::R << ". Back\n";
        rule();

        int ch=promptInt("Action: ");
        if (ch==0) break;                              // 0 = go back
        if (ch==1) {                                   // 1 = like / unlike
            if (currentUser==-1) { err("Log in first."); waitEnter(); continue; }
            if (liked) {
                // UNLIKE: rebuild the likedBy list without me in it
                LinkedList<int> tmp;
                LinkedList<int>::Node* c=p->likedBy.getHead();
                while (c) { if (c->data!=currentUser) tmp.pushBack(c->data); c=c->next; }
                p->likedBy.clear();
                LinkedList<int>::Node* c2=tmp.getHead();
                while (c2) { p->likedBy.pushBack(c2->data); c2=c2->next; }
                p->likes--; ok("Unliked.");
            } else {
                // LIKE: add me, bump count, notify the author, log it
                p->likedBy.pushBack(currentUser);
                p->likes++;
                pushNotif(p->authorId, nameOf(currentUser)+" liked your post: "+p->title);
                actLog.pushFront(nameOf(currentUser)+" liked #"+to_string(p->id));
                ok("Liked! ("+to_string(p->likes)+" likes)");
            }
            savePosts(); waitEnter();
        } else if (ch==2) {                            // 2 = add a comment
            if (currentUser==-1) { err("Log in first."); waitEnter(); continue; }
            string text=promptLine("Comment: ");
            if (text.empty()) { err("Empty."); waitEnter(); continue; }
            Comment c(currentUser,text,nowStamp());
            p->comments.pushBack(c);
            pushNotif(p->authorId, nameOf(currentUser)+" commented on: "+p->title);
            actLog.pushFront(nameOf(currentUser)+" commented on #"+to_string(p->id));
            savePosts(); ok("Comment added."); waitEnter();
        } else if (ch==3 && own) {                     // 3 = delete (own post only)
            p->deleted=true;
            feed.remove(p->feedNode);                  // instant removal using the saved node
            savePosts(); ok("Post deleted."); waitEnter(); break;
        } else { err("Invalid."); waitEnter(); }
    }
}

// The main Feed screen.
void feedPage() {
    bool byLikes=false;        // false = sort by time, true = sort by likes
    const int PAGE=5;          // show 5 posts per page
    int offset=0;              // which page we're on

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

        // get the posts in the chosen order
        Array<Post*> posts = byLikes ? buildFeedByLikes() : buildFeedByTime();

        if (posts.empty()) { info("No posts yet."); }
        else {
            // show one page (PAGE posts starting at offset)
            int shown=0;
            for (int i=offset; i<posts.size()&&shown<PAGE; i++,shown++)
                printPost(posts[i], i+1);
            rule('-');
            // "showing X-Y of Z" line + Prev/Next hints
            cout << C::D << "  " << (offset+1) << "-"
                 << (offset+PAGE<posts.size()?offset+PAGE:posts.size())
                 << " of " << posts.size() << C::R << "\n";
            if (offset>0) cout << C::B << C::YELLOW << "  [P]" << C::R << " Prev    ";
            if (offset+PAGE<posts.size()) cout << C::B << C::YELLOW << "  [N]" << C::R << " Next    ";
            cout << C::B << C::YELLOW << "  [#]" << C::R << " Open by number\n";
        }
        rule('=');

        string ch=promptLine("Choice: ");
        if (handleNavbarChoice(ch)) { offset=0; continue; }    // tab1-4 navigation
        if ((ch=="w"||ch=="W") && currentUser!=-1) {           // W = write a new post
            clearScreen(); header("NEW POST");
            string title=promptLine("Title: ");
            if (title.empty()) { err("Title required."); waitEnter(); continue; }
            string body=promptLine("Body (optional): ");
            cout << "  Privacy:  0 = Public  |  1 = Friends only\n";
            int priv=promptInt("Privacy [0]: ");
            if (priv!=1) priv=0;                               // anything but 1 means public
            Post* p=new Post();
            p->id=postCounter++;                               // give it the next id
            p->authorId=currentUser;
            p->title=title; p->body=body;
            p->privacy=priv;
            p->timestamp=nowStamp();
            p->timeVal=(long long)time(nullptr);
            p->feedNode=feed.pushBack(p);                      // add to feed, keep its node
            savePosts();
            actLog.pushFront(nameOf(currentUser)+" posted: "+title);
            ok("Posted! (#"+to_string(p->id)+")");
            waitEnter(); offset=0; continue;
        }
        if (ch=="s"||ch=="S") { byLikes=!byLikes; offset=0; continue; }   // S = switch sort
        if (ch=="n"||ch=="N") { if (offset+PAGE<posts.size()) offset+=PAGE; continue; } // N = next page
        if (ch=="p"||ch=="P") { if (offset>0) offset-=PAGE; continue; }   // P = previous page
        if (ch=="q"||ch=="Q") {                                          // Q = search posts
            clearScreen(); header("SEARCH POSTS");
            cout << C::D << "  Tip: Use AND / OR / () — e.g.  food AND (travel OR trip)\n" << C::R;
            string query=promptLine("Search query: ");
            if (query.empty()) continue;
            // run the boolean search: words/AND/OR -> postfix -> test each post
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
        // otherwise: the user typed a number to open a post
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

// Show someone's profile, and allow add/remove friend.
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
        // collect this user's posts, walking the feed backward (newest first)
        Array<Post*> theirPosts;
        DoublyLinkedList<Post*>::Node* cur=feed.getTail();
        while (cur) {
            Post* p=cur->data;
            if (!p->deleted && p->authorId==uid && canSee(p))
                theirPosts.pushBack(p);
            cur=cur->prev;
        }
        if (theirPosts.empty()) cout << C::D << "  (no visible posts)\n" << C::R;
        else for (int i=0;i<theirPosts.size()&&i<5;i++) printPost(theirPosts[i],i+1);  // show up to 5

        rule();
        bool isFriend=currentUser!=-1&&social.areFriends(currentUser,uid);
        bool isMe    =currentUser==uid;
        // show the right action button depending on the relationship
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
                social.removeEdge(currentUser,uid);          // unfriend
                saveFriends();
                ok("Removed "+u.username+" from friends.");
            } else {
                // send a friend request into their queue + notify them
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

// The Friends screen: your friends, suggestions (BFS), and network reach (DFS).
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
            int show=myFriends.size()<5?myFriends.size():5;   // show first 5
            for (int i=0;i<show;i++)
                cout << "  [" << (i+1) << "] " << C::B << C::MAG << dispOf(myFriends[i])
                     << C::R << C::D << " @" << nameOf(myFriends[i]) << C::R << "\n";
            if (myFriends.size()>5) {                          // option to see the rest
                string m=promptLine("  View more? [y/N]: ");
                if (m=="y"||m=="Y") {
                    for (int i=5;i<myFriends.size();i++)
                        cout << "  [" << (i+1) << "] " << C::B << C::MAG << dispOf(myFriends[i])
                             << C::R << C::D << " @" << nameOf(myFriends[i]) << C::R << "\n";
                    waitEnter();
                }
            }
        }

        // "People You May Know" via BFS (friends-of-friends + mutual counts)
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

        // how many people you can reach through friendships (DFS)
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
        if (ch=="0") {                                  // search a user by name
            clearScreen(); header("SEARCH USER");
            string name=promptLine("Username: ");
            int found=userDir.search(name);             // BST lookup
            if (found==-1) { err("User not found."); waitEnter(); }
            else viewProfile(found);
            continue;
        }
        if ((ch[0]=='s'||ch[0]=='S') && ch.size()>1) {  // "S2" = open suggestion #2
            try {
                int idx=stoi(ch.substr(1))-1;
                if (idx>=0&&idx<sugg.size()) viewProfile(sugg[idx]);
                else { err("Invalid."); waitEnter(); }
            } catch (...) { err("Invalid."); waitEnter(); }
            continue;
        }
        // plain number = open one of my friends
        try {
            int idx=stoi(ch)-1;
            if (idx>=0&&idx<myFriends.size()) viewProfile(myFriends[idx]);
            else { err("Invalid."); waitEnter(); }
        } catch (...) { err("Invalid."); waitEnter(); }
    }
}

// The Notifications screen: handle friend requests + read notifications + activity
void notificationsPage() {
    if (currentUser==-1) { err("Log in first."); waitEnter(); return; }
    clearScreen();
    printNavbar();
    header("NOTIFICATIONS");

    bool hadReq=false;
    Queue<FriendRequest> kept;
    // go through each friend request in arrival order (FIFO) and accept/decline
    while (!users[currentUser].friendRequests.empty()) {
        FriendRequest req=users[currentUser].friendRequests.dequeue();
        hadReq=true;
        cout << C::B << C::CYAN << "  Friend Request from " << req.fromName << C::R << "\n";
        string ans=promptLine("  Accept? [y/N]: ");
        if (ans=="y"||ans=="Y") {
            social.addEdge(currentUser, req.fromId);    // become friends
            saveFriends();
            pushNotif(req.fromId, nameOf(currentUser)+" accepted your friend request!");
            ok("You are now friends with "+req.fromName+".");
        } else {
            ok("Request from "+req.fromName+" declined.");
        }
    }
    if (!hadReq) info("No pending friend requests.");

    sub("Notifications");
    // read out and clear all notification messages (FIFO order)
    if (users[currentUser].notifications.empty())
        cout << C::D << "  (none)\n" << C::R;
    else {
        while (!users[currentUser].notifications.empty())
            cout << "  " << C::GREEN << "▶ " << C::R
                 << users[currentUser].notifications.dequeue() << "\n";
    }

    sub("Recent Activity");
    actLog.printAll(users[currentUser].username);   // recent activity from the deque
    rule();
    waitEnter();
}

// The Settings screen: edit profile, change password, logout.
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
             << C::B << C::RED  << "  3" << C::R << ". Logout\n";
        rule();

        string chStr=promptLine("Choice: ");
        if (handleNavbarChoice(chStr)) continue;
        int ch; try { ch=stoi(chStr); } catch (...) { ch=-9999; }
        if (ch==1) {                // edit display name + bio
            clearScreen(); header("EDIT PROFILE");
            cout << C::D << "  Leave blank to keep current.\n" << C::R;
            string nd=promptLine("Display name ["+u.displayName+"]: ");
            if (!nd.empty()) u.displayName=nd;
            string nb=promptLine("Bio ["+u.bio+"]: ");
            if (!nb.empty()) u.bio=nb;
            saveUsers(); ok("Profile updated."); waitEnter();
        } else if (ch==2) {         // change password (with checks)
            clearScreen(); header("CHANGE PASSWORD");
            string old=readPassword("Current password: ");
            if (old!=u.password) { err("Incorrect password."); waitEnter(); continue; }
            string np=readPassword("New password: ");
            if (np.empty()) { err("Password cannot be empty."); waitEnter(); continue; }
            string nc=readPassword("Confirm: ");
            if (nc!=np) { err("Passwords don't match."); waitEnter(); continue; }
            u.password=np; saveUsers(); ok("Password changed."); waitEnter();
        } else if (ch==3) {            // logout
            actLog.pushFront(nameOf(currentUser)+" logged out");
            ok("Goodbye, "+u.displayName+"!");
            currentUser=-1; waitEnter(); break;
        }
    }
}

// The first screen (before logging in): Register / Login / Exit.
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
             << C::B << C::RED << "  0" << C::R << ". Exit\n";
        rule('=');

        int ch=promptInt("Choice: ");
        if (ch==0) { saveAll(); cout << C::B << C::CYAN << "\nGoodbye!\n" << C::R; exit(0); }  // save & quit
        if (ch==1) {                // Register
            clearScreen(); header("REGISTER");
            User u;
            u.username=promptLine("Username (no spaces): ");
            if (u.username.empty()||u.username.find(' ')!=string::npos)
                { err("Invalid username."); waitEnter(); continue; }
            if (userDir.search(u.username)!=-1)         // username must be unique
                { err("Username already taken."); waitEnter(); continue; }
            u.displayName=promptLine("Display name: ");
            if (u.displayName.empty()) u.displayName=u.username;
            u.bio=promptLine("Bio (optional): ");
            u.password=readPassword("Password: ");
            if (u.password.empty()) { err("Empty password."); waitEnter(); continue; }
            string conf=readPassword("Confirm password: ");
            if (conf!=u.password) { err("Passwords don't match."); waitEnter(); continue; }
            u.id=users.size();      // new id = current count
            users.pushBack(u);
            userDir.insert(u.username, u.id);       // add to lookup tree
            social.ensureVertex(u.id);      // add to friendship graph
            saveUsers();
            actLog.pushFront(u.username+" joined FAMILY");
            ok("Welcome to FAMILY, "+u.displayName+"!");
            currentUser=u.id;       // log  in
            waitEnter();
        } else if (ch==2) {  // Login
            clearScreen(); header("LOGIN");
            string name=promptLine("Username: ");
            int id=userDir.search(name);    // find the user id by name
            if (id==-1) { err("No such user."); waitEnter(); continue; }
            string pass=readPassword("Password: ");
            if (users[id].password!=pass) { err("Incorrect password."); waitEnter(); continue; }
            currentUser=id; // logged in
            actLog.pushFront(nameOf(id)+" logged in");
            ok("Welcome back, "+dispOf(id)+"!");
            if (!users[id].notifications.empty()||!users[id].friendRequests.empty())
                warn("You have pending notifications!");
            waitEnter();
        }
    }
}

// Once logged in, keep showing the feed until the user logs out.
void mainApp() {
    while (currentUser!=-1) {
        feedPage();
    }
}

// Program starts here.
int main() {
    ensureDir();    // make sure the data folder exists
    loadUsers();    // load saved users
    loadPosts();    // load saved posts
    loadFriends();  // load saved friendships

    while (true) {
        if (currentUser==-1) preLoginMenu();   // not logged in -> show login menu
        if (currentUser!=-1) mainApp();        // logged in -> run the app
    }
    return 0;
}
