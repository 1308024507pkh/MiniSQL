#ifndef API_H
#define API_H

#include "Attribute.h"
#include "Condition.h"
#include "Minisql.h"
#include "IndexInfo.h"
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>

using namespace std;

class CatalogManager;

class RecordManager;

class IndexManager;

class API {
public:
    API();

    ~API();

// Table
    // 表修改
    void TableDrop(string tableName);

    void
    TableCreate(string tableName, vector<Attribute> *attributeVector, string primaryKeyName, int primaryKeyLocation);


// Catalog
    CatalogManager *cm;


// Index
    IndexManager *im;

    // 索引修改
    void IndexDrop(string indexName);

    void IndexCreate(string indexName, string tableName, string attributeName);

    void IndexValueInsert(string indexName, string value, int blockOffset);

    void IndexInsert(string indexName, char *value, int type, int blockOffset);

    // 获取索引地址
    void GetIndexAddress(vector<IndexInfo> *indexNameVector);


// Record
    RecordManager *rm;

    // 记录修改
    void RecordInsert(string tableName, vector<string> *recordContent);

    void RecordDelete(string tableName);

    void RecordDelete(string tableName, vector<Condition> *conditionVector);

    // 对记录索引的修改
    void RecordIndexDelete(char *recordBegin, int recordSize, vector<Attribute> *attributeVector, int blockOffset);

    void RecordIndexInsert(char *recordBegin, int recordSize, vector<Attribute> *attributeVector, int blockOffset);

    // 获取记录数据
    int GetRecordNum(string tableName);

    int GetRecordSize(string tableName);

    // 记录输出
    void RecordShow(string tableName, vector<string> *attributeNameVector = NULL);

    void RecordShow(string tableName, vector<string> *attributeNameVector, vector<Condition> *conditionVector);


// Attribute
    // 获取属性特值
    int GetTypeSize(int type);

    int GetAttributeName(string tableName, vector<string> *attributeNameVector);

    int GetAttributeType(string tableName, vector<string> *attributeTypeVector);

    int GetAttribute(string tableName, vector<Attribute> *attributeVector);


private:
    // 
    int TableExist(string tableName);

    int GetIndexNameList(string tableName, vector<string> *indexNameVector);

    string GetPrimaryIndexName(string tableName);

    void TableAttributePrint(vector<string> *name);
};

#endif