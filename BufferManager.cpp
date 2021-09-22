#include "BufferManager.h"
#include "Minisql.h"
#include <stdlib.h>
#include <string>
#include <cstring>
#include <stack>

/*
 * 构造函数，构造BufferManager对象
 * 在构造函数中给file和block结构体中的成员分配内存空间
 * 调用了初始化函数做相应的初始化操作。
*/
BufferManager::BufferManager() {
    current_block_num = current_file_num = 0;
    fileHead = NULL;
    for (int i = 0; i < MAX_FILE_NUM; i++) {
        file_pool[i].fileName = new char[MAX_FILE_NAME];
        if (file_pool[i].fileName == NULL) {
            printf("File_pool ERROR!\n");
            exit(1);
        }
        InitFile(file_pool[i]);
    }
    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        block_pool[i].fileName = new char[MAX_FILE_NAME];
        if (block_pool[i].fileName == NULL) {
            printf("Block_pool ERROR! filename\n");
            exit(1);
        }
        block_pool[i].address = new char[BLOCK_LEN];
        if (block_pool[i].address == NULL) {
            printf("Block_pool ERROR! address\n");
            exit(1);
        }
        InitBlock(block_pool[i]);
    }
}

/* 析构函数，释放对应的内存空间 */
BufferManager::~BufferManager() {
    WriteToDiskAll();   //全部写回
    for (int i = 0; i < MAX_FILE_NUM; i++) delete[] file_pool[i].fileName;
    for (int i = 0; i < MAX_BLOCK_NUM; i++) delete[] block_pool[i].address;
}

/* 初始化file, 用memset函数给对应的内存赋值，初始化各参数。*/
void BufferManager::InitFile(fileNode &file) {
    file.nextFile = file.preFile = NULL;
    file.blockHead = NULL;
    file.pin = false;
    memset(file.fileName, 0, MAX_FILE_NAME);
}

/*
 * 初始化该block
 * 用memset函数给对应的内存赋值
 * 初始化对应的标记如pin,dirty等。
 */
void BufferManager::InitBlock(blockNode &block) {
    memset(block.address, 0, BLOCK_LEN);
    memset(block.fileName, 0, MAX_FILE_NAME);
    size_t init_usage = 0;
    memcpy(block.address, (char *) &init_usage, sizeof(size_t));
    block.using_size = sizeof(size_t);
    block.dirty = block.pin = block.reference = block.ifbottom = false;
    block.nextBlock = block.preBlock = NULL;
    block.offsetNum = -1;
}

/*
 * 首先判断输入的file是否在file的list中
 * 用一个指针从头开始遍历fileNode通过指针连成的链表，如果找到了则说明需要的file在list中，直接停止遍历, 返回回对应的指针即可。
 * 若没有找到file，则说明fileNode不在List中。
 * 考虑当前list中的file数目，如果filelist为空，则创建filelist, 并且加入当前文件；
 * 如果fileNode的数目小于MAX_FILE_NUM，则将当前的文件放置于list的末尾；
 * 如果fileNode数目大于最大值，则需要找到一个file作为替换。
 * 这个file是没有被锁定（pin）的第一个file，替换时调用WriteToDisk函数将该file的所有block全部写回磁盘。
 */
fileNode *BufferManager::GetFile(const char *fileName, bool if_pin) {
    blockNode *btmp = NULL;
    fileNode *ftmp = NULL;
    if (fileHead != NULL) {
        for (ftmp = fileHead; ftmp != NULL; ftmp = ftmp->nextFile) {
            //已经在list中
            if (!strcmp(fileName, ftmp->fileName)) {
                ftmp->pin = if_pin;
                return ftmp;
            }
        }
    }
    // 不在list中，且空
    if (current_file_num == 0) {
        ftmp = &file_pool[current_file_num];
        current_file_num++;
        fileHead = ftmp;
    }
        // 有多的位置，加在末尾
    else if (current_file_num < MAX_FILE_NUM) {
        ftmp = &file_pool[current_file_num];
        file_pool[current_file_num - 1].nextFile = ftmp;
        ftmp->preFile = &file_pool[current_file_num - 1];
        current_file_num++;
    }
        // 装不下了找到代替的文件，把对应的block写回磁盘
    else {
        ftmp = fileHead;
        while (ftmp->pin) {
            if (ftmp->nextFile)ftmp = ftmp->nextFile;
            else {
                printf("ERROR! File node insufficient");
                exit(2);
            }
        }
        for (btmp = ftmp->blockHead; btmp != NULL; btmp = btmp->nextBlock) {
            if (btmp->preBlock) {
                InitBlock(*(btmp->preBlock));
                current_block_num--;
            }
            //写回
            WriteToDisk(btmp->fileName, btmp);
        }
        InitFile(*ftmp);
    }
    if (strlen(fileName) + 1 > MAX_FILE_NAME) {
        printf("ERROR FILE_NAME%d\n", MAX_FILE_NAME);
        exit(3);
    }
    strncpy(ftmp->fileName, fileName, MAX_FILE_NAME);
    SetPin(*ftmp, if_pin);
    return ftmp;
}

/*
 * 如果block已经在list中，则直接返回这个block节点。
 * 如果block不在list中，则采用LRU替换算法。
 * 从链表头开始遍历，找到第一个没有reference标记并且没有被pin的缓冲块，该缓冲块就是被替换的缓冲块。
 * 并且在遍历的过程中把碰到的所有reference的标记清除，用于考虑下一次替换。
 * 把输入的block替换进去，并对替换出的block做标记，只有替换出的block是dirty的，才会调用WriteToDisk函数将其写回磁盘。
 */
blockNode *BufferManager::GetBlock(fileNode *file, blockNode *position, bool if_pin) {
    const char *fileName = file->fileName;
    blockNode *btmp = NULL;
    // 空的
    if (current_block_num == 0) {
        btmp = &block_pool[0];
        current_block_num++;
    }
        // 没满
    else if (current_block_num < MAX_BLOCK_NUM) {
        for (int i = 0; i < MAX_BLOCK_NUM; i++) {
            if (block_pool[i].offsetNum == -1) {
                btmp = &block_pool[i];
                current_block_num++;
                break;
            } else continue;
        }
    }
        // 满了，用LRU来替换
    else {
        int i = replaced_block;
        while (true) {
            i++;
            if (i >= current_block_num) i = 0;
            if (!block_pool[i].pin) {
                // 使用过的，这次以后就改为false，这次先跳过
                if (block_pool[i].reference == true)
                    block_pool[i].reference = false;
                    // 当前所指的块作为替换
                else {
                    btmp = &block_pool[i];
                    if (btmp->nextBlock) btmp->nextBlock->preBlock = btmp->preBlock;
                    if (btmp->preBlock) btmp->preBlock->nextBlock = btmp->nextBlock;
                    if (file->blockHead == btmp) file->blockHead = btmp->nextBlock;
                    //
                    replaced_block = i;

                    WriteToDisk(btmp->fileName, btmp);
                    InitBlock(*btmp);
                    break;
                }
            } else // 被锁了
                continue;
        }
    }
    // 加进去
    if (position != NULL && position->nextBlock == NULL) {
        btmp->preBlock = position;
        position->nextBlock = btmp;
        btmp->offsetNum = position->offsetNum + 1;
    } else if (position != NULL && position->nextBlock != NULL) {
        btmp->preBlock = position;
        btmp->nextBlock = position->nextBlock;
        position->nextBlock->preBlock = btmp;
        position->nextBlock = btmp;
        btmp->offsetNum = position->offsetNum + 1;
    }
        // 头结点
    else {
        btmp->offsetNum = 0;
        if (file->blockHead) {
            file->blockHead->preBlock = btmp;
            btmp->nextBlock = file->blockHead;
        }
        file->blockHead = btmp;
    }
    SetPin(*btmp, if_pin);
    if (strlen(fileName) + 1 > MAX_FILE_NAME) {
        // 文件名太长了
        printf("The MAX_FILE_NAME is %d!!\n", MAX_FILE_NAME);
        exit(3);
    }
    strncpy(btmp->fileName, fileName, MAX_FILE_NAME);

    //读入文件内容
    FILE *fileHandle;
    if ((fileHandle = fopen(fileName, "ab+")) != NULL) {
        if (fseek(fileHandle, btmp->offsetNum * BLOCK_LEN, 0) == 0) {
            if (fread(btmp->address, 1, BLOCK_LEN, fileHandle) == 0)
                btmp->ifbottom = true;
            btmp->using_size = GetUsingSize(btmp);
        } else {
            printf("Reading file %s error: Seeking", fileName);
            exit(1);
        }
        fclose(fileHandle);
    } else {
        printf("Reading file %s error: Opening", fileName);
        exit(1);
    }
    return btmp;
}


/*
 * 插入当前block
 * 如果block已经在list中存在，并且nextBlock的指针非空则返回下一个block
 * 如果block不存在则调用GetBlock函数，插入block并且返回其返回值。
 */
blockNode *BufferManager::GetNextBlock(fileNode *file, blockNode *block) {
    if (block->nextBlock == NULL) {
        if (block->ifbottom) block->ifbottom = false;
        return GetBlock(file, block);
    } else {
        if (block->offsetNum == block->nextBlock->offsetNum - 1) {
            return block->nextBlock;
        } else {
            return GetBlock(file, block);
        }
    }
}

/*设置block的pin值将该函数封装在类内，防止外部直接调用，形成一定的独立和保护。*/
void BufferManager::SetPin(blockNode &block, bool pin) {
    block.pin = pin;
    if (!pin)
        block.reference = true;
}

void BufferManager::SetPin(fileNode &file, bool pin) {
    file.pin = pin;
}

/*
 * 如果该文件存在blockhead，则返回
 * 否则调用GetBlock(file,NULL)并且返回对应的blockNode指针。
 */
blockNode *BufferManager::GetBlockHead(fileNode *file) {
    blockNode *btmp = NULL;
    if (file->blockHead != NULL) {
        if (file->blockHead->offsetNum == 0) {
            btmp = file->blockHead;
        } else {
            btmp = GetBlock(file, NULL);
        }
    } else {
        btmp = GetBlock(file, NULL);
    }
    return btmp;
}

/*
 * 根据偏移量找block，如果offset为0直接调用GetBlockHead(file)
 * 否则用循环不断调用GetNextBlock(file, btmp)并且让offset递减。
 * 直到最终offset为0
 */
blockNode *BufferManager::GetBlockByOffset(fileNode *file, int offset) {
    blockNode *btmp = NULL;
    if (offset == 0) return GetBlockHead(file);
    else {
        btmp = GetBlockHead(file);
        while (offset > 0) {
            btmp = GetNextBlock(file, btmp);
            offset--;
        }
        return btmp;
    }
}

/* 修改块的时候必须要调用 */
void BufferManager::SetDirty(blockNode &block) {
    block.dirty = true;
}

/* 重载函数，以引用作为参数的是作为API使用，后者仅在BufferManager类中使用 */
size_t BufferManager::GetUsingSize(blockNode &block) {
    return block.using_size;
}

size_t BufferManager::GetUsingSize(blockNode *block) {
    return *(size_t *) block->address;
}

void BufferManager::SetUsingSize(blockNode &block, size_t usage) {
    block.using_size = usage;
    memcpy(block.address, (char *) &usage, sizeof(size_t));
}

char *BufferManager::GetContent(blockNode &block) {
    return block.address + sizeof(size_t);
}

/*
 * 当且仅当block是dirty时才会被写回。
 * 打开fileName对应的file,用rb+的方式二进制写入文件。
 */
void BufferManager::WriteToDisk(const char *fileName, blockNode *block) {
    // 没有被改过
    if (!block->dirty) {
        return;
    } else {
        FILE *fileHandle = NULL;
        if ((fileHandle = fopen(fileName, "rb+")) != NULL) {
            if (fseek(fileHandle, block->offsetNum * BLOCK_LEN, 0) == 0) {
                // fwrite 二进制方式写入。
                if (fwrite(block->address, block->using_size + sizeof(size_t), 1, fileHandle) != 1) {
                    printf("ERROR! Writing file %s in to disk", fileName);
                    exit(1);
                }
            } else {
                printf("ERROR! Seeking file %s in to disk", fileName);
                exit(1);
            }
            fclose(fileHandle);
        } else {
            printf("ERROR! Opening file %s in to disk", fileName);
            exit(1);
        }
    }
}

/*
 * 从FileHead开始，遍历file list中的所有file以及内部的block
 * 将其全部写回磁盘中（调用WriteToDisk函数）。
 */
void BufferManager::WriteToDiskAll() {
    blockNode *btmp = NULL;
    fileNode *ftmp = NULL;
    if (fileHead) {
        for (ftmp = fileHead; ftmp != NULL; ftmp = ftmp->nextFile) {
            if (ftmp->blockHead) {
                for (btmp = ftmp->blockHead; btmp != NULL; btmp = btmp->nextBlock) {
                    if (btmp->preBlock)InitBlock(*(btmp->preBlock));
                    WriteToDisk(btmp->fileName, btmp);
                }
            }
        }
    }
}

/*
 * 删除文件以及对应的block值，循环调用GetNextBlock函数，
 * 将所有的block压入一个stack，并依次删除，更新current_block_num，
 * 最终删除文件，更新current_file_num。
 */
void BufferManager::DeleteFileNode(const char *fileName) {
    fileNode *ftmp = GetFile(fileName);
    blockNode *btmp = GetBlockHead(ftmp);
    stack<blockNode *> blockQ;
    while (true) {
        if (btmp == NULL) return;
        blockQ.push(btmp);
        if (btmp->ifbottom) break;
        btmp = GetNextBlock(ftmp, btmp);
    }
    current_block_num -= blockQ.size();
    while (!blockQ.empty()) {
        InitBlock(*blockQ.top());
        blockQ.pop();
    }
    if (ftmp->preFile) ftmp->preFile->nextFile = ftmp->nextFile;
    if (ftmp->nextFile) ftmp->nextFile->preFile = ftmp->preFile;
    if (fileHead == ftmp) fileHead = ftmp->nextFile;
    InitFile(*ftmp);
    current_file_num--;
}








