// UTD Summer 2020 Project - NTFS Filesystem
// Program 3 - Implementation of $MFT and file record methods
//
// Written by Dien Tran. Compile with a C++11 compiler.

#ifndef __linux
#error This program can only be compiled on Linux systems.
#endif

#if __cplusplus < 201103L
#error This program requires a C++11 or newer compiler.
#endif

#define _FILE_OFFSET_BITS 64 // for 64-bit off_t's

// Standard headers
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// Self-defined headers
#include "Constants.hpp"
#include "utility.hpp"

void displayMFTProperties(const NTFSVBR &vbr) {
    // Find start address of $MFT
    std::printf("$MFT address: 0x%llX\n",
                vbr.GetMFTLCN() * CLUSTER_SIZE * SECTOR_SIZE);

    // Find start address of $MFTMirr
    std::printf("$MFTMirr address: 0x%llX\n",
                vbr.GetMFTMirrLCN() * CLUSTER_SIZE * SECTOR_SIZE);
}

// work function.
int work(int fd) {
    // Read MBR
    unsigned char mbrArr[SECTOR_SIZE + 1];
    if (read(fd, mbrArr, SECTOR_SIZE + 1) < 0) {
        perror("read");
        return READ_ERROR;
    }

    // Create MBR object
    dkt::UString mbrStr(SECTOR_SIZE);
    for (size_t i = 0; i < SECTOR_SIZE; ++i) {
        mbrStr[i] = mbrArr[i];
    }
    MBR mbr(mbrStr);

    // Parse partition entries, and see which ones are NTFS
    dkt::EntryVector entries = mbr.ParseEntries();
    // ...but first, check if the disk is in GPT
    if (entries.size() && entries[0].GetPartitionType() == 0xEE) {
        // We do not deal with GPT disks. Terminate the program gracefully
        std::cout << "This disk is in GPT format, which is unsupported."
                  << '\n';
        return GPT_FORMATTED;
    }
    dkt::NTFSEntryVector NTFSEntries;
    NTFSEntries.reserve(4);
    for (size_t i = 0; i < entries.size(); ++i) {
        std::cout << "Partition " << i + 1 << ": ";
        try {
            // Attempt to create an `NTFSPartitionEntry` object
            NTFSPartitionEntry e(entries[i].GetEntry());
            NTFSEntries.push_back(e); // and push it into the NTFS array
            std::cout << "NTFS entry\n";
        } catch (std::invalid_argument &e) {
            std::cout << "Non-NTFS entry\n";
        }
    }
    std::cout << "\n"
              << NTFSEntries.size() << " NTFS partitions on opened device\n\n";

    // Read VBR for each NTFS partition
    dkt::VBRVector VBRs;
    VBRs.reserve(NTFSEntries.size());
    for (size_t i = 0; i < NTFSEntries.size(); ++i) {
        // Read VBR
        std::uint64_t vbrAddr =
            NTFSEntries[i].GetStartingSector() * SECTOR_SIZE;
        if (lseek(fd, vbrAddr, SEEK_SET) < 0) {
            perror("lseek");
            return LSEEK_ERROR;
        }
        unsigned char vbr[SECTOR_SIZE + 1];
        if (read(fd, vbr, SECTOR_SIZE + 1) < 0) {
            perror("read");
            return READ_ERROR;
        }
        dkt::UString vbrStr(SECTOR_SIZE);
        for (size_t j = 0; j < SECTOR_SIZE; ++j) {
            vbrStr[j] = vbr[j];
        }
        std::cout << "Partition " << i + 1 << ": ";
        try {
            // Attempt to create VBR from NTFS partition
            VBRs.push_back(NTFSVBR(vbrStr));
            std::cout << "valid VBR\n";
            displayMFTProperties(VBRs[i]);
            std::cout << '\n';
        } catch (std::invalid_argument &e) {
            std::cout << "invalid VBR\n";
        }
    }

    return 0;
}

// main function
int main(int argc, char **argv) {
    // Require 2 arguments
    if (argc != 2) {
        std::fprintf(stderr, "Expected 2 arguments, got %d\n", argc);
        std::exit(ARGUMENT_EXPECTED);
    }

    // Open device
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        std::perror("open");
        std::exit(OPEN_ERROR);
    }
    std::cout << argv[1] << " opened successfully\n\n";

    // Do work
    int workResult = work(fd);

    // Close device
    close(fd);

    // Exit with work's return code
    std::exit(workResult);
}