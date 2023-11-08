#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <vector>
#include <chrono>

#include "Copier.h"

int main() {
    auto copier = new Copier();
    copier->startCopying();
    return 0;
}

