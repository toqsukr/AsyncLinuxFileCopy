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
        path = readFilePath(_type == FileTypes::READ ? "Enter path file to be copied: " : "Enter target copying path: ");
        fstat(descriptor, &statistic);
    }

    public: struct stat getStatistic() {return statistic;}

    public: void openFile(int flag=O_RDONLY|O_NONBLOCK) {
        descriptor = open(path.c_str(), flag, 0666);
        if(descriptor == -1) {
            std::string message = type == FileTypes::READ ? "\nEntered incorrect read path! Try again." : "\nEntered incorrect write path! Try again.";
            std::cout << message << std::endl;
            path = readFilePath(type == FileTypes::READ ? "Enter path file to be copied: " : "Enter target copying path: ");
            openFile(flag);
        }
    }

    public: void closeFile() {
        close(descriptor);
    }

    public: std::string readFilePath(const std::string& message = "Enter file path: ") {
        return readConsole(message);
    };
};


#endif //OSI_FILE_H
