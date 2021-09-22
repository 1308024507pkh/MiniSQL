#ifndef Minisql_Minisql_h
#define Minisql_Minisql_h

#include <stdio.h>
#include <time.h>
#include <iostream>

#define MAX_FILE_NUM 40
#define MAX_BLOCK_NUM 300
#define MAX_FILE_NAME 100

using namespace std;

struct blockNode {
    int offsetNum;
    bool pin;  //锁的标记
    bool ifbottom;
    char *fileName; //与fileNode的关联
    friend class BufferManager;
private:
    char *address; //内容地址
    blockNode *preBlock;
    blockNode *nextBlock;
    bool reference; //用于LRU的标记
    bool dirty; //用于脏块的标记
    size_t using_size;

};
struct fileNode {
    char *fileName;
    bool pin; //锁
    blockNode *blockHead;
    fileNode *nextFile;
    fileNode *preFile;
};

extern clock_t start;
extern void print();

#endif
