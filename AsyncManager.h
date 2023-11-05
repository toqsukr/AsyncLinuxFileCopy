#ifndef OSI_ASYNCMANAGER_H
#define OSI_ASYNCMANAGER_H

#include <string>
#include "util.h"



class AsyncManager {
    private: off_t blockSize = 4096;
    private: int operationCount;
    private: int blockCount;

    public: AsyncManager() {
        setOperationCount();
        setBlockCount();
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
        int value = std::stoi(readConsole("\nEnter block count (1, 2, 4, 8, 12, 16, 32): "));
        if((value != 1) &&(value != 2) &&(value != 4) &&(value != 8) &&(value != 12) &&(value != 16) && (value != 32)) {
            std::cout << "\nEntered incorrect block count! Try again." << std::endl;
            setBlockCount();
        }
        else    blockCount = value;
    }

    public: int getTotalSize() {return blockSize * blockCount;}
};


#endif //OSI_ASYNCMANAGER_H
