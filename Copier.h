#ifndef OSI_COPIER_H
#define OSI_COPIER_H

#include <string>
#include <vector>
#include <aio.h>

#include "FileManager.h"
#include "AsyncManager.h"

struct aio_operation {
    int id;
    struct aiocb aio;
    char *buffer;
    int writeOperation;
    int firstRead=0;
    int bytesDeal=0;
    struct aio_operation* nextOperation;
};


class Copier {
    private: FileManager *fileManager;
    private: AsyncManager *asyncManager;
    private: std::vector<aiocb> readList, writeList; // блоки  управления  асинхронным  вводом-выводом
    private: std::vector<std::string> bufferList;
    private: std::vector<aio_operation> operationList;

    public: Copier() {
        asyncManager = new AsyncManager();
        fileManager = new FileManager();
    }

    public: FileManager *getFileManager() {return fileManager;}
    public: AsyncManager *getAsyncManager() {return asyncManager;}
};


#endif //OSI_COPIER_H
