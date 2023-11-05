#ifndef OSI_ASYNCMANAGER_H
#define OSI_ASYNCMANAGER_H

#include <string>
#include <vector>
#include <aio.h>
#include "util.h"

struct aio_operation {
    int id;
    struct aiocb aio;
    char *buffer;
    int writeOperation;
    int firstRead=0;
    int bytesDeal=0;
    struct aio_operation* nextOperation;
};

class AsyncManager {
    private: off_t blockSize = 4096;
    private: int operationCount;
    private: int blockCount;
    private: std::vector<aiocb> readList, writeList; // блоки  управления  асинхронным  вводом-выводом
    private: std::vector<std::string> bufferList;
    private: std::vector<aio_operation> operationList;

    public: AsyncManager() {
        setOperationCount();
        setBlockCount();
        operationList.resize(2 * operationCount);
    }

    public: int getOperationCount() {return operationCount;}
    public: int getBlockCount() {return blockCount;}

    public: int setOperationCount() {
        int value = std::stoi(readConsole("\nEnter async operation count (1, 2, 4, 8, 12, 16): "));
        if((value != 1) && (value != 2) && (value != 4) && (value != 8) && (value != 12) && (value != 16)) {
            std::cout << "\nEntered incorrect operation count! Try again." << std::endl;
            setOperationCount();
        }
        else    operationCount = value;
    }

    public: int setBlockCount() {
        int value = std::stoi(readConsole("\nEnter block size (1, 2, 4, 8, 12, 16, 32): "));
        if((value != 1) &&(value != 2) &&(value != 4) &&(value != 8) &&(value != 12) &&(value != 16) && (value != 32)) {
            std::cout << "\nEntered incorrect block size! Try again." << std::endl;
            setBlockCount();
        }
        else    blockCount = value;
    }

    public: int getTotalSize() {return blockSize * blockCount;}

};


#endif //OSI_ASYNCMANAGER_H
