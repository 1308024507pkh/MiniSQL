#include <iostream>
#include <vector>
#include <sstream>

#include "IndexInfo.h"
#include "IndexManager.h"

using namespace std;

//constructor
IndexManager::IndexManager(API *_api) {
    vector<IndexInfo> allIndex;
    vector<IndexInfo>::iterator iter = allIndex.begin();
    api = _api;
    api->GetIndexAddress(&allIndex);
    for (; iter != allIndex.end(); iter++) {
        CreateIndex(iter->indexName, iter->type);
    }
}

//destructor
IndexManager::~IndexManager() {
    intBTreeMap::iterator intIter = indexInt.begin();
    floatBTreeMap::iterator floatIter = indexFloat.begin();
    stringBTreeMap::iterator stringIter = indexString.begin();
    for (; intIter != indexInt.end(); intIter++) {  //int
        if (intIter->second != NULL) {
            intIter->second->WriteBackToDiskAll();
            delete intIter->second;
        }
    }
    for (; floatIter != indexFloat.end(); floatIter++) {  //float
        if (floatIter->second != NULL) {
            floatIter->second->WriteBackToDiskAll();
            delete floatIter->second;
        }
    }
    for (; stringIter != indexString.end(); stringIter++) {  //string
        if (stringIter->second != NULL) {
            stringIter->second->WriteBackToDiskAll();
            delete stringIter->second;
        }
    }
}

//create a new index
void IndexManager::CreateIndex(string fileName, int dataType) {
    int degree = GetDegree(dataType);
    int size = GetSize(dataType);
    if (dataType == _INT) {  //int
        BPlusTree<int> *BTree = new BPlusTree<int>(fileName, size, degree);
        indexInt.insert(intBTreeMap::value_type(fileName, BTree));
    } else if (dataType == _FLOAT) {  //float
        BPlusTree<float> *BTree = new BPlusTree<float>(fileName, size, degree);
        indexFloat.insert(floatBTreeMap::value_type(fileName, BTree));
    } else {  //string
        BPlusTree<string> *BTree = new BPlusTree<string>(fileName, size, degree);
        indexString.insert(stringBTreeMap::value_type(fileName, BTree));
    }
}

//drop an index
void IndexManager::DropIndex(string fileName, int dataType) {
    if (dataType == _INT) {  //int
        intBTreeMap::iterator intIter;
        intIter = indexInt.find(fileName);
        if (intIter == indexInt.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            delete intIter->second;
            indexInt.erase(intIter);
        }
    } else if (dataType == _FLOAT) {  //float
        floatBTreeMap::iterator floatIter;
        floatIter = indexFloat.find(fileName);
        if (floatIter == indexFloat.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            delete floatIter->second;
            indexFloat.erase(floatIter);
        }
    } else {  //string
        stringBTreeMap::iterator stringIter;
        stringIter = indexString.find(fileName);
        if (stringIter == indexString.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            delete stringIter->second;
            indexString.erase(stringIter);
        }
    }
}

//Search a key in the index
int IndexManager::SearchIndex(string fileName, string value, int dataType) {
    struct data dataValue;
    GetData(value, dataType, dataValue);
    if (dataType == _INT) {  //int
        intBTreeMap::iterator intIter;
        intIter = indexInt.find(fileName);
        if (intIter == indexInt.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return -1;
        } else {
            return intIter->second->SearchKey(dataValue.intValue);
        }

    } else if (dataType == _FLOAT) {  //float
        floatBTreeMap::iterator floatIter;
        floatIter = indexFloat.find(fileName);
        if (floatIter == indexFloat.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return -1;
        } else {
            return floatIter->second->SearchKey(dataValue.floatValue);
        }
    } else {  //string
        stringBTreeMap::iterator stringIter;
        stringIter = indexString.find(fileName);
        if (stringIter == indexString.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return -1;
        } else {
            return stringIter->second->SearchKey(value);
        }
    }
}

//insert a new key into the index
void IndexManager::InsertIndex(string fileName, string value, int blockOffset, int dataType) {
    struct data dataValue;
    GetData(value, dataType, dataValue);
    if (dataType == _INT) {  //int
        intBTreeMap::iterator intIter;
        intIter = indexInt.find(fileName);
        if (intIter == indexInt.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            intIter->second->InsertKey(dataValue.intValue, blockOffset);
        }

    } else if (dataType == _FLOAT) {  //float
        floatBTreeMap::iterator floatIter;
        floatIter = indexFloat.find(fileName);
        if (floatIter == indexFloat.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            floatIter->second->InsertKey(dataValue.floatValue, blockOffset);
        }
    } else {  //string
        stringBTreeMap::iterator stringIter;
        stringIter = indexString.find(fileName);
        if (stringIter == indexString.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            stringIter->second->InsertKey(value, blockOffset);
        }
    }
}

//delete a key from the index
void IndexManager::DeleteIndex(string fileName, string value, int dataType) {
    struct data dataValue;
    GetData(value, dataType, dataValue);
    if (dataType == _INT) {  //int
        intBTreeMap::iterator intIter;
        intIter = indexInt.find(fileName);
        if (intIter == indexInt.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            intIter->second->DeleteKey(dataValue.intValue);
        }
    } else if (dataType == _FLOAT) {  //float
        floatBTreeMap::iterator floatIter;
        floatIter = indexFloat.find(fileName);
        if (floatIter == indexFloat.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            floatIter->second->DeleteKey(dataValue.floatValue);
        }
    } else {   //string
        stringBTreeMap::iterator stringIter;
        stringIter = indexString.find(fileName);
        if (stringIter == indexString.end()) {
            cout << "Error: No index named " << fileName << "exists." << endl;
            return;
        } else {
            stringIter->second->DeleteKey(value);
        }
    }
}

//get the key size
int IndexManager::GetSize(int dataType) {
    if (dataType == _INT)
        return sizeof(int);
    else if (dataType == _FLOAT)
        return sizeof(float);
    else if (dataType > 0)
        return (dataType + 1);
    else {
        cout << "Error: in GetSize: invalid type." << endl;
        return -1;
    }
}

// get the degree of the B+ tree
int IndexManager::GetDegree(int dataType) {
    int degree = BLOCK_LEN_ENABLE / (GetSize(dataType) + sizeof(int));
    //the degree must be odd.
    if (!(degree % 2)) degree--;
    return degree;
}

//Get different value
void IndexManager::GetData(string value, int dataType, struct data &dataValue) {
    stringstream valuestream;
    valuestream << value;
    if (dataType == _INT)
        valuestream >> dataValue.intValue;
    else if (dataType == _FLOAT)
        valuestream >> dataValue.floatValue;
    else if (dataType > 0)
        valuestream >> dataValue.stringValue;
}
