#include <bits/stdc++.h>
using namespace std;

struct Submission {char prob; string status; int time;};

struct ProbState {
    bool solved=false; int firstAcceptTime=0; int wrongBeforeAccept=0; vector<Submission> subs; bool frozen=false; int wrongBeforeFreeze=0; int afterFreeze=0;
};

struct Team {
    string name; map<char, ProbState> probs; int solved=0; long long penalty=0; vector<int> solveTimes; // for tie-break
};

struct ScoreEntry {
    string name; int solved; long long penalty; vector<int> solveTimes;
};

struct Competition {
    bool started=false; int duration=0; int m=0; vector<string> teamOrderLex; // initial ranking
    unordered_map<string, Team> teams;
    vector<char> probList;
    bool frozen=false;
    vector<ScoreEntry> lastScoreboard; // after last FLUSH
};

static int statusPenalty(const string &st){
    return st=="Accepted"?0:1; // indicator of wrong attempt
}

static void recomputeTeam(Team &t){
    t.solved=0; t.penalty=0; t.solveTimes.clear();
    for(auto &kv: t.probs){
        auto &ps=kv.second;
        if(ps.solved){
            t.solved++;
            t.penalty += 20LL*ps.wrongBeforeAccept + ps.firstAcceptTime;
            t.solveTimes.push_back(ps.firstAcceptTime);
        }
    }
    sort(t.solveTimes.begin(), t.solveTimes.end(), greater<int>());
}

static bool rankLess(const ScoreEntry &a, const ScoreEntry &b){
    if(a.solved!=b.solved) return a.solved>b.solved;
    if(a.penalty!=b.penalty) return a.penalty<b.penalty;
    // compare solveTimes lexicographically with missing treated as -inf? Problem states compare max, then second, etc; fewer elements means smaller? If both equal length compare each, else if one shorter, treat missing as 0 (since times positive), so shorter is smaller -> ranks higher.
    size_t na=a.solveTimes.size(), nb=b.solveTimes.size();
    for(size_t i=0;i<max(na,nb);++i){
        int va = (i<na? a.solveTimes[i]: 0);
        int vb = (i<nb? b.solveTimes[i]: 0);
        if(va!=vb) return va<vb;
    }
    return a.name<b.name;
}

static vector<ScoreEntry> buildScoreboard(Competition &comp){
    vector<ScoreEntry> sb;
    sb.reserve(comp.teams.size());
    for(auto &kv: comp.teams){
        Team &t=kv.second; recomputeTeam(t);
        ScoreEntry e{t.name,t.solved,t.penalty,t.solveTimes};
        sb.push_back(move(e));
    }
    sort(sb.begin(), sb.end(), rankLess);
    return sb;
}

static unordered_map<string,int> rankingIndex(const vector<ScoreEntry>& sb){
    unordered_map<string,int> idx; idx.reserve(sb.size()*2);
    for(size_t i=0;i<sb.size();++i) idx[sb[i].name]=(int)i+1; // ranking starts at 1
    return idx;
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

static void printScoreboard(Competition &comp, const vector<ScoreEntry> &sb){
    auto idx = rankingIndex(sb);
    // prepare per-team ranking number
    unordered_map<string,int> rk; for(auto &kv: idx) rk[kv.first]=kv.second;
    // output each team line ordered by ranking
    vector<pair<int,string>> order; order.reserve(comp.teams.size());
    for(auto &kv: comp.teams) order.push_back({rk[kv.first], kv.first});
    sort(order.begin(), order.end());
    for(auto &p: order){
        Team &t=comp.teams[p.second];
        cout<<t.name<<" "<<p.first<<" "<<t.solved<<" "<<t.penalty;
        for(char c: comp.probList){
            auto it=t.probs.find(c);
            ProbState ps;
            if(it!=t.probs.end()) ps=it->second;
            else{
                ps=ProbState();
                if(comp.frozen) ps.frozen=false, ps.wrongBeforeFreeze=0, ps.afterFreeze=0; // default
            }
            cout<<" "<<probCell(ps);
        }
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
            Team t; t.name=name; comp.teams[name]=t; comp.teamOrderLex.push_back(name);
            cout<<"[Info]Add successfully.\n";
        }else if(cmd=="START"){
            string DURATION, PROBLEM; int duration, pc; cin>>DURATION>>duration>>PROBLEM>>pc;
            if(comp.started){ cout<<"[Error]Start failed: competition has started.\n"; continue; }
            comp.started=true; comp.duration=duration; comp.m=pc; comp.probList.clear();
            for(int i=0;i<pc;i++) comp.probList.push_back(char('A'+i));
            sort(comp.teamOrderLex.begin(), comp.teamOrderLex.end());
            cout<<"[Info]Competition starts.\n";
        }else if(cmd=="SUBMIT"){
            string prob, BY, team, WITH, status, AT; int time; cin>>prob>>BY>>team>>WITH>>status>>AT>>time;
            char p = prob[0];
            auto &t = comp.teams[team];
            auto &ps = t.probs[p];
            // determine whether counts contribute now
            // If already solved before freeze, later subs don't change state
            bool isWrong = status!="Accepted";
            if(!comp.frozen){
                // live period: if not solved yet
                if(!ps.solved){
                    if(status=="Accepted"){
                        ps.solved=true; ps.firstAcceptTime=time;
                    }else{
                        ps.wrongBeforeAccept++;
                    }
                }
            }else{
                // frozen period: only affect frozen display for problems not solved before freeze
                if(!ps.solved){
                    ps.afterFreeze++;
                }
            }
            // record submission
            ps.subs.push_back({p,status,time});
        }else if(cmd=="FLUSH"){
            // recompute scoreboard based on non-frozen solved problems
            comp.lastScoreboard = buildScoreboard(comp);
            cout<<"[Info]Flush scoreboard.\n";
        }else if(cmd=="FREEZE"){
            if(comp.frozen){ cout<<"[Error]Freeze failed: scoreboard has been frozen.\n"; continue; }
            // Mark frozen problems per team: any problem not solved yet becomes frozen; wrongBeforeFreeze equals current wrongBeforeAccept; afterFreeze reset counts
            for(auto &kv: comp.teams){
                for(char c: comp.probList){
                    auto &ps = kv.second.probs[c];
                    if(!ps.solved){
                        ps.frozen=true; ps.wrongBeforeFreeze=ps.wrongBeforeAccept; // keep
                        // afterFreeze remains the count collected during frozen submissions
                        // ensure exists
                    }else{
                        ps.frozen=false; // solved not frozen
                    }
                }
            }
            comp.frozen=true;
            cout<<"[Info]Freeze scoreboard.\n";
        }else if(cmd=="SCROLL"){
            if(!comp.frozen){ cout<<"[Error]Scroll failed: scoreboard has not been frozen.\n"; continue; }
            cout<<"[Info]Scroll scoreboard.\n";
            // Before scrolling, flush first then output scoreboard
            comp.lastScoreboard = buildScoreboard(comp);
            printScoreboard(comp, comp.lastScoreboard);
            // Perform scrolling: repeatedly pick lowest-ranked team with frozen problems, unfreeze smallest problem id among its frozen problems, recompute; output ranking changes when occur
            // Helper to find lowest-ranked team with frozen problems
            auto hasFrozen = [&](const Team &t){
                for(char c: comp.probList){ auto it=t.probs.find(c); if(it!=t.probs.end() && it->second.frozen && !it->second.solved) return true; }
                return false;
            };
            auto smallestFrozenProb = [&](Team &t)->char{
                for(char c: comp.probList){ auto &ps=t.probs[c]; if(ps.frozen && !ps.solved) return c; }
                return 0;
            };
            vector<ScoreEntry> sb = comp.lastScoreboard;
            auto idx = rankingIndex(sb);
            while(true){
                // pick lowest-ranked with frozen
                string chosen=""; int chosenRank=0;
                for(auto &e: sb){ Team &t=comp.teams[e.name]; if(hasFrozen(t)){ chosen=e.name; chosenRank=idx[e.name]; break; } }
                if(chosen.empty()) break;
                Team &t = comp.teams[chosen];
                char c = smallestFrozenProb(t);
                if(c==0){
                    // no frozen? should not happen
                    // remove from consideration
                    idx[chosen]=INT_MAX; // skip
                    continue;
                }
                // Unfreeze c: evaluate submissions of c that happened during freeze: if any Accepted among frozen subs, treat first Accept time as time of that Accepted, wrongBeforeAccept equals wrongBeforeFreeze + wrongs before that Accept; else remain unsolved but frozen cleared
                ProbState &ps = t.probs[c];
                // Find first Accepted after freezing in ps.subs in order; we don't store freeze time, but problem statement says scroll uses current frozen counts; assume subs during freeze appear after FREEZE. We'll scan subs and count those after freeze via afterFreeze and assume the last afterFreeze submissions are exactly the tail; we cannot differentiate times, but correct behavior: if any Accepted occurred during freeze, mark solved with first such time.
                // To approximate, scan from end counting afterFreeze and inspect those submissions.
                int af = ps.afterFreeze;
                bool accepted=false; int firstAccTime=0; int wrongsAfter=0;
                for(int i=(int)ps.subs.size()-af;i<(int)ps.subs.size();++i){
                    const auto &s = ps.subs[i];
                    if(s.status=="Accepted"){ accepted=true; firstAccTime=s.time; break; }
                    else wrongsAfter++;
                }
                ps.frozen=false; // unfreeze this problem
                if(accepted){
                    ps.solved=true; ps.firstAcceptTime=firstAccTime; ps.wrongBeforeAccept = ps.wrongBeforeFreeze + wrongsAfter;
                }
                // recompute scoreboard and detect ranking change
                vector<ScoreEntry> newsb = buildScoreboard(comp);
                auto newIdx = rankingIndex(newsb);
                int newRank = newIdx[chosen];
                if(newRank < chosenRank){
                    // print one line change: chosen and the team previously at newRank (the one it replaced)
                    string replaced="";
                    for(auto &e: sb){ if(idx[e.name]==newRank) { replaced=e.name; break; } }
                    cout<<chosen<<" "<<replaced<<" "<<comp.teams[chosen].solved<<" "<<comp.teams[chosen].penalty<<"\n";
                }
                sb = move(newsb); idx = move(newIdx);
            }
            // After scrolling ends, output final scoreboard
            printScoreboard(comp, sb);
            comp.frozen=false; // frozen state lifted after scrolling
            // reset afterFreeze counts? Keep as part of history but frozen flags cleared
        }else if(cmd=="QUERY_RANKING"){
            string name; cin>>name;
            auto it = comp.teams.find(name);
            if(it==comp.teams.end()){ cout<<"[Error]Query ranking failed: cannot find the team.\n"; continue; }
            cout<<"[Info]Complete query ranking.\n";
            if(comp.frozen) cout<<"[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            vector<ScoreEntry> sb = comp.lastScoreboard.empty()? buildScoreboard(comp): comp.lastScoreboard;
            auto idx = rankingIndex(sb);
            int r = idx[name];
            cout<<name<<" NOW AT RANKING "<<r<<"\n";
        }else if(cmd=="QUERY_SUBMISSION"){
            string team, WHERE, PROBLEM, EQ1, problemFilter, AND, STATUS, EQ2, statusFilter;
            cin>>team>>WHERE>>PROBLEM>>EQ1>>problemFilter>>AND>>STATUS>>EQ2>>statusFilter;
            auto itTeam = comp.teams.find(team);
            if(itTeam==comp.teams.end()){ cout<<"[Error]Query submission failed: cannot find the team.\n"; continue; }
            cout<<"[Info]Complete query submission.\n";
            // Scan all submissions and find last satisfying
            Submission last; bool found=false;
            for(char c: comp.probList){
                if(problemFilter!="ALL" && c!=problemFilter[0]) continue;
                auto it = itTeam->second.probs.find(c);
                if(it==itTeam->second.probs.end()) continue;
                for(const auto &s: it->second.subs){
                    if(statusFilter!="ALL" && s.status!=statusFilter) continue;
                    if(problemFilter!="ALL" && s.prob!=problemFilter[0]) continue; // redundant
                    if(!found || s.time>=last.time){ last=s; found=true; }
                }
            }
            if(!found){ cout<<"Cannot find any submission.\n"; }
            else{
                cout<<team<<" "<<last.prob<<" "<<last.status<<" "<<last.time<<"\n";
            }
        }else if(cmd=="END"){
            cout<<"[Info]Competition ends.\n"; break;
        }else{
            // unknown command
            string rest; getline(cin,rest); // consume line
        }
    }
    return 0;
}
