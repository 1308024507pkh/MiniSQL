#include "CatalogManager.h"
#include "BufferManager.h"
#include "IndexInfo.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include "Attribute.h"


CatalogManager::CatalogManager() {}

CatalogManager::~CatalogManager() {}

/*
 * 构建tableName的table。存放在tableName的二进制文件中
 * 输入参数为记录属性的vector，主键对应位置
 * 输出为是否正常执行
 */
int CatalogManager::Create_Table(string tableName, vector<Attribute> *attributeVector, int primaryKeyLocation = 0) {
    FILE *fp;
    fp = fopen(tableName.c_str(), "w+");
    if (fp == NULL) return 0;
    fclose(fp);
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr = bm.GetContent(*btmp);
        int *num = (int *) addr;
        *num = 0;// 0 record number
        addr += sizeof(int);
        *addr = primaryKeyLocation;//1 as what it says
        addr++;
        *addr = (*attributeVector).size();// 2 attribute number
        addr++;
        for (int i = 0; i < (*attributeVector).size(); i++) {
            memcpy(addr, &((*attributeVector)[i]), sizeof(Attribute));
            addr += sizeof(Attribute);
        }
        bm.SetUsingSize(*btmp,
                        bm.GetUsingSize(*btmp) + (*attributeVector).size() * sizeof(Attribute) + 2 + sizeof(int));
        bm.SetDirty(*btmp);
        return 1;
    }
    return 0;
}

/*
 * 删除文件名为tableName的文件
 */
int CatalogManager::Drop_Table(string tableName) {
    bm.DeleteFileNode(tableName.c_str());
    if (remove(tableName.c_str())) {
        return 0;
    }
    return 1;
}

/*
 * 创建Index，创建IndexInfo对象i,调用Create_AttrIndex函数给对应Attribute创建Index
 */
int CatalogManager::Create_Index(string indexName, string tableName, string Attribute, int type) {
    fileNode *ftmp = bm.GetFile("Indexes");
    blockNode *btmp = bm.GetBlockHead(ftmp);
    IndexInfo i(indexName, tableName, Attribute, type);
    while (true) {
        if (btmp == NULL) return 0;
        if (bm.GetUsingSize(*btmp) <= BLOCK_LEN_ENABLE - sizeof(IndexInfo)) {

            char *addr;
            addr = bm.GetContent(*btmp) + bm.GetUsingSize(*btmp);
            memcpy(addr, &i, sizeof(IndexInfo));
            bm.SetUsingSize(*btmp, bm.GetUsingSize(*btmp) + sizeof(IndexInfo));
            bm.SetDirty(*btmp);
            return this->Create_AttrIndex(tableName, Attribute, indexName);
        } else {
            btmp = bm.GetNextBlock(ftmp, btmp);
        }
    }
    return 0;
}

/*
 * 非API函数，创建Attribute的Index
 */
int CatalogManager::Create_AttrIndex(string tableName, string AttributeName, string indexName) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);

    if (btmp) {
        char *addr = bm.GetContent(*btmp);
        addr += 1 + sizeof(int);
        int size = *addr;
        addr++;
        Attribute *a = (Attribute *) addr;
        int i;
        for (i = 0; i < size; i++) {
            if (a->name == AttributeName) {
                a->index = indexName;
                bm.SetDirty(*btmp);
                break;
            }
            a++;
        }
        if (i < size) return 1;
        else return 0;
    }
    return 0;
}

/*
 * Drop Index，同时调用Drop_AttrIndex函数，Drop Attribute对应的Index
 */
int CatalogManager::Drop_Index(string index) {
    fileNode *ftmp = bm.GetFile("Indexes");
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr;
        addr = bm.GetContent(*btmp);
        IndexInfo *i = (IndexInfo *) addr;
        int j = 0;
        for (j = 0; j < (bm.GetUsingSize(*btmp) / sizeof(IndexInfo)); j++) {
            if ((*i).indexName == index) {
                break;
            }
            i++;
        }
        this->Drop_AttrIndex((*i).tableName, (*i).attribute, (*i).indexName);
        for (; j < (bm.GetUsingSize(*btmp) / sizeof(IndexInfo) - 1); j++) {
            (*i) = *(i + sizeof(IndexInfo));
            i++;
        }
        bm.SetUsingSize(*btmp, bm.GetUsingSize(*btmp) - sizeof(IndexInfo));
        bm.SetDirty(*btmp);
        return 1;
    }
    return 0;
}

/*
 * Drop表里对应的Attribute的Index
 */
int CatalogManager::Drop_AttrIndex(string tableName, string AttributeName, string indexName) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);

    if (btmp) {
        char *addr = bm.GetContent(*btmp);
        addr += (1 + sizeof(int));
        int size = *addr;
        addr++;
        Attribute *a = (Attribute *) addr;
        int i;
        for (i = 0; i < size; i++) {
            if (a->name == AttributeName) {
                if (a->index == indexName) {
                    a->index = "";
                    bm.SetDirty(*btmp);
                } else {
                    cout << "ERROR! Unknown index: " << indexName << " on " << tableName << "!" << endl;
                    cout << "Attribute: " << AttributeName << " on table " << tableName << " has index: " << a->index
                         << "!" << endl;
                }
                break;
            }
            a++;
        }
        if (i < size)
            return 1;
        else
            return 0;
    }
    return 0;
}

/*
 * 在DB files中查找tableName的文件
 */
int CatalogManager::Find_Table(string tableName) {
    FILE *fp;
    fp = fopen(tableName.c_str(), "r");
    if (fp == NULL) {
        return 0;
    } else {
        fclose(fp);
        return TABLE_FILE;
    }
}

/*
 * 在文件Index文件中查找，返回对应的宏定义标记。
 */
int CatalogManager::Find_Index(string fileName) {
    fileNode *ftmp = bm.GetFile("Indexes");
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr;
        addr = bm.GetContent(*btmp);
        IndexInfo *i = (IndexInfo *) addr;
        int flag = UNKNOWN_FILE;
        for (int j = 0; j < (bm.GetUsingSize(*btmp) / sizeof(IndexInfo)); j++) {
            if ((*i).indexName == fileName) {
                flag = INDEX_FILE;
                break;
            }
            i++;
        }
        return flag;
    }
    return 0;
}

/*
 * 返回对应Index的type
 */
int CatalogManager::Get_Indextype(string indexName) {
    fileNode *ftmp = bm.GetFile("Indexes");
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr;
        addr = bm.GetContent(*btmp);
        IndexInfo *i = (IndexInfo *) addr;
        for (int j = 0; j < (bm.GetUsingSize(*btmp) / sizeof(IndexInfo)); j++) {
            if ((*i).indexName == indexName) {
                return i->type;
            }
            i++;
        }
        return -2;
    }

    return -2;
}

/*
 * 返回所有的Index,储存在vector中
 */
int CatalogManager::Get_Indexes(vector<IndexInfo> *indexes) {
    fileNode *ftmp = bm.GetFile("Indexes");
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr;
        addr = bm.GetContent(*btmp);
        IndexInfo *i = (IndexInfo *) addr;
        for (int j = 0; j < (bm.GetUsingSize(*btmp) / sizeof(IndexInfo)); j++) {
            indexes->push_back((*i));
            i++;
        }
    }

    return 1;
}

/*
 * 返回tableName表的所有index,全部压到vector中
 */
int CatalogManager::Get_Indexes(string tableName, vector<string> *indexes) {
    fileNode *ftmp = bm.GetFile("Indexes");
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr;
        addr = bm.GetContent(*btmp);
        IndexInfo *i = (IndexInfo *) addr;
        for (int j = 0; j < (bm.GetUsingSize(*btmp) / sizeof(IndexInfo)); j++) {
            if ((*i).tableName == tableName) {
                (*indexes).push_back((*i).indexName);
            }
            i++;
        }
        return 1;
    }

    return 0;
}

/*
 * 删除tableName表格中deleteNum位置的记录
 */
int CatalogManager::Delete_Record(string tableName, int deleteNum) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);

    if (btmp) {

        char *addr = bm.GetContent(*btmp);
        int *recordNum = (int *) addr;
        if ((*recordNum) < deleteNum) {
            cout << "ERROR Delete_Record" << endl;
            return 0;
        } else
            (*recordNum) -= deleteNum;

        bm.SetDirty(*btmp);
        return *recordNum;
    }
    return 0;
}

/*
 * 在tableName表格中deleteNum位置的增加记录
 */
int CatalogManager::Insert_Record(string tableName, int recordNum) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);

    if (btmp) {

        char *addr = bm.GetContent(*btmp);
        int *originalRecordNum = (int *) addr;
        *originalRecordNum += recordNum;
        bm.SetDirty(*btmp);
        return *originalRecordNum;
    }
    return 0;
}

/*
 * 返回recordNum
 */
int CatalogManager::Get_RecordNum(string tableName) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    if (btmp) {
        char *addr = bm.GetContent(*btmp);
        int *recordNum = (int *) addr;
        return *recordNum;
    }
    return 0;
}

/*
 * 把所有的tableName中的Attribute存储到attributeVector中
 */
int CatalogManager::Get_Attribute(string tableName, vector<Attribute> *attributeVector) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);

    if (btmp) {

        char *addr = bm.GetContent(*btmp);
        addr += 1 + sizeof(int);
        int size = *addr;
        addr++;
        Attribute *a = (Attribute *) addr;
        for (int i = 0; i < size; i++) {
            attributeVector->push_back(*a);
            a++;
        }

        return 1;
    }
    return 0;
}

/*
 * 找到record对应的内容，在recordResult中返回
 */

void CatalogManager::Get_Record(string tableName, vector<string> *recordContent, char *recordResult) {
    vector<Attribute> attributeVector;
    Get_Attribute(tableName, &attributeVector);
    char *contentBegin = recordResult;

    for (int i = 0; i < attributeVector.size(); i++) {
        Attribute attribute = attributeVector[i];
        string content = (*recordContent)[i];
        int type = attribute.type;
        int typeSize = Get_Length(type);
        stringstream ss;
        ss << content;
        if (type == Attribute::TYPE_INT) {
            //if the content is a int
            int intTmp;
            ss >> intTmp;
            memcpy(contentBegin, ((char *) &intTmp), typeSize);
        } else if (type == Attribute::TYPE_FLOAT) {
            //if the content is a float
            float floatTmp;
            ss >> floatTmp;
            memcpy(contentBegin, ((char *) &floatTmp), typeSize);
        } else {
            //if the content is a string
            memcpy(contentBegin, content.c_str(), typeSize);
        }
        contentBegin += typeSize;
    }
    return;
}

/*
 * 返回tableName表格的length
 */
int CatalogManager::Get_Length(string tableName) {
    fileNode *ftmp = bm.GetFile(tableName.c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);

    if (btmp) {
        int length = 0;
        char *addr = bm.GetContent(*btmp);
        addr += 1 + sizeof(int);
        int size = *addr;
        addr++;
        Attribute *attr = (Attribute *) addr;
        for (int i = 0; i < size; i++) {
            if ((*attr).type == -1) {
                length += sizeof(float);
            } else if ((*attr).type == 0) {
                length += sizeof(int);
            } else if ((*attr).type > 0) {
                length += (*attr).type * sizeof(char);
            } else {
                cout << "ERROR! Data type!" << endl;
                return 0;
            }
            attr++;
        }
        return length;
    }
    return 0;
}

/*
 * 返回type类型数据对应的length
 */
int CatalogManager::Get_Length(int t) {
    if (t == Attribute::TYPE_INT) {
        return sizeof(int);
    } else if (t == Attribute::TYPE_FLOAT) {
        return sizeof(float);
    } else {
        return (int) sizeof(char[t]); // Note that the type stores in Attribute.h
    }
}
