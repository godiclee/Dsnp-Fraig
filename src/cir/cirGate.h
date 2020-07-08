/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include "cirDef.h"
#include "sat.h"
#include <map>

using namespace std;

class CirGate;
static vector<int> printed;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
    CirGate(int ID = 0, int LINE = 0, char TYPE = 'C', int INDEX = 0, string NAME = "") :
        _ID(ID), _LINE(LINE), _TYPE(TYPE), _INDEX(INDEX), _NAME(NAME),
        _visited(false), _fraiged(false), _removed(false), _value(0), _gatefecList(0) {}
    ~CirGate() {}
    friend class CirMgr;

    // Basic access methods
    string getTypeStr() const {
        switch (_TYPE) {
            case 'C': return "CONST";
            case 'U': return "UNDEF";
            case 'A': return "AIG";
            case 'I': return "PI";
            case 'O': return "PO";
            default: return "ERROR"; }
    }
    int getLineNo() const { return _LINE; }
    bool isAig() const { return false; }
    size_t getValue() const { return _value; }
    Var getVar() const { return _var; }
    void setVar(const Var& v) { _var = v; }

    // Printing functions
    void reportGate() const;
    void reportFanin(int level, bool reseted = false, bool inverted = false, int indent = 0);
    void reportFanout(int level, bool reseted = false, bool inverted = false, int indent = 0);

private:
    int _ID;
    int _LINE;
    char _TYPE;
    int _INDEX;
    string _NAME;
    bool _visited;
    bool _fraiged;
    bool _removed;
    size_t _value;

    Var _var;

    map<int,pair<CirGate*,bool>>* _gatefecList;
    pair<CirGate*, bool> _fanin1;
    pair<CirGate*, bool> _fanin2;
    multimap<int, pair<CirGate*, bool>> _fanoutList;
};

#endif // CIR_GATE_H
