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
int completedOperationCount;                // количество завершенных асинхронных операций (условие выхода)
std::vector<aiocb> readList, writeList;     // блоки  управления  асинхронным  вводом-выводом
std::vector<std::string> bufferList;        // список совместных буферов для пар операций чтения/записи
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
        Инициализация асинхронных операций, настройка буфера, объединение в пары, добавление в соответствующие списки
    */

    for (int i = 0; i < operationCount * 2; i++) {
        memset(&operationList[i], 0, sizeof(aio_operation));

        if (i % 2 == 0)     readAsync(i);
        else    writeAsync(i);

        /*
            Настройка общего буфера данных для соседних потоков
            Так как четные операции чтения, а нечетные - записи, то общий буфер для соседних потоков будет находится как i / 2
        */

        operationList[i].buffer = (char *)bufferList[(i / 2)].c_str();
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

        /*
            Смещение происходит таким образом, чтобы потоки не конфликтовали и не считывали одни и те же области файла
            Для этого у каждой операции свое смещение, которое позволяет считывать и записывать конкретные области файла не важно в каком порядке
        */

        operation->aio.aio_offset = operation->aio.aio_offset + totalBlockSizeInBytes * operationCount;

        // делаем смещение для данных которые записывают другие потоки
        next->aio.aio_offset += totalBlockSizeInBytes * operationCount;

        if (fileSize < totalBlockSizeInBytes + operation->aio.aio_offset) {
            operation->aio.aio_nbytes = fileSize - operation->aio.aio_offset;
            next->aio.aio_nbytes = fileSize - operation->aio.aio_offset;
        }

        /*
            Ловим конец файла по количеству считанных байт для конкретной операции
        */
        if (operation->bytesDeal < asyncManager->getSizeByOperation()) {
            if (aio_read(&next->aio) == -1) printLastError();                   // ставит запрос на асинхронное чтение в очередь
        }
        else    completedOperationCount += 1;
        
    }
    else {
        operation->bytesDeal += bytesToDo;

        if (operation->firstRead == 0 && bytesToDo > 0) {
            operation->firstRead = 1;
        }

        if ((operation->firstRead == 0) || (bytesToDo > 0)) {
            if (aio_write(&next->aio) == -1) {                                  // ставит запрос на асинхронную запись в очередь
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

    bufferList.push_back(std::string());                                        // string() - Создает пустую строку длиной, состоящую из нуля символов
    bufferList[index / 2] = std::string(totalBlockSizeInBytes, ' ');            // string(totalBlockSizeInBytes, ' ') - Заполняет строку totalBlockSizeInBytes последовательных копий символа ' '
    bufferList[index / 2].clear();                                              // clear() - Удаляет содержимое строки, которая становится пустой строкой (длиной 0 символов)

    initAsyncOperation(index, readList[index / 2]);    
}

void writeAsync(int index) {
    writeList.push_back(aiocb());
    memset(&writeList[index / 2], 0, sizeof(aiocb));
    writeList[index / 2].aio_fildes = fileManager->getWriteFile()->getDescriptor();
    operationList[index].writeOperation = 1;

    operationList[index].nextOperation = &operationList[index - 1];

    initAsyncOperation(index, writeList[index/2]);    
}


void initAsyncOperation(int index, aiocb &operation) {
    off_t totalBlockSizeInBytes = asyncManager->getTotalSize();

    ssize_t fileSize = fileManager->getReadFile()->getStatistic().st_size;

    operation.aio_buf = (void *)bufferList[index / 2].c_str();

    if (totalBlockSizeInBytes > fileSize) {
        operation.aio_nbytes = fileSize;
        operation.aio_offset = fileSize;
    } else {
        operation.aio_nbytes = totalBlockSizeInBytes;
        operation.aio_offset = totalBlockSizeInBytes * (index / 2);         // установка начального смещения для конкретной операции, чтобы исключить конфликты считывания одних и тех же данных
    }
    
    operation.aio_sigevent.sigev_notify = SIGEV_THREAD;                     // способ выполнения уведомления (в данном случае уведомляем процесс о завершении операции)
    operation.aio_sigevent.sigev_value.sival_ptr = &operationList[index];
    operation.aio_sigevent.sigev_notify_function = completionHandler;       // функция запуска потока (вызывается при уведомлении о завершении асинхронной операции)
    operation.aio_sigevent.sigev_notify_attributes = nullptr;


    operationList[index].aio = operation;
    operationList[index].id = index;
}
