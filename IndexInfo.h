#ifndef MINISQL_INDEX_INFO_H
#define MINISQL_INDEX_INFO_H

#include <string>

using namespace std;

class IndexInfo {
public:
    string indexName;
    string tableName;
    string attribute;
    int type;

    IndexInfo(string _indexName, string _tableName, string _attribute, int _type) {
        indexName = _indexName;
        tableName = _tableName;
        attribute = _attribute;
        type = _type;
    }
};

#endif