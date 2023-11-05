#define _GNU_SOURCE
#include <iostream>
#include <aio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <stdint.h>
#include <inttypes.h>

#include <vector>
#include <chrono>

#include "FileManager.h"
#include "AsyncManager.h"

int main() {
        auto *asyncManager = new AsyncManager();
        std::cout << asyncManager->getOperationCount() << '\t' << asyncManager->getTotalSize() << std::endl;
//    auto *fileManager = new FileManager();
//    fileManager->openReadFile();
//    fileManager->openWriteFile();
//
//    std::cout << fileManager->getReadFile()->getStatistic().st_size << '\t' << fileManager->getWriteFile()->getStatistic().st_size << std::endl;
//
//    fileManager->closeReadFile();
//    fileManager->closeWriteFile();
    return 0;
}

