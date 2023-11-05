#ifndef OSI_FILEMANAGER_H
#define OSI_FILEMANAGER_H

#include "File.h"
#include <fcntl.h>


class FileManager {
    private: File *readFile;
    private: File *writeFile;

    public: FileManager() {
        readFile = new File(FileTypes::READ);
        writeFile = new File(FileTypes::WRITE);
    }

    public: File *getReadFile() {return readFile;}
    public: File *getWriteFile() {return writeFile;}


    public: void openReadFile() {
        readFile->openFile();
    }

    public: void openWriteFile() {
        writeFile->openFile(O_CREAT | O_WRONLY | O_TRUNC | O_NONBLOCK);
    }

    public: void closeReadFile() {
        readFile->closeFile();
    }

    public: void closeWriteFile() {
        writeFile->closeFile();
    }
};




#endif //OSI_FILEMANAGER_H
