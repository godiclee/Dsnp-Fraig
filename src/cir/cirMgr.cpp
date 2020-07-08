/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <sstream>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;
static unsigned colNo  = 0;
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

CirGate* CirMgr::getGate(int gid)
{
    if (!_gateList.count(gid)) { return 0; }
    return _gateList[gid];
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool CirMgr::readCircuit(const string& fileName)
{
    fstream file;
    file.open(fileName,ios::in);
    if (!file) { cout<<"Cannot open design \""<<fileName<<"\"!!"<<endl; return false; }
    if (file.peek() == EOF) { lineNo = 0; errMsg = "aag"; parseError(MISSING_IDENTIFIER); return false; }

    lineNo = 0;
    int m,i,o,a;

    while (file.getline(buf, sizeof(buf), '\n')) {
        string str = buf;
        lineNo++;
        colNo = 0;

        if (lineNo == 1) {
            if (!readHeader(str)) { return false; }
            m = M; i = I; o = O; a = A; }
        else if (i > 0) { if (!readInput(str))     { return false; } i--; }
        else if (o > 0) { if (!readOutput(str, m)) { return false; } o--; }
        else if (a > 0) { if (!readAig(str))       { return false; } a--; }
        else {
            bool _end = false;
            if (!readComment(str, _end)) { return false; }
            if (_end) { break; }
            if (!readSymbol(str)) { return false; } }
    }

    if (i) { lineNo++; errMsg = "PI";  parseError(MISSING_DEF); return false; }
    if (o) { lineNo++; errMsg = "PO";  parseError(MISSING_DEF); return false; }
    if (a) { lineNo++; errMsg = "AIG"; parseError(MISSING_DEF); return false; }

    connectCircuit();
    for (auto iter = _poList.begin(); iter != _poList.end(); iter++) { dfsTraversal(*iter); }
    fraiged = false;
    simed = false;
    return true;
}

bool CirMgr::readHeader(const string& str)
{
    if (str.find_first_not_of(' ')) { parseError(EXTRA_SPACE); return false; }
    if (str.find_first_not_of('\t')) { errInt = 9; parseError(ILLEGAL_WSPACE); return false; }

    vector<string> v;
    splitString(str, v);

    if (v[0] == "aag") { colNo = 3; }
    else if (v[0].substr(0,3) == "aag" && isdigit(v[0][3])) { colNo = 3; parseError(MISSING_SPACE); return false; }
    else { errMsg = v[0]; parseError(ILLEGAL_IDENTIFIER); return false; }

    unsigned _size = str.size();

    vector<string> _type = {"variables", "PIs", "latches", "POs", "AIGs"};
    for (int typenum = 0; typenum < 5;) {
        if (colNo == _size) { errMsg = "number of "+_type[typenum]; parseError(MISSING_NUM); return false; }
        if (str[colNo] == '\t') { parseError(MISSING_SPACE); return false; }
        colNo++;
        if (colNo == _size) { errMsg = "number of "+_type[typenum]; parseError(MISSING_NUM); return false; }
        if (str[colNo] == ' ') { parseError(EXTRA_SPACE); return false; }
        if (str[colNo] == '\t') { errInt = 9; parseError(ILLEGAL_WSPACE); return false; }
        colNo += v[++typenum].size();
        if (v[typenum].find_first_not_of("0123456789") != string::npos) {
            errMsg = "number of "+_type[typenum-1]+"("+v[typenum]+")"; parseError(ILLEGAL_NUM); return false; }
    }
    if (colNo != _size) { parseError(MISSING_NEWLINE); return false; }

    M = stoi(v[1]);
    I = stoi(v[2]);
    L = stoi(v[3]);
    O = stoi(v[4]);
    A = stoi(v[5]);

    if (L) { errMsg = "latches"; parseError(ILLEGAL_NUM); return false; }
    if (M < (I+L+A)) { errMsg = "Number of variables"; errInt = M; parseError(NUM_TOO_SMALL); return false; }
    return true;
}

bool CirMgr::readInput(const string& str)
{
    if (str.empty()) { errMsg = "PI literal ID"; parseError(MISSING_NUM); return false; }
    size_t not_int_pos = str.find_first_not_of("0123456789");
    if (not_int_pos == string::npos) {
        int id = stoi(str);
        if (id == 0 || id == 1) { errInt = id; parseError(REDEF_CONST); return false; }
        if (id > M*2+1) { errInt = id; parseError(MAX_LIT_ID); return false; }
        if (id % 2 != 0) { errMsg = "PI"; errInt = id; parseError(CANNOT_INVERTED); return false; }
        auto iter = _gateList.find(id/2);
        if (iter != _gateList.end()) { errInt = id; errGate = iter->second; parseError(REDEF_GATE); return false;}

        CirGate* p = new CirGate(id/2, lineNo, 'I', id);
        _piList.push_back(p);
        _gateList[id/2] = p;
        return true; }

    if (str[0] == ' ') { parseError(EXTRA_SPACE); return false; }
    if (str[0] == '\t') { errInt = 9; parseError(ILLEGAL_WSPACE); return false; }
    if (str[not_int_pos] == ' ' || str[not_int_pos] == '\t') { colNo = 1; parseError(MISSING_NEWLINE); return false; }
    else { errMsg = "PI literal ID("+ str.substr(0,str.find_first_of(" \t")) +")"; parseError(ILLEGAL_NUM); return false; }
}

bool CirMgr::readOutput(const string& str, int& m)
{
    if (str.empty()) { errMsg = "PO literal ID"; parseError(MISSING_NUM); return false; }
    size_t not_int_pos = str.find_first_not_of("0123456789");
    if (not_int_pos == string::npos) {
        int id = stoi(str);
        if (id > M*2+1) { errInt = id; parseError(MAX_LIT_ID); return false; }

        m++;
        CirGate* p = new CirGate(m, lineNo, 'O', id);
        _poList.push_back(p);
        _gateList[m] = p;
        return true; }

    if (str[0] == ' ') { parseError(EXTRA_SPACE); return false; }
    if (str[0] == '\t') { errInt = 9; parseError(ILLEGAL_WSPACE); return false; }
    if (str[not_int_pos] == ' ' || str[not_int_pos] == '\t') { colNo = 1; parseError(MISSING_NEWLINE); return false; }
    else { errMsg = "PO literal ID("+ str.substr(0,str.find_first_of(" \t")) +")"; parseError(ILLEGAL_NUM); return false; }
}

bool CirMgr::readAig(const string& str)
{
    if (str.empty()) { errMsg = "AIG gate literal ID"; parseError(MISSING_NUM); return false; }

    unsigned _size = str.size();
    vector<string> v;
    splitString(str, v);

    for (int num = 0; num < 3;) {
        if (colNo == _size) { errMsg = "space character"; parseError(MISSING_NUM); return false; }
        if (str[colNo] == '\t') { parseError(MISSING_SPACE); return false; }
        if (num) { colNo++; }
        if (colNo == _size) { errMsg = "AIG input literal ID"; parseError(MISSING_NUM); return false; }
        if (str[colNo] == ' ') { parseError(EXTRA_SPACE); return false; }
        if (str[colNo] == '\t') { errInt = 9; parseError(ILLEGAL_WSPACE); return false; }
        if (v[num].find_first_not_of("0123456789") != string::npos) {
            errMsg = "AIG input literal ID("+v[num]+")"; parseError(ILLEGAL_NUM); return false; }

        int id = stoi(v[num]);
        if (num == 0 && (id == 0 || id == 1)) { errInt = id; parseError(REDEF_CONST); return false; }
        if (id > M*2+1) { errInt = id; parseError(MAX_LIT_ID); return false; }
        if (num == 0) {
            if (id % 2 != 0) { errMsg = "AIG gate"; errInt = id; parseError(CANNOT_INVERTED); return false; }
            auto iter = _gateList.find(id/2);
            if (iter != _gateList.end() && iter->second->_TYPE != 'O') {
                errInt = id; errGate = iter->second; parseError(REDEF_GATE); return false; }
        }
        colNo += v[num].size();
        num++;
    }
    if (colNo != _size) { parseError(MISSING_NEWLINE); return false; }

    CirGate* p = new CirGate(stoi(v[0])/2, lineNo, 'A', stoi(v[0]));
    _aigList.push_back(make_pair(p, vector<int>{stoi(v[1]), stoi(v[2])}));
    _gateList[stoi(v[0])/2] = p;
    return true;
}

bool CirMgr::readComment(const string & str, bool& _end)
{
    if (str == "c") { _end = true; return true; }
    if (str[0] == 'c') { colNo = 1; parseError(MISSING_NEWLINE); return false; }
    return true;
}

bool CirMgr::readSymbol(const string& str)
{
    switch (str[0]) {
        case (' '):  parseError(EXTRA_SPACE); return false;
        case ('\t'): errInt = 9; parseError(ILLEGAL_WSPACE); return false;
        case ('i'):  case ('o'): colNo++; break;
        default:     errMsg = str[0]; parseError(ILLEGAL_SYMBOL_TYPE); return false; }

    if (!str[1])          { errMsg = "symbol index"; parseError(MISSING_NUM); return false; }
    if (str[1] == ' ')    { parseError(EXTRA_SPACE); return false; }
    if (str[1] == '\t')   { errInt = 9; parseError(ILLEGAL_WSPACE); return false; }

    string index_string = str.substr(1, str.find_first_of(" \t")-1);
    if (index_string.find_first_not_of("0123456789") != string::npos)
        { errMsg = "symbol index("+index_string+")"; parseError(ILLEGAL_NUM); return false; }

    colNo += index_string.size();
    if (!str[colNo] || (str.substr(colNo).find_first_not_of(' ') == string::npos))
        { errMsg = "symbolic name"; parseError(MISSING_IDENTIFIER); return false; }
    if (str[colNo] == '\t') { errMsg = "space character"; parseError(MISSING_NUM); return false; }

    colNo++;
    string name = str.substr(colNo);
    for (auto iter = name.begin(); iter != name.end(); iter++, colNo++) {
        if (!isprint(*iter)) { errInt = int(*iter); parseError(ILLEGAL_SYMBOL_NAME); return false; }
    }

    int index = stoi(index_string);
    if (str[0] == 'i') {
        if (index >= I)
            { errMsg = "PI index"; errInt = index; parseError(NUM_TOO_BIG); return false; }
        if (_piList[index]->_NAME != "")
            { errMsg = str[0]; errInt = index; parseError(REDEF_SYMBOLIC_NAME); return false; }
        _piList[index]->_NAME = name;
        return true; }

    if (str[0] == 'o') {
        if (index >= O)
            { errMsg = "PO index"; errInt = index; parseError(NUM_TOO_BIG); return false; }
        if (_poList[index]->_NAME != "")
            { errMsg = str[0]; errInt = index; parseError(REDEF_SYMBOLIC_NAME); return false; }
        _poList[index]->_NAME = name;
        return true; }

    return false;
}

void CirMgr::splitString (const string& str, vector<string>& v) const
{
    string t;
    istringstream in(str);
    while (in >> t) { v.push_back(t); }
}

void CirMgr::connectCircuit()
{
    bool pushed = false;
    _gateList[0] = new CirGate();

    for (auto iter = _poList.begin(); iter != _poList.end(); iter++) {
        int index = (*iter)->_INDEX;
        if (!_gateList.count(index/2)) {
            _gateList[index/2] = new CirGate(index/2,0,'U',index);
            _floatList.insert((*iter)->_ID); }
        (*iter)->_fanin1.first  = _gateList[index/2];
        (*iter)->_fanin1.second = index%2;
        _gateList[index/2]->_fanoutList.insert({(*iter)->_ID, {*iter, index%2}});
    }
    for (auto iter = _aigList.begin(); iter != _aigList.end(); iter++) {
        int index0 = iter->second[0];
        int index1 = iter->second[1];
        if (!_gateList.count(index0/2)) {
            _floatList.insert(iter->first->_ID);
            pushed = true;
            _gateList[index0/2] = new CirGate(index0/2,0,'U',index0); }
        if (!_gateList.count(index1/2)) {
            if (!pushed) {
                _floatList.insert(iter->first->_ID); }
            _gateList[index1/2] = new CirGate(index1/2,0,'U',index1); }
        if (_gateList[index0/2]->_TYPE == 'U')
            _floatList.insert(iter->first->_ID);

        if (_gateList[index1/2]->_TYPE == 'U')
            _floatList.insert(iter->first->_ID);

        iter->first->_fanin1.first  = _gateList[index0/2];
        iter->first->_fanin1.second = index0%2;
        iter->first->_fanin2.first  = _gateList[index1/2];
        iter->first->_fanin2.second = index1%2;

        _gateList[iter->second[0]/2]->_fanoutList.insert({iter->first->_ID, {iter->first,index0%2}});
        _gateList[iter->second[1]/2]->_fanoutList.insert({iter->first->_ID, {iter->first,index1%2}});
        pushed = false;
    }

    for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) {
        if (iter->second->_TYPE != 'O' && iter->second->_TYPE != 'C' && iter->second->_fanoutList.empty())
            _notusedList.insert(iter->second->_ID);
    }
}

void CirMgr::dfsTraversal(CirGate* gate)
{
    if (gate->_fanin1.first) {
        if (!gate->_fanin1.first->_visited) {
            gate->_fanin1.first->_visited = true;
            dfsTraversal(gate->_fanin1.first); } }

    if (gate->_fanin2.first) {
        if (!gate->_fanin2.first->_visited) {
            gate->_fanin2.first->_visited = true;
            dfsTraversal(gate->_fanin2.first); } }
    _dfsList.push_back(gate);
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/

void CirMgr::printSummary() const
{
    cout<<endl;
    cout<<"Circuit Statistics"<<endl;
    cout<<"=================="<<endl;
    cout<<"  PI"<<setfill(' ')<<setw(12)<<I<<endl;
    cout<<"  PO"<<setfill(' ')<<setw(12)<<O<<endl;
    cout<<"  AIG"<<setfill(' ')<<setw(11)<<A<<endl;
    cout<<"------------------"<<endl;
    cout<<"  Total"<<setfill(' ')<<setw(9)<<I+O+A<<endl;
}

void CirMgr::printNetlist() const
{
    cout<<endl;
    int i = 0;
    for (auto iter = _dfsList.begin(); iter != _dfsList.end(); iter++) {
        if ((*iter)->_TYPE == 'U') { continue; }
        cout<<"["<<i<<"] ";
        i++;
        if ((*iter)->_TYPE == 'C') { cout<<"CONST0"<<endl; continue; }
        cout<<left<<setw(4)<<(*iter)->getTypeStr()<<internal<<(*iter)->_ID;
        if ((*iter)->_fanin1.first) {
            cout<<" ";
            if ((*iter)->_fanin1.first->_TYPE == 'U') { cout<<"*"; }
            if ((*iter)->_fanin1.second) { cout<<"!"; }
            cout<<(*iter)->_fanin1.first->_ID; }

        if ((*iter)->_fanin2.first) {
            cout<<" ";
            if ((*iter)->_fanin2.first->_TYPE == 'U') { cout<<"*"; }
            if ((*iter)->_fanin2.second) { cout<<"!"; }
            cout<<(*iter)->_fanin2.first->_ID; }
        if ((*iter)->_NAME != "")
            cout<<" ("<<(*iter)->_NAME<<")";
        cout<<endl;
    }
}

void CirMgr::printPIs() const
{
    cout << "PIs of the circuit:";
    for (auto iter = _piList.begin(); iter != _piList.end(); iter++)
        cout<<" "<<(*iter)->_ID;
    cout << endl;
}

void
CirMgr::printPOs() const
{
    cout << "POs of the circuit:";
    for (int i = M+1; i <= M+O; i++)
        cout<<" "<<i;
    cout << endl;
}

void
CirMgr::printFloatGates() const
{
    if (!_floatList.empty()) {
        cout<<"Gates with floating fanin(s):";
        for (auto iter = _floatList.begin(); iter != _floatList.end(); iter++)
            cout<<" "<<*iter;
        cout<<endl; }
    if (!_notusedList.empty()) {
        cout<<"Gates defined but not used  :";
        for (auto iter = _notusedList.begin(); iter != _notusedList.end(); iter++)
            cout<<" "<<*iter;
        cout<<endl; }
}

void
CirMgr::printFECPairs()
{
    int num = 0;
    for (auto iter : _fecList) {
        cout<<"["<<num<<"]";
        for (auto it : iter.second) {
            cout<<" ";
            if (it.second.second) { cout<<"!"; }
            cout<<it.second.first->_ID; }
        cout<<endl;
        num++; }
}

void
CirMgr::writeAag(ostream& outfile) const
{
    int newA = 0;
    outfile<<"aag "<<M<<" "<<I<<" "<<L<<" "<<O<<" ";
    for (auto iter = _dfsList.begin(); iter != _dfsList.end(); iter++) {
        if ((*iter)->_TYPE == 'A')
            newA++;
    }
    outfile<<newA<<endl;
    for (auto iter = _piList.begin(); iter != _piList.end(); iter++)
        outfile<<(*iter)->_INDEX<<endl;
    for (auto iter = _poList.begin(); iter != _poList.end(); iter++)
        outfile<<(*iter)->_INDEX<<endl;
    for (auto iter = _dfsList.begin(); iter != _dfsList.end(); iter++) {
        if ((*iter)->_TYPE == 'A')
            outfile<<(*iter)->_INDEX<<" "<<(*iter)->_fanin1.first->_INDEX + (*iter)->_fanin1.second<<" "
                                         <<(*iter)->_fanin2.first->_INDEX + (*iter)->_fanin2.second<<endl;
    }
    int i = 0, j = 0;
    for (auto iter = _piList.begin(); iter != _piList.end(); iter++) {
        if ((*iter)->_NAME != "") {
            outfile<<"i"<<i<<" "<<(*iter)->_NAME<<endl;
            i++; }
    }
    for (auto iter = _poList.begin(); iter != _poList.end(); iter++) {
        if ((*iter)->_NAME != "") {
            outfile<<"o"<<j<<" "<<(*iter)->_NAME<<endl;
            j++; }
    }
    outfile<<"c"<<endl;
    outfile<<"AAG output by Chung-Yang (Ric) Huang"<<endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
}

