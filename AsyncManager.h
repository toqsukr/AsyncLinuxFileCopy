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
    private: int operationCount;
    private: int blockSize;
    private: std::vector<aiocb> readList, writeList; // блоки  управления  асинхронным  вводом-выводом
    private: std::vector<std::string> bufferList;
    private: std::vector<aio_operation> operationList;

    public: AsyncManager() {
        setOperationCount();
        setBlockSize();
        operationList.resize(2 * operationCount);
    }

    public: int setOperationCount(const std::string& message = "\nEnter async operation count (1, 2, 4, 8, 12, 16): ") {
        int value = std::stoi(readConsole(message));
        if((value != 1) && (value != 2) && (value != 4) && (value != 8) && (value != 12) && (value != 16)) {
            setOperationCount("\nEntered incorrect operation count! Try again.");
        }
        operationCount = value;
    }

    public: int setBlockSize(const std::string& message = "\nEnter block size (1, 2, 4, 8, 12, 16, 32): ") {
        int value = std::stoi(readConsole(message));
        if((value != 1) &&(value != 2) &&(value != 4) &&(value != 8) &&(value != 12) &&(value != 16) && (value != 32)) {
            setBlockSize("\nEntered incorrect block size! Try again.");
        }
        blockSize = value;
    }

    public: int getTotalSize() {return operationCount * blockSize;}

};


#endif //OSI_ASYNCMANAGER_H
