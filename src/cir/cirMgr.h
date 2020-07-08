/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <map>
#include <deque>
#include <set>
#include <vector>
#include <string>
#include <utility>
#include <fstream>
#include <iostream>

using namespace std;

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr() : fraiged(false), simed(false) {}
   ~CirMgr() { for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) { delete iter->second; } }

   // Access functions: return '0' if "gid" corresponds to an undefined gate.
    CirGate* getGate(int);

    // Member functions about circuit construction
    bool readCircuit(const string&);
    bool readHeader(const string&);
    bool readInput(const string&);
    bool readOutput(const string&, int&);
    bool readAig(const string&);
    bool readComment(const string&, bool&);
    bool readSymbol(const string&);
    void splitString (const string&, vector<string>&) const;
    void connectCircuit();
    void dfsTraversal(CirGate*);

    // Member functions about circuit optimization
    void sweep();
    void optimize();

    // Member functions about simulation
    void randomSim();
    void fileSim(ifstream&);
    void setSimLog(ofstream *logFile) { _simLog = logFile; }
    int  Simulate(bool random, vector<string>& pi, int num = 64);
    void add_first_FEC();
    void store_FEC();

    // Member functions about fraig
    void strash();
    size_t StrashKey(CirGate* g) const;
    void fraig();
    void genProofModel(SatSolver& s);
    bool proof(SatSolver& s, CirGate* first, CirGate* second);
    int satUpdate(vector<string>& input, int num = 64);
    void mergeResult();

    // Member functions about circuit reporting
    void printSummary() const;
    void printNetlist() const;
    void printPIs() const;
    void printPOs() const;
    void printFloatGates() const;
    void printFECPairs();
    void writeAag(ostream&) const;
    void writeGate(ostream&, CirGate*) const;


private:
    ofstream *_simLog;

    bool fraiged;
    bool simed;
    int M,I,L,O,A;
    vector<CirGate*> _piList;
    vector<CirGate*> _poList;
    vector<pair<CirGate*,vector<int>>> _aigList;
    map<int,CirGate*> _gateList;
    vector<CirGate*> _dfsList;
    set<int> _floatList;
    set<int> _notusedList;
    deque<pair<int,map<int, pair<CirGate*, bool>>>> _fecList;
    vector<pair<CirGate*,pair<CirGate*,bool>>> _mergeList;
};

#endif // CIR_MGR_H
