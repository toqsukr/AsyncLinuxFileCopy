#include <string.h>
#include <signal.h>
#include <vector>
#include <string>
#include <aio.h>
#include <atomic>
#include <chrono>

#include "FileManager.h"
#include "AsyncManager.h"
#include "util.h"

void completionHandler(sigval_t sigval);
void readAsync(int index);
void writeAsync(int index);      
void initAsyncOperation(int index, aiocb &operation);


/*
    Cтруктура для представления асинхронной операции

    int id                                  - идентификатор операции
    struct aiocb aio                        - структура упоравления асинхронным вводом/выводом
    char *buffer                            - буфер для данных
    bool writeOperation                     - используется как флаг
    bool firstRead=0                        - первичное чтение (типа тоже флаг)
    ssize_t bytesDeal=0                     - кол-во считанных байт
    struct aio_operation* nextOperation     - следующая асинхронная операция
*/

struct aio_operation {
    int id;
    struct aiocb aio;
    char *buffer;
    bool writeOperation = 0;
    bool firstRead = 0;
    ssize_t bytesDeal = 0;
    struct aio_operation* nextOperation;
};

FileManager *fileManager;                   // менеджер, предоставляющий интерфейс работы с source и target файлами
AsyncManager *asyncManager;                 // менеджер, предоставляющий интерфейс работы с асинхронными операциями
int operationCount;                         // заданное кол-во асинхронных операций
ssize_t fileSizeToCopy;                     // количество байт, которые осталось скопировать
int completedOperationCount;                // количество завершенных асинхронных операций
std::vector<aiocb> readList, writeList;     // блоки  управления  асинхронным  вводом-выводом
std::vector<std::string> bufferList;        // 
std::vector<aio_operation> operationList;   // список асинхронных операций


int main() {
    fileManager = new FileManager();
    asyncManager = new AsyncManager();
    operationCount = asyncManager->getOperationCount();
    operationList.resize(operationCount * 2);

    completedOperationCount = 0;                                                               

    fileManager->openReadFile();
    fileManager->openWriteFile();

    asyncManager->setSizeByOperation(fileManager->getReadFile()->getStatistic().st_size / operationCount);

    /*
        Инициализация асинхронных операций, добавление в соответствующие списки
    */

    for (int i = 0; i < operationCount * 2; i++) {
        memset(&operationList[i], 0, sizeof(aio_operation));
        
        if (i % 2 == 0)     readAsync(i);
        else    writeAsync(i);

        operationList[i].buffer = (char *)bufferList[(i / 2)].c_str();  // 
    }

    /*
        Запуск чтения всеми потоками
        После завершения чтения освободившийся поток приостанавливается и сразу же запускается на запись
    */

    for (int i = 0; i < operationCount; i++) {
        if (aio_read(&readList[i]) == -1) {
            printLastError();
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    while (completedOperationCount != operationCount) {
        usleep(1000);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "\nОперация копирования выполнилась за : " << elapsed.count() << " секунд." << std::endl;

    fileManager->closeReadFile();
    fileManager->closeWriteFile();

    return 0;
}

void completionHandler(sigval_t sigval) {
    auto operation = (struct aio_operation *) sigval.sival_ptr;
    auto *next = operation->nextOperation;
    bool isWriteOperation = operation->writeOperation;

    ssize_t fileSize = fileManager->getReadFile()->getStatistic().st_size;
    off_t totalBlockSizeInBytes = asyncManager->getTotalSize();

    ssize_t bytesToDo = aio_return(&operation->aio);

    if (isWriteOperation) {

        operation->bytesDeal += bytesToDo;

        operation->aio.aio_offset = operation->aio.aio_offset + totalBlockSizeInBytes * operationCount;

        // делаем смещение для данных которые записывают другие потоки
        next->aio.aio_offset += totalBlockSizeInBytes * operationCount;

        if (fileSize < totalBlockSizeInBytes + operation->aio.aio_offset) {
            operation->aio.aio_nbytes = fileSize - operation->aio.aio_offset;
            next->aio.aio_nbytes = fileSize - operation->aio.aio_offset;
        }

        if (operation->bytesDeal < asyncManager->getSizeByOperation()) {
            if (aio_read(&next->aio) == -1) printLastError();
        }
        else    completedOperationCount += 1;
        
    }
    else {
        operation->bytesDeal += bytesToDo;

        if (operation->firstRead == 0 && bytesToDo > 0) {
            operation->firstRead = 1;
        }

        if ((operation->firstRead == 0) || (bytesToDo > 0)) {
            if (aio_write(&next->aio) == -1) {
                printLastError();
            }
        }
        else    completedOperationCount += 1;
    }
}



void readAsync(int index) {
    off_t totalBlockSizeInBytes = asyncManager->getTotalSize();
    readList.push_back(aiocb());
    memset(&readList[index / 2], 0, sizeof(aiocb));
    readList[index / 2].aio_fildes = fileManager->getReadFile()->getDescriptor();
    operationList[index].writeOperation = 0;
    operationList[index].nextOperation = &operationList[index + 1];

    bufferList.push_back(std::string());                                // string() - Создает пустую строку длиной, состоящую из нуля символов
    bufferList[index / 2] = std::string(totalBlockSizeInBytes, ' ');    // string(block_size, ' ') - Заполняет строку block_size последовательных копий символа ' '
    bufferList[index / 2].clear();                                      // clear() - Удаляет содержимое строки, которая становится пустой строкой (длиной 0 символов)

    initAsyncOperation(index, readList[index / 2]);    
}

void writeAsync(int index) {
    writeList.push_back(aiocb());
    memset(&writeList[index / 2], 0, sizeof(aiocb));
    writeList[index / 2].aio_fildes = fileManager->getWriteFile()->getDescriptor();
    operationList[index].writeOperation = 1;
    operationList[index].buffer = operationList[index - 1].buffer;

    operationList[index].nextOperation = &operationList[index - 1];

    initAsyncOperation(index, writeList[index/2]);    
}


void initAsyncOperation(int index, aiocb &operation) {
    off_t totalBlockSizeInBytes = asyncManager->getTotalSize();

    ssize_t fileSize = fileManager->getReadFile()->getStatistic().st_size;

    operation.aio_buf = (void *)bufferList[index / 2].c_str();
    operation.aio_nbytes = totalBlockSizeInBytes;
    operation.aio_offset = totalBlockSizeInBytes * (index / 2);

    operation.aio_sigevent.sigev_notify = SIGEV_THREAD;
    operation.aio_sigevent.sigev_value.sival_ptr = &operationList[index];
    operation.aio_sigevent.sigev_notify_function = completionHandler;
    operation.aio_sigevent.sigev_notify_attributes = nullptr;

    if (totalBlockSizeInBytes > fileSize) {
        operation.aio_nbytes = fileSize;
    }

    operationList[index].aio = operation;
    operationList[index].id = index;
}
