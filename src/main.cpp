#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace std;
using namespace __gnu_pbds;

struct Submission {char prob; string status; int time; int seq;};

struct ProbState {
    bool solved=false; int firstAcceptTime=0; int wrongBeforeAccept=0; vector<Submission> subs;
    bool frozen=false; int wrongBeforeFreeze=0; int afterFreeze=0;
};

struct Team {
    string name; array<ProbState,26> probs; int solved=0; long long penalty=0; vector<int> solveTimes; // desc order
};

static void insertSolveTime(vector<int> &v, int t){
    auto it = lower_bound(v.begin(), v.end(), t, greater<int>());
    v.insert(it, t);
}

struct Key {
    int solved; long long penalty; const vector<int>* solveTimes; string name;
};

struct ScoreCmp {
    bool operator()(const Key &a, const Key &b) const {
        if(a.solved!=b.solved) return a.solved>b.solved;
        if(a.penalty!=b.penalty) return a.penalty<b.penalty;
        const auto &va=*a.solveTimes, &vb=*b.solveTimes;
        size_t na=va.size(), nb=vb.size();
        for(size_t i=0;i<max(na,nb);++i){
            int xa = (i<na? va[i]: 0), xb = (i<nb? vb[i]: 0);
            if(xa!=xb) return xa<xb;
        }
        return a.name<b.name;
    }
};

using OrderTree = tree<Key, null_type, ScoreCmp, rb_tree_tag, tree_order_statistics_node_update>;

struct Competition {
    bool started=false; int duration=0; int m=0; vector<char> probList; int submitSeq=0; bool frozen=false;
    unordered_map<string, Team> teams;
    OrderTree board; OrderTree frozenBoard;
};

static Key buildKey(Team &t){ return Key{t.solved, t.penalty, &t.solveTimes, t.name}; }

static bool teamHasFrozen(const Team &t, const vector<char>& probList){
    for(char c: probList){ const auto &ps=t.probs[c-'A']; if(ps.frozen && !ps.solved) return true; }
    return false;
}

static string probCell(const ProbState &ps){
    if(ps.frozen){
        if(ps.wrongBeforeFreeze==0) return to_string(0)+"/"+to_string(ps.afterFreeze);
        else return string("-")+to_string(ps.wrongBeforeFreeze)+"/"+to_string(ps.afterFreeze);
    }
    if(ps.solved){
        if(ps.wrongBeforeAccept==0) return "+";
        else return string("+")+to_string(ps.wrongBeforeAccept);
    }
    if(ps.wrongBeforeAccept==0) return ".";
    return string("-")+to_string(ps.wrongBeforeAccept);
}

static void rebuildBoard(Competition &comp){
    comp.board.clear();
    for(auto &kv: comp.teams){ comp.board.insert(buildKey(kv.second)); }
}

static void printScoreboard(Competition &comp){
    int n = (int)comp.teams.size();
    vector<Key> ordered; ordered.reserve(n);
    for(int i=0;i<n;i++){ ordered.push_back(*comp.board.find_by_order(i)); }
    unordered_map<string,int> rank; rank.reserve(n*2);
    for(int i=0;i<n;i++) rank[ordered[i].name]=i+1;
    for(int i=0;i<n;i++){
        Team &t = comp.teams[ordered[i].name];
        cout<<t.name<<" "<<rank[t.name]<<" "<<t.solved<<" "<<t.penalty;
        for(char c: comp.probList){ cout<<" "<<probCell(t.probs[c-'A']); }
        cout<<"\n";
    }
}

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    Competition comp;
    string cmd;
    while(cin>>cmd){
        if(cmd=="ADDTEAM"){
            string name; cin>>name;
            if(comp.started){ cout<<"[Error]Add failed: competition has started.\n"; continue; }
            if(comp.teams.count(name)){ cout<<"[Error]Add failed: duplicated team name.\n"; continue; }
            Team t; t.name=name; comp.teams.emplace(name, move(t));
            cout<<"[Info]Add successfully.\n";
        }else if(cmd=="START"){
            string DURATION, PROBLEM; int duration, pc; cin>>DURATION>>duration>>PROBLEM>>pc;
            if(comp.started){ cout<<"[Error]Start failed: competition has started.\n"; continue; }
            comp.started=true; comp.duration=duration; comp.m=pc; comp.probList.clear();
            for(int i=0;i<pc;i++) comp.probList.push_back(char('A'+i));
            for(auto &kv: comp.teams){ kv.second.solveTimes.clear(); kv.second.solved=0; kv.second.penalty=0; }
            rebuildBoard(comp); // lex order via name tie-break
            cout<<"[Info]Competition starts.\n";
        }else if(cmd=="SUBMIT"){
            string prob, BY, teamName, WITH, status, AT; int time; cin>>prob>>BY>>teamName>>WITH>>status>>AT>>time;
            char p = prob[0]; Team &t = comp.teams[teamName]; ProbState &ps = t.probs[p-'A'];
            if(!comp.frozen){
                if(!ps.solved){ if(status=="Accepted"){ ps.solved=true; ps.firstAcceptTime=time; t.solved++; t.penalty += 20LL*ps.wrongBeforeAccept + time; insertSolveTime(t.solveTimes, time); } else { ps.wrongBeforeAccept++; } }
            }else{
                if(!ps.solved){ ps.afterFreeze++; }
            }
            comp.submitSeq++; ps.subs.push_back({p,status,time,comp.submitSeq});
        }else if(cmd=="FLUSH"){
            rebuildBoard(comp);
            cout<<"[Info]Flush scoreboard.\n";
        }else if(cmd=="FREEZE"){
            if(comp.frozen){ cout<<"[Error]Freeze failed: scoreboard has been frozen.\n"; continue; }
            for(auto &kv: comp.teams){ Team &t=kv.second; for(char c: comp.probList){ auto &ps=t.probs[c-'A']; if(!ps.solved){ ps.frozen=true; ps.wrongBeforeFreeze=ps.wrongBeforeAccept; ps.afterFreeze=0; } else ps.frozen=false; } }
            comp.frozenBoard.clear(); for(auto &kv: comp.teams){ if(teamHasFrozen(kv.second, comp.probList)) comp.frozenBoard.insert(buildKey(kv.second)); }
            comp.frozen=true; cout<<"[Info]Freeze scoreboard.\n";
        }else if(cmd=="SCROLL"){
            if(!comp.frozen){ cout<<"[Error]Scroll failed: scoreboard has not been frozen.\n"; continue; }
            cout<<"[Info]Scroll scoreboard.\n";
            if(comp.board.size()!=comp.teams.size()) rebuildBoard(comp);
            printScoreboard(comp);
            while(comp.frozenBoard.size()>0){
                Key lowest = *comp.frozenBoard.find_by_order((int)comp.frozenBoard.size()-1);
                Team &t = comp.teams[lowest.name];
                char chosenProb=0; for(char c: comp.probList){ auto &ps=t.probs[c-'A']; if(ps.frozen && !ps.solved){ chosenProb=c; break; } }
                if(chosenProb==0){ comp.frozenBoard.erase(buildKey(t)); continue; }
                ProbState &ps = t.probs[chosenProb-'A'];
                int af = ps.afterFreeze; bool accepted=false; int firstAccTime=0; int wrongsAfter=0;
                for(int i=(int)ps.subs.size()-af;i<(int)ps.subs.size();++i){ const auto &s=ps.subs[i]; if(s.status=="Accepted"){ accepted=true; firstAccTime=s.time; break; } else wrongsAfter++; }
                Key oldKey = buildKey(t); int oldPos = comp.board.order_of_key(oldKey);
                ps.frozen=false; ps.wrongBeforeAccept = ps.wrongBeforeFreeze + wrongsAfter; ps.afterFreeze=0;
                if(accepted){ ps.solved=true; ps.firstAcceptTime=firstAccTime; t.solved++; t.penalty += 20LL*wrongsAfter + firstAccTime; insertSolveTime(t.solveTimes, firstAccTime); }
                Key newKey = buildKey(t); int newPos = comp.board.order_of_key(newKey);
                if(newPos < oldPos){ Key replacedKey = *comp.board.find_by_order(newPos); cout<<t.name<<" "<<replacedKey.name<<" "<<t.solved<<" "<<t.penalty<<"\n"; }
                comp.board.erase(oldKey); comp.board.insert(newKey);
                comp.frozenBoard.erase(oldKey); if(teamHasFrozen(t, comp.probList)) comp.frozenBoard.insert(newKey);
            }
            printScoreboard(comp);
            comp.frozen=false;
        }else if(cmd=="QUERY_RANKING"){
            string name; cin>>name; auto it=comp.teams.find(name);
            if(it==comp.teams.end()){ cout<<"[Error]Query ranking failed: cannot find the team.\n"; continue; }
            cout<<"[Info]Complete query ranking.\n"; if(comp.frozen) cout<<"[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            if(comp.board.size()!=comp.teams.size()) rebuildBoard(comp);
            int r = comp.board.order_of_key(buildKey(it->second))+1; cout<<name<<" NOW AT RANKING "<<r<<"\n";
        }else if(cmd=="QUERY_SUBMISSION"){
            string team, WHERE, PROBLEM, EQ1, problemFilter, AND, STATUS, EQ2, statusFilter; cin>>team>>WHERE>>PROBLEM>>EQ1>>problemFilter>>AND>>STATUS>>EQ2>>statusFilter;
            auto itTeam=comp.teams.find(team); if(itTeam==comp.teams.end()){ cout<<"[Error]Query submission failed: cannot find the team.\n"; continue; }
            cout<<"[Info]Complete query submission.\n";
            Submission last; bool found=false;
            for(char c: comp.probList){ if(problemFilter!="ALL" && c!=problemFilter[0]) continue; const auto &ps=itTeam->second.probs[c-'A']; for(const auto &s: ps.subs){ if(statusFilter!="ALL" && s.status!=statusFilter) continue; if(problemFilter!="ALL" && s.prob!=problemFilter[0]) continue; if(!found || s.time>last.time || (s.time==last.time && s.seq>last.seq)){ last=s; found=true; } } }
            if(!found) cout<<"Cannot find any submission.\n"; else cout<<team<<" "<<last.prob<<" "<<last.status<<" "<<last.time<<"\n";
        }else if(cmd=="END"){
            cout<<"[Info]Competition ends.\n"; break;
        }else{ string rest; getline(cin,rest); }
    }
    return 0;
}
