/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
//#define noprint
/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
    for (auto iter = _gateList.begin(); iter != _gateList.end(); ) {
        if (!iter->second->_visited) {
            if (iter->second->_TYPE == 'A') {
                A--;
                #ifndef noprint
                cout<<"Sweeping: AIG("<<iter->first<<") removed..."<<endl;
                #endif
                iter->second->_fanin1.first->_fanoutList.erase(iter->first);
                iter->second->_fanin2.first->_fanoutList.erase(iter->first);
                if (iter->second->_fanin1.first->_TYPE == 'I' && iter->second->_fanin1.first->_fanoutList.empty())
                    _notusedList.insert(iter->second->_fanin1.first->_ID);
                if (iter->second->_fanin2.first->_TYPE == 'I' && iter->second->_fanin2.first->_fanoutList.empty())
                    _notusedList.insert(iter->second->_fanin2.first->_ID);
                _floatList.erase(iter->first);
                _notusedList.erase(iter->first);
                iter = _gateList.erase(iter);
                continue; }
            else if (iter->second->_TYPE == 'U') {
                #ifndef noprint
                cout<<"Sweeping: UNDEF("<<iter->first<<") removed..."<<endl;
                #endif
                iter = _gateList.erase(iter);
                continue; }
        }
        iter++;
    }
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
    char TO_DO = ' ';
    int original_A = A;
    for (auto iter = _dfsList.begin(); iter != _dfsList.end(); iter++)
    {
        if ((*iter)->_TYPE == 'A')
        {
            if ((*iter)->_fanin1.first->_ID == (*iter)->_fanin2.first->_ID) {
                if ((*iter)->_fanin1.first->_ID == 0) { // 0 0 / 0 !0 / !0 0 / !0 !0
                    if ((*iter)->_fanin1.second && (*iter)->_fanin2.second) { TO_DO = '?'; } // !0 !0
                    else { TO_DO = '0'; } } // 0 0 / 0 !0 / !0 0
                else { // ? ? / ? !? / !? ? / !? !?
                    if ((*iter)->_fanin1.second != (*iter)->_fanin2.second) { TO_DO = '0'; } // ? !? / !? ?
                    else { TO_DO = '?'; } } } // ? ? / !? !?
            else if ((*iter)->_fanin1.first->_ID == 0) { // 0 ? / !0 ?
                if ((*iter)->_fanin1.second) { TO_DO = '?'; } // !0 ? / !0 !?
                else { TO_DO = '0'; } } // 0 ?
            else if ((*iter)->_fanin2.first->_ID == 0) {
                if ((*iter)->_fanin2.second) { TO_DO = '?'; } // ? !0 / !? !0
                else { TO_DO = '0'; } } // ? 0
            else { TO_DO = ' '; }

            switch (TO_DO) {
                case ' ': continue;
                case '0': {
                    (*iter)->_fanin1.first->_fanoutList.erase((*iter)->_ID);
                    (*iter)->_fanin2.first->_fanoutList.erase((*iter)->_ID);
                    for (auto it = (*iter)->_fanoutList.begin(); it != (*iter)->_fanoutList.end(); it++) {
                        if (it->second.first->_fanin1.first->_ID == (*iter)->_ID) {
                            it->second.first->_fanin1.first = _gateList[0];
                            if (it->second.first->_TYPE == 'O') { it->second.first->_INDEX = 0 + it->second.first->_fanin1.second; }
                        }
                        if (it->second.first->_fanin2.first && it->second.first->_fanin2.first->_ID == (*iter)->_ID)
                            it->second.first->_fanin2.first = _gateList[0];
                        _gateList[0]->_fanoutList.insert(*it);
                    }
                    #ifndef noprint
                    cout<<"Simplifying: 0 merging "<<(*iter)->_ID<<"..."<<endl;
                    #endif
                    A--;
                    _gateList.erase((*iter)->_ID);
                    delete *iter; *iter = 0;
                    break; }
                case '?': {
                    (*iter)->_fanin1.first->_fanoutList.erase((*iter)->_ID);
                    (*iter)->_fanin2.first->_fanoutList.erase((*iter)->_ID);
                    for (auto it = (*iter)->_fanoutList.begin(); it != (*iter)->_fanoutList.end(); it++) {
                        if ((*iter)->_fanin1.first->_ID != 0) { // ? ? / !? !? / ? !0 / !? !0
                            if (it->second.first->_fanin1.first->_ID == (*iter)->_ID) {
                                it->second.first->_fanin1.first = (*iter)->_fanin1.first;
                                it->second.first->_fanin1.second ^= (*iter)->_fanin1.second;
                                it->second.second = it->second.first->_fanin1.second;
                                if (it->second.first->_TYPE == 'O') {
                                    it->second.first->_INDEX = (*iter)->_fanin1.first->_ID * 2 + it->second.first->_fanin1.second; }
                            }
                            if (it->second.first->_fanin2.first && it->second.first->_fanin2.first->_ID == (*iter)->_ID) {
                                it->second.first->_fanin2.first = (*iter)->_fanin1.first;
                                it->second.first->_fanin2.second ^= (*iter)->_fanin1.second;
                                it->second.second = it->second.first->_fanin2.second;
                            }

                            (*iter)->_fanin1.first->_fanoutList.insert(*it);
                        }
                        else { // !0 !0 / !0 ? / !0 !?
                            if (it->second.first->_fanin1.first->_ID == (*iter)->_ID) {
                                it->second.first->_fanin1.first = (*iter)->_fanin2.first;
                                it->second.first->_fanin1.second ^= (*iter)->_fanin2.second;
                                it->second.second = it->second.first->_fanin1.second;
                                if (it->second.first->_TYPE == 'O') {
                                    it->second.first->_INDEX = (*iter)->_fanin2.first->_ID * 2 + it->second.first->_fanin1.second; }
                            }
                            if (it->second.first->_fanin2.first && it->second.first->_fanin2.first->_ID == (*iter)->_ID) {
                                it->second.first->_fanin2.first = (*iter)->_fanin2.first;
                                it->second.first->_fanin2.second ^= (*iter)->_fanin2.second;
                                it->second.second = it->second.first->_fanin2.second;
                            }
                            (*iter)->_fanin2.first->_fanoutList.insert(*it);
                        }
                    }
                    #ifndef noprint
                    if ((*iter)->_fanin1.first->_ID != 0) {
                        cout<<"Simplifying: "<<(*iter)->_fanin1.first->_ID<<" merging ";
                        if ((*iter)->_fanin1.second) { cout<<"!"; }
                        cout<<(*iter)->_ID<<"..."<<endl; }
                    else {
                        cout<<"Simplifying: "<<(*iter)->_fanin2.first->_ID<<" merging ";
                        if ((*iter)->_fanin2.second) { cout<<"!"; }
                        cout<<(*iter)->_ID<<"..."<<endl; }
                    #endif
                    A--;
                    _gateList.erase((*iter)->_ID);
                    delete *iter; *iter = 0;
                    break; }
            }
        }
    }
    if (A != original_A) {
        _dfsList.clear();
        for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) { iter->second->_visited = false; }
        for (auto iter = _poList.begin(); iter != _poList.end(); iter++) { dfsTraversal(*iter); }
    }
    for (auto iter = _gateList.begin(); iter != _gateList.end(); iter++) {
        if (iter->second->_TYPE != 'O' && iter->second->_TYPE != 'C' && iter->second->_fanoutList.empty())
            _notusedList.insert(iter->second->_ID);
    }
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
