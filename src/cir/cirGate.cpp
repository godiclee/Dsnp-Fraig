/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <bitset>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void CirGate::reportGate() const
{
    cout<<"================================================================================"<<endl;
    string str = "= " + getTypeStr() + "(" + to_string(_ID) + ")";
    if (!_NAME.empty()) { str = str + "\"" + _NAME + "\""; }
    str = str + ", line " + to_string(_LINE);
    cout<<str<<endl;
    cout<<"= FECs:";

    bool invert = false;
    if (_gatefecList) {
        auto it = _gatefecList->find(_ID);
        if (it != _gatefecList->end()) {
            invert = it->second.second;
            for (auto i : *_gatefecList) {
                if (i.first == _ID) { continue; }
                cout<<" ";
                if (invert ^ i.second.second) { cout<<"!"; }
                cout<<i.first; }
        }
    }
    cout<<endl;
    string v = bitset<64>(_value).to_string();
    cout<<"= Value: ";
    for (int i = 0; i < 56; i += 8)
        cout<<v.substr(i,8)<<"_";
    cout<<v.substr(56,8)<<endl;
    cout<<"================================================================================"<<endl;
}

void CirGate::reportFanin(int level, bool reseted, bool inverted, int indent)
{
    assert (level >= 0);
    if (!reseted) { printed.clear(); }
    for (int i = 0; i < indent; i++) { cout<<"  "; }
    if (inverted) { cout<<"!"; }
    cout<<getTypeStr()<<" "<<_ID;
    if (!level) { cout<<endl; return; }

    if ((find(printed.begin(), printed.end(), _ID) != printed.end()) && (_fanin1.first || _fanin2.first)) {
        cout<<" (*)"<<endl; return; }
    cout<<endl;
    printed.push_back(_ID);

    if (_fanin1.first) { _fanin1.first->reportFanin(level-1, true, _fanin1.second, indent+1); }
    if (_fanin2.first) { _fanin2.first->reportFanin(level-1, true, _fanin2.second, indent+1); }
}

void CirGate::reportFanout(int level, bool reseted, bool inverted, int indent)
{
    assert (level >= 0);
    if (!reseted) { printed.clear(); }
    for (int i = 0; i < indent; i++) { cout<<"  "; }
    if (inverted) { cout<<"!"; }
    cout<<getTypeStr()<<" "<<_ID;
    if (!level) { cout<<endl; return; }

    bool have_out = false;
    for (auto iter = _fanoutList.begin(); iter != _fanoutList.end(); iter++) {
        if ((*iter).second.first) { have_out = true; }
    }
    if ((find(printed.begin(), printed.end(), _ID) != printed.end()) && have_out) {
        cout<<" (*)"<<endl; return; }
    cout<<endl;
    printed.push_back(_ID);

    for (auto iter = _fanoutList.begin(); iter != _fanoutList.end(); iter++) {
        if ((*iter).second.first) { (*iter).second.first->reportFanout(level-1, true, (*iter).second.second, indent+1); }
    }
}

