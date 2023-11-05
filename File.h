#ifndef OSI_FILE_H
#define OSI_FILE_H

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"

enum class FileTypes {
    READ,
    WRITE
};


class File {
    private: int descriptor;
    private: std::string path;
    private: FileTypes type;
    private: struct stat statistic {};

    public: File(FileTypes _type) {
        descriptor = -1;
        type = _type;
    }

    public: struct stat getStatistic() {return statistic;}

    public: void openFile(int flag=O_RDONLY|O_NONBLOCK) {
        path = readFilePath(type == FileTypes::READ ? "\nEnter path file to be copied: " : "\nEnter target copying path: ");
        descriptor = open(path.c_str(), flag, 0666);
        if(descriptor == -1) {
            std::string message = type == FileTypes::READ ? "\nEntered incorrect read path! Try again." : "\nEntered incorrect write path! Try again.";
            std::cout << message << std::endl;
            openFile(flag);
        }
        else {
            std::cout << "File opened successful!" << std::endl;
            fstat(descriptor, &statistic);
        }
    }

    public: void closeFile() {
        close(descriptor);
    }

    public: std::string readFilePath(const std::string& message = "\nEnter file path: ") {
        return readConsole(message);
    };
};


#endif //OSI_FILE_H
