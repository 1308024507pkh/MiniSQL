//RecordManager.cpp

#include "RecordManager.h"
#include "API.h"


//create a table
//创建表
//table_name: 表名
int RecordManager::tableCreate(string table_name) {
    string tableFileName = tableFileNameGet(table_name);
    FILE *fp = fopen(tableFileName.c_str(), "w+");
    if (fp == NULL) return 0;
    fclose(fp);
    return 1;
}

//drop a table
//删除表
//table_name: 表名
int RecordManager::tableDrop(string table_name) {
    string tableFileName = tableFileNameGet(table_name);
    bm.DeleteFileNode(tableFileName.c_str());
    if (remove(tableFileName.c_str())) return 0;
    return 1;
}

//create a index
//创建索引
//index_name: 索引名
int RecordManager::indexCreate(string index_name) {
    string indexFileName = indexFileNameGet(index_name);
    FILE *fp = fopen(indexFileName.c_str(), "w+");
    if (fp == NULL) return 0;
    fclose(fp);
    return 1;
}

//drop a index
//删除索引
//index_name: 索引名
int RecordManager::indexDrop(string index_name) {
    string indexFileName = indexFileNameGet(index_name);
    bm.DeleteFileNode(indexFileName.c_str());
    if (remove(indexFileName.c_str())) return 0;
    return 1;
}

//insert a record to table
//把一条记录插入到表中
//table_name: 表名
//record: 记录的内容
//size_of_recourd: 记录的size
//return the position of block in the file(-1 表示出错)
int RecordManager::recordInsert(string table_name, char *record, int size_of_recourd) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    while (1) {
        if (btmp == NULL) return -1;
        if (bm.GetUsingSize(*btmp) <= BLOCK_LEN_ENABLE - size_of_recourd) {
            char *addressBegin;
            addressBegin = bm.GetContent(*btmp) + bm.GetUsingSize(*btmp);
            memcpy(addressBegin, record, size_of_recourd);
            bm.SetUsingSize(*btmp, bm.GetUsingSize(*btmp) + size_of_recourd);
            bm.SetDirty(*btmp);
            return btmp->offsetNum;
        } else btmp = bm.GetNextBlock(ftmp, btmp);
    }
}

//print all record of a table meet requirement
//显示表中所有符合条件的记录
//table_name: 表名
//attribute_name_vector: 属性名列表
//condition_vector: 条件列表
//return int: the number of the record meet requirements(-1 表示出错)
int RecordManager::recordAllShow(string table_name, vector<string> *attribute_name_vector,
                                 vector<Condition> *condition_vector) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    int count = 0;
    while (true) {
        if (btmp == NULL) return -1;
        if (btmp->ifbottom) {
            int recordBlockNum = recordBlockShow(table_name, attribute_name_vector, condition_vector, btmp);
            count += recordBlockNum;
            return count;
        } else {
            int recordBlockNum = recordBlockShow(table_name, attribute_name_vector, condition_vector, btmp);
            count += recordBlockNum;
            btmp = bm.GetNextBlock(ftmp, btmp);
        }
    }
}

//print record of a table in a block
//table_name: 表名
//attribute_name_vector: 属性名列表
//condition_vector: 条件列表
//block_offset: block偏移量
//return int: the number of the record meet requirements in the block(-1 表示出错)
int RecordManager::recordBlockShow(string table_name, vector<string> *attribute_name_vector,
                                   vector<Condition> *condition_vector, int block_offset) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *block = bm.GetBlockByOffset(ftmp, block_offset);
    if (block == NULL) return -1;
    else return recordBlockShow(table_name, attribute_name_vector, condition_vector, block);

}

//print record of a table in a block
//table_name: 表名
//attribute_name_vector: 属性名列表
//condition_vector: 条件列表
//block: search record in the block
//return int: the number of the record meet requirements in the block(-1 表示出错)
int RecordManager::recordBlockShow(string table_name, vector<string> *attribute_name_vector,
                                   vector<Condition> *condition_vector, blockNode *block) {
    if (block == NULL) return -1;
    int count = 0;
    char *recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int size_of_recourd = api->GetRecordSize(table_name);
    api->GetAttribute(table_name, &attributeVector);
    char *blockBegin = bm.GetContent(*block);
    size_t usingSize = bm.GetUsingSize(*block);
    while (recordBegin - blockBegin < usingSize) {
        if (recordConditionFit(recordBegin, size_of_recourd, &attributeVector, condition_vector)) {
            count++;
            recordPrint(recordBegin, size_of_recourd, &attributeVector, attribute_name_vector);
            printf("\n");
        }
        recordBegin += size_of_recourd;
    }
    return count;
}

//find the number of all record of a table meet requirement
//table_name: 表名
//condition_vector: 条件列表
//return int: the number of the record meet requirements(-1 表示出错)
int RecordManager::recordAllFind(string table_name, vector<Condition> *condition_vector) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    int count = 0;
    while (true) {
        if (btmp == NULL) return -1;
        if (btmp->ifbottom) {
            int recordBlockNum = recordBlockFind(table_name, condition_vector, btmp);
            count += recordBlockNum;
            return count;
        } else {
            int recordBlockNum = recordBlockFind(table_name, condition_vector, btmp);
            count += recordBlockNum;
            btmp = bm.GetNextBlock(ftmp, btmp);
        }
    }
}

//find the number of record of a table in a block
//table_name: 表名
//block: search record in the block
//condition_vector: 条件列表
//return int: the number of the record meet requirements in the block(-1 表示出错)
int RecordManager::recordBlockFind(string table_name, vector<Condition> *condition_vector, blockNode *block) {
    if (block == NULL) return -1;
    int count = 0;
    char *recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int size_of_recourd = api->GetRecordSize(table_name);
    api->GetAttribute(table_name, &attributeVector);
    while (recordBegin - bm.GetContent(*block) < bm.GetUsingSize(*block)) {
        if (recordConditionFit(recordBegin, size_of_recourd, &attributeVector, condition_vector)) count++;
        recordBegin += size_of_recourd;
    }
    return count;
}

//delete all record of a table meet requirement
//table_name: 表名
//condition_vector: 条件列表
//return int: the number of the record meet requirements(-1 表示出错)
int RecordManager::recordAllDelete(string table_name, vector<Condition> *condition_vector) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    int count = 0;
    while (true) {
        if (btmp == NULL) return -1;
        if (btmp->ifbottom) {
            int recordBlockNum = recordBlockDelete(table_name, condition_vector, btmp);
            count += recordBlockNum;
            return count;
        } else {
            int recordBlockNum = recordBlockDelete(table_name, condition_vector, btmp);
            count += recordBlockNum;
            btmp = bm.GetNextBlock(ftmp, btmp);
        }
    }
}

//delete record of a table in a block
//table_name: 表名
//condition_vector: 条件列表
//block_offset: block偏移量
//return int: the number of the record meet requirements in the block(-1 表示出错)
int RecordManager::recordBlockDelete(string table_name, vector<Condition> *condition_vector, int block_offset) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *block = bm.GetBlockByOffset(ftmp, block_offset);
    if (block == NULL) return -1;
    else return recordBlockDelete(table_name, condition_vector, block);
}

//delete record of a table in a block
//table_name: 表名
//condition_vector: 条件列表
//block: search record in the block
//return int: the number of the record meet requirements in the block(-1 表示出错)
int RecordManager::recordBlockDelete(string table_name, vector<Condition> *condition_vector, blockNode *block) {
    if (block == NULL) return -1;
    int count = 0;
    char *recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int size_of_recourd = api->GetRecordSize(table_name);
    api->GetAttribute(table_name, &attributeVector);
    while (recordBegin - bm.GetContent(*block) < bm.GetUsingSize(*block)) {
        if (recordConditionFit(recordBegin, size_of_recourd, &attributeVector, condition_vector)) {
            count++;
            api->RecordIndexDelete(recordBegin, size_of_recourd, &attributeVector, block->offsetNum);
            int i = 0;
            for (i = 0;
                 i + size_of_recourd + recordBegin - bm.GetContent(*block) < bm.GetUsingSize(*block); i++)
                recordBegin[i] = recordBegin[i + size_of_recourd];
            memset(recordBegin + i, 0, size_of_recourd);
            bm.SetUsingSize(*block, bm.GetUsingSize(*block) - size_of_recourd);
            bm.SetDirty(*block);
        } else recordBegin += size_of_recourd;
    }
    return count;
}

//insert the index of all record of the table
//table_name: 表名
//index_name: 索引名
//return int: the number of the record meet requirements(-1 表示出错)
int RecordManager::indexRecordAllAlreadyInsert(string table_name, string index_name) {
    fileNode *ftmp = bm.GetFile(tableFileNameGet(table_name).c_str());
    blockNode *btmp = bm.GetBlockHead(ftmp);
    int count = 0;
    while (true) {
        if (btmp == NULL) return -1;
        if (btmp->ifbottom) {
            int recordBlockNum = indexRecordBlockAlreadyInsert(table_name, index_name, btmp);
            count += recordBlockNum;
            return count;
        } else {
            int recordBlockNum = indexRecordBlockAlreadyInsert(table_name, index_name, btmp);
            count += recordBlockNum;
            btmp = bm.GetNextBlock(ftmp, btmp);
        }
    }
}


//insert the index of a record of a table in a block
//table_name: 表名
//index_name: 索引名
//block: search record in the block
//return int: the number of the record meet requirements in the block(-1 表示出错)
int RecordManager::indexRecordBlockAlreadyInsert(string table_name, string index_name, blockNode *block) {
    if (block == NULL) return -1;
    int count = 0;
    char *recordBegin = bm.GetContent(*block);
    vector<Attribute> attributeVector;
    int size_of_recourd = api->GetRecordSize(table_name);
    api->GetAttribute(table_name, &attributeVector);
    int type;
    int typeSize;
    char *contentBegin;
    while (recordBegin - bm.GetContent(*block) < bm.GetUsingSize(*block)) {
        contentBegin = recordBegin;
        for (int i = 0; i < attributeVector.size(); i++) {
            type = attributeVector[i].type;
            typeSize = api->GetTypeSize(type);
            //找到了记录的index，并将它插入到indextree中
            if (attributeVector[i].index == index_name) {
                api->IndexInsert(index_name, contentBegin, type, block->offsetNum);
                count++;
            }
            contentBegin += typeSize;
        }
        recordBegin += size_of_recourd;
    }
    return count;
}

//judge if the record meet the requirement
//recordBegin: 一条记录的指针
//size_of_recourd: 记录的size
//attributeVector: 属性名列表 of the record
//condition_vector: 条件列表
//return bool: if the record fit the condition
bool RecordManager::recordConditionFit(char *recordBegin, int size_of_recourd, vector<Attribute> *attributeVector,
                                       vector<Condition> *condition_vector) {
    if (condition_vector == NULL) return true;
    int type;
    string attributeName;
    int typeSize;
    char content[255];
    char *contentBegin = recordBegin;
    for (int i = 0; i < attributeVector->size(); i++) {
        type = (*attributeVector)[i].type;
        attributeName = (*attributeVector)[i].name;
        typeSize = api->GetTypeSize(type);
        memset(content, 0, 255);
        memcpy(content, contentBegin, typeSize);
        for (int j = 0; j < (*condition_vector).size(); j++)
            if ((*condition_vector)[j].attributeName == attributeName)//如果这条记录有必要判断条件
                if (!contentConditionFit(content, type, &(*condition_vector)[j])) return false;
        //如果记录不符合条件
        contentBegin += typeSize;
    }
    return true;
}

//print 记录的内容
//recordBegin: point to a record
//size_of_recourd: 记录的size
//attributeVector: 属性名列表 of the record
//attributeVector: the name list of all attribute you want to print
void RecordManager::recordPrint(char *recordBegin, int size_of_recourd, vector<Attribute> *attributeVector,
                                vector<string> *attribute_name_vector) {
    int type;
    string attributeName;
    int typeSize;
    char content[255];
    char *contentBegin = recordBegin;
    for (int i = 0; i < attributeVector->size(); i++) {
        type = (*attributeVector)[i].type;
        typeSize = api->GetTypeSize(type);
        memset(content, 0, 255);
        memcpy(content, contentBegin, typeSize);
        for (int j = 0; j < (*attribute_name_vector).size(); j++)
            if ((*attribute_name_vector)[j] == (*attributeVector)[i].name) {
                contentPrint(content, type);
                break;
            }
        contentBegin += typeSize;
    }
}

//print value of content
//输出内容
//content: point to content
//type: type of content
void RecordManager::contentPrint(char *content, int type) {
    if (type == Attribute::TYPE_INT) {
        int tmp = *((int *) content);
        printf("%d ", tmp);
    } else if (type == Attribute::TYPE_FLOAT) {
        float tmp = *((float *) content);
        printf("%f ", tmp);
    } else {
        string tmp = content;
        printf("%s ", tmp.c_str());
    }
}

//judge if the content meet the requirement
//content: 内容的指针
//type: 内容类型
//condition: 条件
//return bool: the content if meet
bool RecordManager::contentConditionFit(char *content, int type, Condition *condition) {
    if (type == Attribute::TYPE_INT) {
        int tmp = *((int *) content);
        return condition->ifRight(tmp);
    } else if (type == Attribute::TYPE_FLOAT) {
        float tmp = *((float *) content);
        return condition->ifRight(tmp);
    } else return condition->ifRight(content);
}

//get a index's file name
//index_name: 索引名
string RecordManager::indexFileNameGet(string index_name) {
    string tmp = "";
    return "INDEX_FILE_" + index_name;
}

//get a table's file name
//table_name: 表名
string RecordManager::tableFileNameGet(string table_name) {
    string tmp = "";
    return tmp + "TABLE_FILE_" + table_name;
}