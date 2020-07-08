/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include <unordered_map>
#include <bitset>
#include <deque>

using namespace std;

/*******************************/
/*   Global variable and enum  */
/*******************************/
//#define noprint
/**************************************/
/*   Static varaibles and functions   */
/**************************************/
struct SimKey
{
    SimKey(CirGate* g, bool i) {
        if (i) { _value = ~(g->getValue()); }
        else { _value = g->getValue(); } }
    size_t _value;
    bool operator == (const SimKey& k) const { return _value == k._value; }
};


namespace std {
    template <> struct hash<SimKey>
    { size_t operator()(const SimKey& k) const { return (k._value); } };
}

void
CirMgr::add_first_FEC()
{
    if (_fecList.empty() && !fraiged) { // Put All aig gates in one FEC group and add to _fecList (if fraiged, don't put again)
        map<int, pair<CirGate*,bool>> fecGroup;
        fecGroup.insert({0, {_gateList[0],false}});
        for (auto gate : _dfsList) {
            if (gate->_TYPE == 'A')  fecGroup.insert({gate->_ID, {gate,false}}); }
        _fecList.push_front(make_pair(0,fecGroup)); }
}

void
CirMgr::store_FEC()
{
    sort(_fecList.begin(), _fecList.end(),
    [](pair<int, map<int, pair<CirGate*, bool>>>& lhs, pair<int, map<int, pair<CirGate*, bool>>>& rhs)
    { return lhs.first < rhs.first; });

    for (auto gate : _dfsList) { gate->_gatefecList = 0; } // for safety purpose
    for (auto iter = _fecList.begin(); iter != _fecList.end(); iter++) { // store pointer of map to gate
        for (auto it : iter->second)
            it.second.first->_gatefecList = &(iter->second);
    }
}

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/

void
CirMgr::randomSim()
{
    if (fraiged) { return; }// no need to repeat
    int _size    = 0;       // number of FEC groups
    int prevsize = 0;       // previous number of FEC groups
    int time     = 0;       // number of simulations
    int max_time = 2<<12;   // max number of simulations
    int min_time = 4;       // min number of simulations
    int same     = 0;       // number of simulations with same number of FEC groups
    int max_same;
    if (I < 20)        { max_same = 3;   }
    else if (I < 100)  { max_same = 10;  }
    else if (I < 1000) { max_same = 50;  }
    else {               max_same = 100; }

    vector<string> input;
    #ifndef noprint
    cout<<endl<<"Total #FEC Group = 0"<<flush;
    #endif
    if (!simed) { add_first_FEC(); }

    while (time < min_time || (time < max_time && same < max_same)) {
        time++;
        prevsize = _size;
        input.clear();
        // Generate Random patterns
        for (auto iter = _piList.begin(); iter != _piList.end(); iter++)
            (*iter)->_value = (size_t)rnGen(1<<16)<<48 | (size_t)rnGen(1<<16)<<32 |
                          (size_t)rnGen(1<<16)<<16 | rnGen(1<<16);
        _size = Simulate(true, input);
        if (_size == prevsize) { same++; }
        else { same = 0; }
    }

    #ifndef noprint
    cout<<"\r"<<time*64<<" patterns simulated."<<endl;
    #endif
    store_FEC();
    simed = true;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
    bool wrong_input = false;
    int pattern = 0;
    string str;
    vector<string> input;
    if (!simed) { add_first_FEC(); }
    while (patternFile >> str) {
        if ((int)str.size() != I) {
            cerr<<endl<<"Error: Pattern("<<str<<") length("<<str.size()
                <<") does not match the number of inputs("<<I<<") in a circuit!!"<<endl;
            pattern -= pattern % 64;
            wrong_input = true;
            break; }
        if (str.find_first_not_of("01") != string::npos) {
            cerr<<endl<<"Error: Pattern("<<str<<") contains a non-0/1 character('"
                <<str[str.find_first_not_of("01")]<<"')."<<endl;
            pattern -= pattern % 64;
            wrong_input = true;
            break; }

        input.push_back(str);
        pattern++;
        if (pattern % 64 == 0) {
            int j = 0;
            for (auto iter = _piList.begin(); iter != _piList.end(); iter++) {
                string value = "";
                for (int i = 63; i >= 0; i--) { value += input[i][j]; }
                (*iter)->_value = stoull(value, nullptr, 2);
                j++; }
            Simulate(false, input);
            input.clear(); }
    }

    if (pattern % 64 != 0) {
        int j = 0;
        for (auto iter = _piList.begin(); iter != _piList.end(); iter++) {
            string value = "";
            for (int i = pattern % 64 -1; i >= 0; i--) { value += input[i][j];}
            (*iter)->_value = stoull(value, nullptr, 2);
            j++;
        }
        Simulate(false, input, pattern%64);
    }
    cout<<"\r"<<pattern<<" patterns simulated."<<endl;
    if (!wrong_input && !fraiged) { store_FEC(); }
    simed = true;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
int
CirMgr::Simulate(bool random, vector<string>& input, int num)
{
    for (auto gate : _dfsList) // Stimulate entire circuit
    {
        if (gate->_TYPE == 'A') {
            size_t in1 = gate->_fanin1.first->_value;
            size_t in2 = gate->_fanin2.first->_value;
            if (gate->_fanin1.second) { in1 = ~in1; }
            if (gate->_fanin2.second) { in2 = ~in2; }
            gate->_value = in1 & in2; }
        else if (gate->_TYPE == 'O') {
            size_t in1 = gate->_fanin1.first->_value;
            if (gate->_fanin1.second) { in1 = ~in1; }
            gate->_value = in1; }
    }

    int _size = _fecList.size();
    for (int i = 0 ; i < _size; i++) {
        unordered_map<SimKey, map<int, pair<CirGate*, bool>>> Hash;
        for (auto in : _fecList.front().second) {
            auto m = Hash.find(SimKey(in.second.first, false));
            if (m != Hash.end()) { m->second.insert({in.second.first->_ID, {in.second.first, false}}); continue; }
            auto n = Hash.find(SimKey(in.second.first, true));
            if (n != Hash.end()) { n->second.insert({in.second.first->_ID, {in.second.first, true}}); continue; }
            Hash.insert({SimKey(in.second.first,false),map<int, pair<CirGate*, bool>>{{in.second.first->_ID, {in.second.first,false}}}});
        }
        for (auto bucket : Hash) {
            if (bucket.second.size() > 1) { _fecList.push_back(make_pair(bucket.second.begin()->first, bucket.second)); }
        }
        _fecList.pop_front();
    }

    _size = _fecList.size();
    #ifndef noprint
    cout<<"\33[2K\r"<<"Total #FEC Group = "<<_size<<flush;
    #endif

    if (_simLog) {
        if (random) {
            vector<string> pi;
            for (auto iter = _piList.begin(); iter != _piList.end(); iter++)
            pi.push_back(bitset<64>((*iter)->_value).to_string());
            for (int i = 63; i >= 0; i--) {
                string input_string = "";
                for (int j = 0; j < I; j++) { input_string += pi[j][i]; }
                    input.push_back(input_string); }
        }
        vector<string> po;
        vector<string> output;
        for (auto iter = _poList.begin(); iter != _poList.end(); iter++)
            po.push_back(bitset<64>((*iter)->_value).to_string());
        for (int i = 63; i >= 0; i--) {
            string output_string = "";
            for (int j = 0; j < O; j++) { output_string += po[j][i]; }
                output.push_back(output_string); }
        for (int i = 0; i < num; i++) { (*_simLog)<<input[i]<<" "<<output[i]<<endl; }
    }
    return _size;
}
