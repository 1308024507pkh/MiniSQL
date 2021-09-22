#ifndef CATALOGMANAGER_H
#define CATALOGMANAGER_H

#include <string>
#include <vector>
#include "Attribute.h"
#include "BufferManager.h"
#include "IndexInfo.h"

#define UNKNOWN_FILE 1
#define TABLE_FILE 2
#define INDEX_FILE 3

using namespace std;


class CatalogManager {
private:
    //类内函数，非接口
    int Create_AttrIndex(string tableName, string AttributeName, string indexName);

    int Drop_AttrIndex(string tableName, string AttributeName, string indexName);

public:
    BufferManager bm;

    //接口函数
    CatalogManager();

    virtual ~CatalogManager();

    //Table相关构建函数
    int Create_Table(string tableName, vector<Attribute> *attributeVector, int primaryKeyLocation);

    int Find_Table(string tableName);   //查找对应的table文件
    int Drop_Table(string tableName);

    //Index相关构建函数
    int Create_Index(string indexName, string tableName, string attributeName, int type);

    int Find_Index(string indexName);   //查找对应的Index文件
    int Drop_Index(string index);

    //Record相关构建函数
    int Insert_Record(string tableName, int recordNum); // 插入记录
    int Delete_Record(string tableName, int deleteNum);// 删除记录

    //找特定Table的所有Index，存在vector数组中
    int Get_Indexes(string tableName, vector<string> *indexNameVector);

    //Overload function:找所有的Index，存在vector数组中
    int Get_Indexes(vector<IndexInfo> *indexs);

    //返回Index类型
    int Get_Indextype(string indexName);

    //返回特定table的所有attribute
    int Get_Attribute(string tableName, vector<Attribute> *attributeVector);

    //返回长度
    int Get_Length(string tableName);

    //Overload function：返回type对应长度
    int Get_Length(int type);

    //返回特定table的record，通过修改参数实现返回
    void Get_Record(string tableName, vector<string> *recordContent, char *recordResult);

    //返回所有record数目
    int Get_RecordNum(string tableName);
};


#endif