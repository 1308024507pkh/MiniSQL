#ifndef MINISQL_INDEX_MANAGER
#define MINISQL_INDEX_MANAGER

#include <iostream>
#include <string>
#include <map>

#include "BPlusTree.h"
#include "Attribute.h"
#include "API.h"

typedef map<string, BPlusTree<int> *> intBTreeMap;
typedef map<string, BPlusTree<float> *> floatBTreeMap;
typedef map<string, BPlusTree<string> *> stringBTreeMap;

struct data {  //store different values
    int intValue;
    float floatValue;
    string stringValue;
};

class API;

class IndexManager {
private:
    intBTreeMap indexInt;
    floatBTreeMap indexFloat;
    stringBTreeMap indexString;
    int static const _FLOAT = Attribute::TYPE_FLOAT;
    int static const _INT = Attribute::TYPE_INT;
    API *api;

    int GetSize(int dataType);

    int GetDegree(int dataType);

    void GetData(string value, int dataType, struct data &dataValue);

public:
    IndexManager();

    IndexManager(API *_api);

    ~IndexManager();

    void CreateIndex(string fileName, int dataType);

    void DropIndex(string fileName, int dataType);

    void InsertIndex(string fileName, string value, int blockOffset, int dataType);

    int SearchIndex(string fileName, string value, int dataType);

    void DeleteIndex(string fileName, string value, int dataType);
};

#endif
