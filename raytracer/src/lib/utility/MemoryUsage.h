#pragma once
#include <cstdint>


uint64_t getMemoryUsage();

#if defined(_WIN64) || defined(_WIN32)
#include <exception>
#include "windows.h"
#include "psapi.h"

uint64_t getMemoryUsage()
{
  PROCESS_MEMORY_COUNTERS_EX pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)) == 0) {
    throw std::exception("Failed to retrieve memory info");
  }
  return pmc.PrivateUsage;
}

#elif defined(__unix__)

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdexcept>

int parseLine(char* line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p < '0' || *p > '9')
        p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

uint64_t getMemoryUsage()
{
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);

    if (result == -1) {
        throw std::runtime_error("Failed to retrieve memory info");
    }

    return result * 1024; //KB to bytes
}

#endif