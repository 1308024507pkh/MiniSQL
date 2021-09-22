#ifndef __Minisql__BufferManager__
#define __Minisql__BufferManager__

#include "Minisql.h"
#include <stdio.h>

#define BLOCK_LEN 4096
#define BLOCK_LEN_ENABLE 4088
//减去BlockHead
static int replaced_block = -1;

class BufferManager {
private:
    fileNode *fileHead;
    fileNode file_pool[MAX_FILE_NUM];
    blockNode block_pool[MAX_BLOCK_NUM];
    int current_block_num;
    int current_file_num;

    void InitBlock(blockNode &block);

    void InitFile(fileNode &file);

    void SetPin(blockNode &block, bool pin);

    void SetPin(fileNode &file, bool pin);

public:
    BufferManager();

    virtual ~BufferManager();

//        static int GetBlockSize() //用宏定义BLOCK_LEN_ENABLE
//        {
//            return BLOCK_LEN - sizeof(size_t);
//        }
    void SetDirty(blockNode &block);

    void SetUsingSize(blockNode &block, size_t usage);


    fileNode *GetFile(const char *fileName, bool if_pin = false);

    char *GetContent(blockNode &block);

    size_t GetUsingSize(blockNode &block);

    size_t GetUsingSize(blockNode *block);

    blockNode *GetBlock(fileNode *file, blockNode *position, bool if_pin = false);

    blockNode *GetNextBlock(fileNode *file, blockNode *block);

    blockNode *GetBlockHead(fileNode *file);

    blockNode *GetBlockByOffset(fileNode *file, int offestNumber);

    void DeleteFileNode(const char *fileName);//delete_fileNode
    void WriteToDiskAll();

    void WriteToDisk(const char *fileName, blockNode *block);
};

#endif


