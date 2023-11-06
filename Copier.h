#ifndef OSI_COPIER_H
#define OSI_COPIER_H

#include <string>
#include <vector>
#include <aio.h>
#include <atomic>

#include "FileManager.h"
#include "AsyncManager.h"
#include "util.h"

struct aio_operation {
    int id;
    struct aiocb aio;
    char *buffer;
    int writeOperation;
    int firstRead=0;
    ssize_t bytesDeal=0;
    struct aio_operation* nextOperation;
};

std::atomic<bool> shouldStop(false);

void sigtermHandler(int signum) {
    std::cout << "Received SIGTERM signal." << std::endl;
    shouldStop = true;
}

class Copier {
    struct CallbackData {
        aio_operation *operation;
        Copier *copier;
    };
    private: FileManager *fileManager;
    private: AsyncManager *asyncManager;
    private: ssize_t fileSizeToCopy;
    private: off_t sizeByOperation;
    private: std::vector<aiocb> readList, writeList; // блоки  управления  асинхронным  вводом-выводом
    private: std::vector<std::string> bufferList;
    private: std::vector<aio_operation> operationList;

    public: Copier() {
        asyncManager = new AsyncManager();
        fileManager = new FileManager();
        operationList.resize(asyncManager->getOperationCount() * 2);
    }

    public: FileManager *getFileManager() {return fileManager;}
    public: AsyncManager *getAsyncManager() {return asyncManager;}

    private: void readAsync(int index) {
            readList.push_back(aiocb());    // aiocb() - Создает блок управления асинхронным вводом-выводом

            memset(&readList[index/2], 0, sizeof(aiocb));
            readList[index/2].aio_fildes = fileManager->getReadFile()->getDescriptor(); // .aio_fildes - файловый дескриптор

            operationList[index].writeOperation = 0;
            operationList[index].nextOperation = &operationList[index + 1];

            bufferList.push_back(std::string()); // string() - Создает пустую строку длиной, состоящую из нуля символов
            bufferList[index / 2] = std::string(asyncManager->getTotalSize(), ' '); // string(block_size, ' ') - Заполняет строку block_size последовательных копий символа ' '
            bufferList[index / 2].clear(); // clear() - Удаляет содержимое строки, которая становится пустой строкой (длиной 0 символов)

            readList[index/2].aio_buf = (void *)bufferList[index/2].c_str(); // .aio_buf - расположение буфера
            readList[index/2].aio_nbytes = asyncManager->getTotalSize(); // .aio_nbytes - длина передачи
            readList[index/2].aio_offset = asyncManager->getTotalSize() * (index / 2); // .aio_offset - файловое смещение

            CallbackData callbackData{};
            callbackData.operation = &operationList[index];
            callbackData.copier = this;
            readList[index/2].aio_sigevent.sigev_notify = SIGEV_THREAD;
            readList[index/2].aio_sigevent.sigev_value.sival_ptr = new CallbackData(callbackData);
            readList[index/2].aio_sigevent.sigev_notify_function = completionHandler;
            readList[index/2].aio_sigevent.sigev_notify_attributes = nullptr;
            if (asyncManager->getTotalSize() > fileSizeToCopy) {
                readList[index/2].aio_nbytes = fileSizeToCopy;
            }

            operationList[index].aio = readList[index/2];
            operationList[index].id = index;
    }

    private: void writeAsync(int index) {
            writeList.push_back(aiocb());
            memset(&writeList[index / 2], 0, sizeof(aiocb));
            writeList[index / 2].aio_fildes = fileManager->getWriteFile()->getDescriptor();
            operationList[index].writeOperation = 1;
            operationList[index].buffer = operationList[index - 1].buffer;

            operationList[index].nextOperation = &operationList[index - 1]; // операция чтения и записи ссылаются друг на друга как следующая операция -_- (как будто тут можно ставить null)
            // напр. 0 ссылается на 1, а 1 на 0
            writeList[index/2].aio_buf = (void *)bufferList[index/2].c_str(); // тот же буфер что и на чтении
            writeList[index/2].aio_nbytes = asyncManager->getTotalSize();
            writeList[index/2].aio_offset = asyncManager->getTotalSize() * (index / 2);

            CallbackData callbackData{};
            callbackData.operation = &operationList[index];
            callbackData.copier = this;
            writeList[index/2].aio_sigevent.sigev_notify = SIGEV_THREAD;
            writeList[index/2].aio_sigevent.sigev_value.sival_ptr = new CallbackData(callbackData);
            writeList[index/2].aio_sigevent.sigev_notify_function = completionHandler;
            writeList[index/2].aio_sigevent.sigev_notify_attributes = nullptr;

            if (asyncManager->getTotalSize() > fileSizeToCopy) {
                writeList[index/2].aio_nbytes = fileSizeToCopy;
            }

            operationList[index].aio = writeList[index/2];
            operationList[index].id = index;
    }

    public: void startCopying() {
        fileManager->openReadFile();
        fileManager->openWriteFile();
        fileSizeToCopy = fileManager->getReadFile()->getStatistic().st_size;
        sizeByOperation = fileSizeToCopy / asyncManager->getOperationCount();
        signal(SIGTERM, sigtermHandler);
        for (int i = 0; i < asyncManager->getOperationCount() * 2; i++) {
            memset(&operationList[i], 0, sizeof(aio_operation));
            // Копирует значение 0 в каждый из первых символов sizeof(aio_operation) объекта, на который указывает &aio_op_list[i].
            if (i % 2 == 0) { // четные на чтение, нечетные на запись
                readAsync(i);
            }
            else {
                writeAsync(i);
            }
            operationList[i].buffer = (char *)bufferList[(i / 2)].c_str();
        }
        for (int i = 0; i < asyncManager->getOperationCount(); i++) {
            if (aio_read(&readList[i]) == -1) { // aio_read - Инициирует операцию асинхронного чтения
                printLastError();
            }
        }
        while (!shouldStop) {
            usleep(1000);
        }
        fileManager->closeReadFile();
        fileManager->closeWriteFile();
    }

public: static void completionHandler(sigval_t sigval) {
        auto *data = (struct CallbackData *) sigval.sival_ptr;
        auto operation = data->operation;
        auto *copier = data->copier;
        auto *next = operation->nextOperation;
        std::cout << "id: " << operation->id << std::endl;
        if (operation->writeOperation) {
            ssize_t bytesWritten = aio_return(&operation->aio);
            /*
            ssize_t - это то же самое, что size_t (тип используется для представления размера объектов в памяти), но способен представлять число -1
            aio_return() - возвращает статус асинхронной операции ввода-вывода
            */
            operation->bytesDeal += bytesWritten;
            copier->fileSizeToCopy -= bytesWritten;
            if (operation->bytesDeal < copier->sizeByOperation) {
                std::cout << operation->aio.aio_offset << std::endl;
                operation->aio.aio_offset = operation->aio.aio_offset + copier->asyncManager->getTotalSize() * copier->asyncManager->getOperationCount();
                // делаем смещение для данных которые записывают другие потоки
                next->aio.aio_offset = next->aio.aio_offset + copier->asyncManager->getTotalSize() * copier->asyncManager->getOperationCount();
                if (copier->fileSizeToCopy < copier->asyncManager->getTotalSize() + operation->aio.aio_offset) {
                    operation->aio.aio_nbytes = copier->fileSizeToCopy - operation->aio.aio_offset;
                    next->aio.aio_nbytes = copier->fileSizeToCopy - operation->aio.aio_offset;
                    if (aio_read(&next->aio) == -1) {
                        printLastError();
                    }
                }
                else {
                    if (aio_read(&next->aio) == -1) {
                        printLastError();
                    }
                }
            }
            else {
                copier->sizeByOperation += 1;
            }
        }
        else {
            ssize_t bytes_read = aio_return(&operation->aio);
            operation->bytesDeal += bytes_read;

            if (operation->firstRead == 0 && bytes_read > 0) {
                operation->firstRead = 1;
            }

            if ((operation->firstRead == 0) || (bytes_read > 0)) {
                //почему-то во время первого чтения нет bytes_read
                if (aio_write(&next->aio) == -1) {
                    printLastError();
                }
            }
            else {
                copier->sizeByOperation += 1;
                // Если смещение больше размера файла то завершаем процесс
            }
        }
    }


};





#endif //OSI_COPIER_H
