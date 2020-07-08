/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include <unordered_map>
#include <algorithm>
#include "util.h"
#include <string>

using namespace std;

/*******************************/
/*   Global variable and enum  */
/*******************************/
//#define noprint
/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/

size_t
CirMgr::StrashKey(CirGate* g) const
{
    size_t x = g->_fanin1.first->_ID * 2 + g->_fanin1.second;
    size_t y = g->_fanin2.first->_ID * 2 + g->_fanin2.second;
    return 0.5*std::max(x,y)*(std::max(x,y)+1)+std::min(x,y);
}

// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
    unordered_map<size_t, CirGate*> Hash;
    int original_A = A;
    for (auto iter = _dfsList.begin(); iter != _dfsList.end(); iter++) {
        if ((*iter)->_TYPE == 'A') {
            auto n = Hash.find(StrashKey(*iter));
            if (n != Hash.end()) { // merge(*iter, n->second)
                (*iter)->_fanin1.first->_fanoutList.erase((*iter)->_ID);
                (*iter)->_fanin2.first->_fanoutList.erase((*iter)->_ID);
                for (auto it = (*iter)->_fanoutList.begin(); it != (*iter)->_fanoutList.end(); it++) {
                    if (it->second.first->_fanin1.first->_ID == (*iter)->_ID)
                        it->second.first->_fanin1.first = n->second;
                    if (it->second.first->_fanin2.first && it->second.first->_fanin2.first->_ID == (*iter)->_ID)
                        it->second.first->_fanin2.first = n->second;
                    if (it->second.first->_TYPE == 'O')
                        it->second.first->_INDEX = n->second->_ID * 2 + it->second.first->_fanin1.second;
                    n->second->_fanoutList.insert(*it);
                }
                A--;
                _gateList.erase((*iter)->_ID);
                _floatList.erase((*iter)->_ID);
                #ifndef noprint
                cout<<"Strashing: "<<n->second->_ID<<" merging "<<(*iter)->_ID<<"..."<<endl;
                #endif
                delete *iter; *iter = 0;
            }
            else { Hash.insert(make_pair(StrashKey(*iter),*iter)); }
        }
    }
    if (A != original_A) {
        _dfsList.clear();
        for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) { iter->second->_visited = false; }
        for (auto iter = _poList.begin(); iter != _poList.end(); iter++) { dfsTraversal(*iter); }
    }
}

void
CirMgr::fraig()
{
    if (_fecList.empty()) { return; }
    if (fraiged) { return; }
    SatSolver solver;
    solver.initialize();
    genProofModel(solver);
    int effort = 0;

    while (!_fecList.empty())
    {
        if (effort > 2000) { break; }
        int num = 0;
        vector<string> input;
        bool skip_zero  = false; // Waiting for simulation
        bool skip_first = false; // Waiting for simulation

        for (auto gate : _dfsList) { gate->_fraiged = false; }

        for (auto gate : _dfsList) {
            if (gate->_TYPE != 'A')  { continue; }
            if (!gate->_gatefecList) { continue; }
            if (gate->_fraiged)      { continue; } // Waiting for simulation
            if (gate->_removed)      { continue; } // Waiting for merge

            if (gate->_gatefecList->begin()->first == 0) {
                if (skip_zero) { continue; }
                if (!proof(solver, _gateList[0], gate)) { // UNSAT
                    _mergeList.push_back({_gateList[0], {gate, gate->_value}});
                    gate->_removed = true;
                    gate->_gatefecList->erase(gate->_ID);
                    gate->_gatefecList = 0;
                    effort = 0; }
                else { // SAT
                    string str;
                    num++;
                    for (auto input_gate : _piList) {
                        str += to_string(solver.getValue(input_gate->getVar())); }
                    input.push_back(str);
                    skip_zero = true;
                    effort++;
                    if (num == 64) { break; }
                }
                continue;
            }

            for (auto iter : *(gate->_gatefecList)) {
                if (skip_first) { iter.second.first->_fraiged = true; continue; }
                if (iter.second.first->_fraiged) { continue; }
                if (iter.first == gate->_ID) { continue; }
                if (!proof(solver, gate, iter.second.first)) { // UNSAT
                    // avoid merging self.fanin + self -> self because self.fanin.fanout = self
                    if (gate->_fanin1.first->_ID == iter.second.first->_ID) {
                        CirGate* temp = gate->_fanin1.first;
                        iter.second.first = gate;
                        gate = temp; }
                    else if (gate->_fanin2.first->_ID == iter.second.first->_ID) {
                        CirGate* temp = gate->_fanin2.first;
                        iter.second.first = gate;
                        gate = temp; }

                    _mergeList.push_back({gate, {iter.second.first, iter.second.first->_value != gate->_value}});
                    iter.second.first->_removed = true;
                    gate->_gatefecList->erase(iter.second.first->_ID);
                    iter.second.first->_gatefecList = 0;
                    effort = 0;
                }
                else {
                    string str;
                    num++;
                    for (auto input_gate : _piList)
                        str += to_string(solver.getValue(input_gate->getVar()));
                    input.push_back(str);
                    iter.second.first->_fraiged = true;
                    gate->_fraiged = true;
                    skip_first = true;
                    effort++;
                    if (num == 64) { goto sim; }
                }
            }
            skip_first = false;
        }
        sim:;
        if (!input.empty()) {
            int _size = satUpdate(input, input.size());
            #ifndef noprint
            cout<<"\33[2K\r"<<"Updating by SAT... Total #FEC Group = "<<_size<<endl;
            #endif
            input.clear(); }
        else { if (_fecList.size() != 0) { _fecList.clear(); } }
    }
    for (auto gate : _dfsList) { gate->_gatefecList = 0; } // for safety purpose
    mergeResult();
    for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) { // update _notusedList
        if (iter->second->_fanoutList.empty() && iter->second->_TYPE != 'O' && iter->second->_TYPE != 'C')
            _notusedList.insert(iter->second->_ID);
    }
    fraiged = true;
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirMgr::genProofModel(SatSolver& s)
{
    _gateList[0]->setVar(s.newVar());
    s.addAigCNF(_gateList[0]->getVar(), _gateList[0]->getVar(), 0, _gateList[0]->getVar(), 1); // CONST 0

    for (auto gate : _piList) { // Ensure pi cannot reach from po have Var, so fraig sat sim traverse pi list won't go wrong
        Var v = s.newVar();
        gate->setVar(v);
    }

    for (auto gate : _dfsList) {
        if (gate->_TYPE == 'A') {
            Var v = s.newVar();
            gate->setVar(v);
            s.addAigCNF(gate->getVar(), gate->_fanin1.first->getVar(), gate->_fanin1.second,
                               gate->_fanin2.first->getVar(), gate->_fanin2.second);
            continue; }
        if (gate->_TYPE == 'U') { // Set to CONST 0
            Var v = s.newVar();
            gate->setVar(v);
            s.addAigCNF(gate->getVar(), gate->getVar(), 0, gate->getVar(), 1); }
    }
}

bool
CirMgr::proof(SatSolver& solver, CirGate* first, CirGate* second)
{
    bool inv = first->_value != second->_value;
    Var newV = solver.newVar();
    solver.addXorCNF(newV, first->getVar(), false, second->getVar(), inv);
    solver.assumeRelease();
    solver.assumeProperty(newV, true);
    bool result = solver.assumpSolve();

    #ifndef noprint
    cout<<"\33[2K\r"<<"Proving "<<first->_ID<< " = ";
    if (inv) { cout<<"!"; }
    cout<<second->_ID<<"..."<< (result ? "SAT" : "UNSAT") << "!!"<<flush;
    #endif
    return result;
}

int
CirMgr::satUpdate(vector<string>& input, int num)
{
    int j = 0;
    for (auto iter = _piList.begin(); iter != _piList.end(); iter++) {
        string value = "";
        for (int i = num-1; i >= 0; i--) { value += input[i][j]; }
        (*iter)->_value = stoull(value, nullptr, 2);
        j++; }
    int _size = Simulate(false, input, num);
    store_FEC();
    return _size;
}

void
CirMgr::mergeResult()
{
    int original_A = A;
    for (auto iter = _mergeList.begin(); iter != _mergeList.end(); iter++) {
        iter->second.first->_fanin1.first->_fanoutList.erase(iter->second.first->_ID);
        iter->second.first->_fanin2.first->_fanoutList.erase(iter->second.first->_ID);
        for (auto it : iter->second.first->_fanoutList) {
            if (it.second.first->_fanin1.first->_ID == iter->second.first->_ID) {
                it.second.first->_fanin1.first = iter->first;
                it.second.first->_fanin1.second ^= iter->second.second;
            }
            if (it.second.first->_fanin2.first && it.second.first->_fanin2.first->_ID == iter->second.first->_ID) {
                it.second.first->_fanin2.first = iter->first;
                it.second.first->_fanin2.second ^= iter->second.second;
            }
            if (it.second.first->_TYPE == 'O')
                it.second.first->_INDEX = iter->first->_ID * 2 + it.second.first->_fanin1.second;

            iter->first->_fanoutList.insert({it.first, make_pair(it.second.first, it.second.second ^ iter->second.second)});
        }
        A--;
        _gateList.erase(iter->second.first->_ID);
        iter->second.first->_gatefecList = 0;

        #ifndef noprint
        cout<<"Fraig: "<<iter->first->_ID<<" merging ";
        if (iter->second.second) { cout<<"!"; }
        cout<<iter->second.first->_ID<<"..."<<endl;
        #endif
        delete iter->second.first; iter->second.first = 0;
    }

    if (A != original_A) {
        _dfsList.clear();
        for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) { iter->second->_visited = false; }
        for (auto iter = _poList.begin(); iter != _poList.end(); iter++) { dfsTraversal(*iter); }
        _mergeList.clear();
        #ifndef noprint
        cout<<'\r'<<"Updating by UNSAT... Total #FEC Group = "<<_fecList.size()<<endl;
        #endif
    }
}


