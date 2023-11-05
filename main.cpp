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

#include "Copier.h"

int main() {
    auto copier = new Copier();
    return 0;
}

