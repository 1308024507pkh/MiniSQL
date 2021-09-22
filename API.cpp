#include "API.h"
#include "RecordManager.h"
#include "CatalogManager.h"
#include "IndexManager.h"

API::API() {}

API::~API() {}

/**
 * TableDrop
 * 删除名称为tableName的表
 * @param tableName: 表名
 */
void API::TableDrop(string tableName) {
    if (!TableExist(tableName))
        return;

    vector<string> indexName;

    GetIndexNameList(tableName, &indexName);

    for (int i = 0; i < indexName.size(); i++) {
        cout << indexName[i] << endl;
        IndexDrop(indexName[i]);
    }

    if (rm->tableDrop(tableName)) {
        cm->Drop_Table(tableName);
        cout << "Drop Table " << tableName << " Successfully" << endl;
    }
}


/**
 * TableCreate
 * 根据传参 创建一个表
 * @param tableName: 表名
 * @param attributeVector: 属性向量
 * @param primaryKeyName: 表的主键
 * @param primaryKeyLocation: 主键定位
 */
void API::TableCreate(string tableName, vector<Attribute> *attributes, string primaryKeyName, int primaryKeyLocation) {
    if (cm->Find_Table(tableName) == TABLE_FILE) {
        cout << "Table " << tableName << " Alredy Exists" << endl;
        return;
    }

    if (!rm->tableCreate(tableName)) {
        cout << "File Open ERROR !!" << endl;
    }
    cm->Create_Table(tableName, attributes, primaryKeyLocation);
    cout << "Create Table " << tableName << " Successfully" << endl;

    if (primaryKeyName.size() > 0) {
        string indexName = GetPrimaryIndexName(tableName);
        IndexCreate(indexName, tableName, primaryKeyName);
    }
}


/**
 * IndexDrop
 * 删除名为indexName的索引
 * @param indexName: 索引名称
 */
void API::IndexDrop(string indexName) {
    if (!cm->Find_Index(indexName)) {
        cout << "Index " << indexName << " Not Exists" << endl;
        return;
    }

    if (!rm->indexDrop(indexName)) {
        cout << "Fail to Drop Index " << indexName << endl;
        return;
    }

    int indexType = cm->Get_Indextype(indexName);

    if (indexType == -2) {
        printf("error\n");
        return;
    }

    cm->Drop_Index(indexName);

    im->DropIndex(rm->indexFileNameGet(indexName), indexType);
    cout << "Drop Index " << indexName << " Successfully";
}


/**
 * IndexCreate
 * 根据传参 创建一个索引
 * @param indexName: 索引名
 * @param tableName: 表名
 * @param attributeName: 属性名
 */
void API::IndexCreate(string indexName, string tableName, string attributeName) {
    if (cm->Find_Index(indexName) == INDEX_FILE) {
        cout << "Index " << indexName << " Already Exists" << endl;
        return;
    }

    if (!TableExist(tableName)) {
        cout << "Table " << tableName << " Not Exists";
        return;
    }

    vector<Attribute> attributes;
    cm->Get_Attribute(tableName, &attributes);
    int i;
    int type = 0;

    for (i = 0; i < attributes.size(); i++) {
        if (attributeName == attributes[i].name) {
            if (!attributes[i].ifUnique) {
                cout << "Attribute Not Unique!" << endl;
                return;
            }
            type = attributes[i].type;
            break;
        }
    }

    if (i == attributes.size()) {
        cout << "Attribute " << attributeName << " Not Found in table " << tableName << endl;
        return;
    }


    if (rm->indexCreate(indexName)) {
        cm->Create_Index(indexName, tableName, attributeName, type);

        int indexType = cm->Get_Indextype(indexName);
        if (indexType == -2) {
            cout << "error";
            return;
        }

        im->CreateIndex(rm->indexFileNameGet(indexName), indexType);

        rm->indexRecordAllAlreadyInsert(tableName, indexName);
        cout << "Create Index " << indexName << " Successfully";
    } else
        cout << "Fail to Create Index " << indexName << endl;
}


/**
 * RecordShow()
 * 展示所有记录及其数目
 * @param tableName: 表名
 * @param attributeNameVector: 属性名向量
 */
void API::RecordShow(string tableName, vector<string> *attributeNameVector) {
    vector<Condition> condition;
    RecordShow(tableName, attributeNameVector, &condition);
}


/**
 * RecordShow()
 * 展示所有符合属性状态的记录及其数目
 * @param tableName: 表名
 * @param attributeNameVector: 属性名向量
 * @param conditionVector: 状态向量
 */
void API::RecordShow(string tableName, vector<string> *attributeNameVector, vector<Condition> *conditionVector) {
    if (cm->Find_Table(tableName) == TABLE_FILE) {
        int num = 0;
        vector<Attribute> attributeVector;
        GetAttribute(tableName, &attributeVector);

        vector<string> allAttributeName;
        if (attributeNameVector == NULL) {
            for (Attribute attribute : attributeVector) {
                allAttributeName.insert(allAttributeName.end(), attribute.name);
            }

            attributeNameVector = &allAttributeName;
        }

        //print attribute name you want to show
        TableAttributePrint(attributeNameVector);

        for (string name : (*attributeNameVector)) {
            int i = 0;
            for (i = 0; i < attributeVector.size(); i++) {
                if (attributeVector[i].name == name) {
                    break;
                }
            }

            if (i == attributeVector.size()) {
                cout << "Attribute " << name << " Not Exists" << endl;
                return;
            }
        }

        int blockOffset = -1;
        if (conditionVector) {
            for (Condition condition : *conditionVector) {
                int i = 0;
                for (i = 0; i < attributeVector.size(); i++) {
                    if (attributeVector[i].name == condition.attributeName) {
                        if (condition.operate == Condition::OPERATOR_EQUAL && attributeVector[i].index != "") {
                            blockOffset = im->SearchIndex(rm->indexFileNameGet(attributeVector[i].index),
                                                          condition.value, attributeVector[i].type);
                        }
                        break;
                    }
                }

                if (i == attributeVector.size()) {
                    cout << "Attribute " << condition.attributeName << " Not Exists" << endl;
                    return;
                }
            }
        }

        if (blockOffset == -1)
            num = rm->recordAllShow(tableName, attributeNameVector, conditionVector);
        else
            num = rm->recordBlockShow(tableName, attributeNameVector, conditionVector, blockOffset);

        cout << num << " Records Selected" << endl;
    } else
        cout << "Table " << tableName << " Not Exists" << endl;
}


/**
 * RecordInsert()
 * 向tableName表中插入一条记录
 * @param tableName: 表名
 * @param recordContent: 记录内数据
 */
void API::RecordInsert(string tableName, vector<string> *recordContent) {
    if (!TableExist(tableName))
        return;

    string indexName;
    vector<Attribute> attributeVector;
    vector<Condition> conditionVector;

    GetAttribute(tableName, &attributeVector);

    for (int i = 0; i < attributeVector.size(); i++) {
        indexName = attributeVector[i].indexNameGet();
        if (indexName.size() > 0) {
            int blockoffest = im->SearchIndex(rm->indexFileNameGet(indexName), (*recordContent)[i],
                                              attributeVector[i].type);

            if (blockoffest != -1) {
                cout << "Fail to Insert : Index Value Exists" << endl;
                return;
            }
        } else if (attributeVector[i].ifUnique) {
            Condition condition(attributeVector[i].name, (*recordContent)[i], Condition::OPERATOR_EQUAL);
            conditionVector.insert(conditionVector.end(), condition);
        }
    }

    if (conditionVector.size() > 0) {
        for (int i = 0; i < conditionVector.size(); i++) {
            vector<Condition> conditionV;
            conditionV.insert(conditionV.begin(), conditionVector[i]);

            int recordConflictNum = rm->recordAllFind(tableName, &conditionV);
            if (recordConflictNum > 0) {
                cout << "Fail to Insert : Unique Value Exists" << endl;
                return;
            }

        }
    }

    char recordString[2000];
    memset(recordString, 0, 2000);

    cm->Get_Record(tableName, recordContent, recordString);

    int recordSize = cm->Get_Length(tableName);
    int blockOffset = rm->recordInsert(tableName, recordString, recordSize);

    if (blockOffset >= 0) {
        RecordIndexInsert(recordString, recordSize, &attributeVector, blockOffset);
        cm->Insert_Record(tableName, 1);
        cout << "Insert Into " << tableName << " Successfully" << endl;
    } else
        cout << "Fail to Insert Into " << tableName << endl;
}


/**
 * RecordDelete()
 * 删除表中所有记录
 * @param tableName: 表名
 */
void API::RecordDelete(string tableName) {
    vector<Condition> conditionVector;
    RecordDelete(tableName, &conditionVector);
}


/**
 * RecordDelete()
 * 删除表中符合查询条件的集合
 * @param tableName: 表名
 * @param conditionVector: 状态向量
 */
void API::RecordDelete(string tableName, vector<Condition> *conditionVector) {
    if (!TableExist(tableName))
        return;

    int number = 0;
    vector<Attribute> attributeVector;
    GetAttribute(tableName, &attributeVector);

    int blockOffset = -1;
    if (conditionVector) {
        for (Condition condition : *conditionVector) {
            if (condition.operate == Condition::OPERATOR_EQUAL) {
                for (Attribute attribute : attributeVector) {
                    if (attribute.index.size() > 0 && attribute.name == condition.attributeName) {
                        blockOffset = im->SearchIndex(rm->indexFileNameGet(attribute.index), condition.value,
                                                      attribute.type);
                    }
                }
            }
        }
    }


    if (blockOffset == -1)
        number = rm->recordAllDelete(tableName, conditionVector);
    else
        number = rm->recordBlockDelete(tableName, conditionVector, blockOffset);

    cm->Delete_Record(tableName, number);
    cout << "Delete " << number << " Records in Table " << tableName << endl;
}


/**
 * GetRecordNum()
 * 获取tableName表中记录总条数
 * @param tableName: 表名
 */
int API::GetRecordNum(string tableName) {
    if (!TableExist(tableName)) {
        cout << "Table " << tableName << "Not Exists" << endl;
        return 0;
    } else
        return cm->Get_RecordNum(tableName);
}

int API::GetRecordSize(string tableName) {
    if (!TableExist(tableName)) {
        cout << "Table " << tableName << "Not Exists" << endl;
        return 0;
    } else
        return cm->Get_Length(tableName);
}

/**
 * GetTypeSize()
 * 得到类型的大小
 * @param type:  属性类型
 */
int API::GetTypeSize(int type) {
    return cm->Get_Length(type);
}


/**
 * GetIndexNameList()
 * 得到tableName表中索引的名单
 * @param tableName:  表名
 * @param indexNameVector:  保存索引名的向量指针
 */
int API::GetIndexNameList(string tableName, vector<string> *indexNameVector) {
    if (!TableExist(tableName)) {
        cout << "Table " << tableName << "Not Exists" << endl;
        return 0;
    } else
        return cm->Get_Indexes(tableName, indexNameVector);
}


/**
 * GetIndexAddress()
 * 得到索引文件中的所有索引名
 * @param indexNameVector: 保存索引名的向量指针
 */
void API::GetIndexAddress(vector<IndexInfo> *indexNameVector) {
    cm->Get_Indexes(indexNameVector);
    for (int i = 0; i < (*indexNameVector).size(); i++)
        (*indexNameVector)[i].indexName = rm->indexFileNameGet((*indexNameVector)[i].indexName);
}


/**
 * GetAttribute()
 * 得到表中所有属性的类型
 * @param tableName:  表名
 * @param attributeNameVector:  保存属性类型的向量指针
 */
int API::GetAttribute(string tableName, vector<Attribute> *attributeVector) {
    if (!TableExist(tableName)) {
        cout << "Table " << tableName << "Not Exists" << endl;
        return 0;
    } else
        return cm->Get_Attribute(tableName, attributeVector);
}


/**
 * RecordIndexInsert()
 * 将一条记录的所有索引值插入到索引树中
 * @param recordBegin: 记录起始点
 * @param recordSize: 记录大小
 * @param attributeVector:  属性类型的向量指针
 * @param blockOffset: 区块偏移值
 */
void API::RecordIndexInsert(char *recordBegin, int recordSize, vector<Attribute> *attributeVector, int blockOffset) {
    char *contentBegin = recordBegin;
    for (int i = 0; i < (*attributeVector).size(); i++) {
        int type = (*attributeVector)[i].type;
        int typeSize = GetTypeSize(type);
        if ((*attributeVector)[i].index.size()) {
            IndexInsert((*attributeVector)[i].index, contentBegin, type, blockOffset);
        }

        contentBegin += typeSize;
    }
}


/**
 * IndexInsert()
 * 向索引树中插入值
 * @param indexName: 索引名
 * @param contentBegin: 内容地址
 * @param type: 内容类型
 * @param blockOffset: 区块偏移值
 */
void API::IndexInsert(string indexName, char *contentBegin, int type, int blockOffset) {
    string content = "";
    stringstream tmp;

    if (type == Attribute::TYPE_INT) {
        int value = *((int *) contentBegin);
        tmp << value;
    } else if (type == Attribute::TYPE_FLOAT) {
        float value = *((float *) contentBegin);
        tmp << value;
    } else {
        char value[255];
        memset(value, 0, 255);
        memcpy(value, contentBegin, sizeof(type));
        string stringTmp = value;
        tmp << stringTmp;
    }
    tmp >> content;
    im->InsertIndex(rm->indexFileNameGet(indexName), content, blockOffset, type);
}


/**
 * RecordIndexDelete()
 * 删除索引树中一条记录的所有索引值
 * @param recordBegin: 记录起始点
 * @param recordSize: 记录大小
 * @param attributeVector:  属性类型的向量指针
 * @param blockOffset: 区块偏移值
 */
void API::RecordIndexDelete(char *recordBegin, int recordSize, vector<Attribute> *attributeVector, int blockOffset) {
    char *contentBegin = recordBegin;
    for (int i = 0; i < (*attributeVector).size(); i++) {
        int type = (*attributeVector)[i].type;
        int typeSize = GetTypeSize(type);

        string content = "";
        stringstream tmp;

        if ((*attributeVector)[i].index != "") {
            // 若属性有索引
            // 这里传*attributeVector)[i].index这个index的名字, blockOffset,还有值
            if (type == Attribute::TYPE_INT) {
                int value = *((int *) contentBegin);
                tmp << value;
            } else if (type == Attribute::TYPE_FLOAT) {
                float value = *((float *) contentBegin);
                tmp << value;
            } else {
                char value[255];
                memset(value, 0, 255);
                memcpy(value, contentBegin, sizeof(type));
                string stringTmp = value;
                tmp << stringTmp;
            }

            tmp >> content;
            im->DeleteIndex(rm->indexFileNameGet((*attributeVector)[i].index), content, type);

        }
        contentBegin += typeSize;
    }
}


/**
 * TableExist()
 * 按照tableName检测是否存在此表
 * @param tableName 表名
 */
int API::TableExist(string tableName) {
    if (cm->Find_Table(tableName) != TABLE_FILE) {
        cout << "Table " << tableName << " Not Exist" << endl;
        return 0;
    } else
        return 1;
}


/**
 * GetPrimaryIndexName()
 * 查询主索引名
 * @param tableName : 表名
 */
string API::GetPrimaryIndexName(string tableName) {
    return "PRIMARY_" + tableName;
}


/**
 * TableAttributePrint()
 * 打印出表的属性名
 * @param attributeNameVector: 指向存储属性名的向量
 */
void API::TableAttributePrint(vector<string> *attributeNameVector) {
    int i = 0;
    for (i = 0; i < (*attributeNameVector).size(); i++)
        cout << (*attributeNameVector)[i] << endl;
    if (i != 0)
        cout << endl;
}