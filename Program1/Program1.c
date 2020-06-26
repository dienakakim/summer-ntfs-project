// UTD Summer 2020 Project - NTFS Filesystem
// Program 1 - Get to first NTFS partition's VBR
//
// Written by Dien Tran. Compile with a C99 compiler.

#ifndef __linux
#error This program can only be compiled on Linux systems.
#endif

#define _FILE_OFFSET_BITS 64 // for 64-bit off_t's

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h> // C99
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Constants
const int SECTOR_SIZE = 512;            // sector size
const int PARTITION_TABLE_OFFSET = 446; // partition table offset
const int PARTITION_TABLE_SIZE = 64;    // partition table size
const int PARTITION_ENTRY_SIZE = 16;    // partition entry size

// Structures
typedef struct {
    bool BootIndicator;      // byte 0: boot flag. 0x80 is bootable, 0x00 if not
    uint64_t StartingSector; // bytes 8-11 little-endian: starting sector of
                             // partition
    unsigned char PartitionType : 1; // byte 4: partition type
} PartitionEntry;

// Function prototypes
bool verifyNTFSVBR(unsigned char *);
int work(int);
PartitionEntry *newPartitionEntry(unsigned char *buf);
unsigned char *getVBR(PartitionEntry *, int);

// Driver function
int main(int argc, char **argv) {
    // Check argc
    if (argc != 2) {
        fprintf(stderr, "Expected 2 arguments, got %d\n", argc);
        exit(1);
    }

    // Attempt to open device
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        // Failed to open
        perror("open");
        exit(2);
    }
    printf("%s opened successfully\n", argv[1]);

    // Do work
    int status = work(fd);

    // Done, close device
    close(fd);
    exit(status);
}

// the main work function -- second function as specified in briefing
int work(int fd) {
    // Read first 512 bytes

    // MBR
    unsigned char mbr[SECTOR_SIZE + 1];

    // Read MBR
    ssize_t bytesRead = read(fd, mbr, SECTOR_SIZE + 1);
    if (bytesRead < 0) {
        perror("read");
        return -1;
    }

    // Print MBR
    printf("Master boot record:\n");
    int i, j;
    for (i = 0; i < SECTOR_SIZE; i++) {
        printf("%02X ", mbr[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n===============================================\n\n");

    // Print partition table
    printf("Partition table:\n");
    for (i = 0, j = PARTITION_TABLE_OFFSET;
         j < PARTITION_TABLE_OFFSET + PARTITION_TABLE_SIZE; ++i, ++j) {
        printf("%02X ", mbr[j]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n===============================================\n\n");

    // Construct partition entries
    PartitionEntry *partitions[4];
    for (int i = 0; i < 4; ++i) {
        unsigned char buf[16];
        for (int j = 0; j < 16; ++j) {
            buf[j] = mbr[PARTITION_TABLE_OFFSET + PARTITION_ENTRY_SIZE * i + j];
        }
        partitions[i] = newPartitionEntry(buf);
    }

    // Print VBRs
    unsigned char *VBRs[4];
    for (int i = 0; i < 4; ++i) {
        VBRs[i] = getVBR(partitions[i], fd);
        printf("VBR of partition %d:\n", i);
        if (VBRs[i] == NULL) {
            printf("Partition does not exist\n\n");
            continue;
        }
        for (int j = 0; j < SECTOR_SIZE; ++j) {
            printf("%02X ", VBRs[i][j]);
            if ((j + 1) % 16 == 0) {
                printf("\n");
            }
        }
        if (verifyNTFSVBR(VBRs[i])) {
            printf("Bytes 3-11 are \"NTFS    \" -- this partition is in NTFS "
                   "format\n");
            printf("Reached beginning of VBR for NTFS\n");
        }
        printf("\n");
    }

    // Deallocation
    for (int i = 0; i < 4; ++i) {
        if (partitions[i] != NULL) {
            free(partitions[i]);
        }
        if (VBRs[i] != NULL) {
            free(VBRs[i]);
        }
    }

    // Work done.
    return 0;
}

PartitionEntry *newPartitionEntry(unsigned char *buf) {
    // Check partition type
    if (buf[4] == 0x0) {
        // no partition defined
        return NULL;
    }

    // Create new entry
    PartitionEntry *entry = (PartitionEntry *)malloc(sizeof(PartitionEntry));
    entry->BootIndicator = buf[0]; // boolean cast, 0x00 is false, any other is
                                   // true, which covers 0x80
    entry->StartingSector = (uint32_t)buf[11] << 24 | buf[10] << 16 |
                            buf[9] << 8 | buf[8]; // converting to little endian
    // For example, flipping 0x00208517's endianness:
    //  0. Start with 0x00000000
    //  1. Add left shifted 17 by 6 (which is 24 in the code. 2^24 = 16^6). This
    //  turns 0x00000017 into 0x17000000, which when OR'd with previous results
    //  in 0x17000000
    //  2. Add left shifted 85 by 4 (which is 16 in the code. 2^16 = 16^4). This
    //  turns 0x00000085 into 0x00850000, which when OR'd with previous results
    //  in 0x17850000
    //  3. Add left shifted 20 by 2 (which is 8 in the code. 2^8 = 16^2). This
    //  turns 0x00000085 into 0x00850000, which when OR'd with previous results
    //  in 0x17852000
    //  4. OR previous with 0x00, which results in 0x17852000. Done.
    entry->PartitionType = buf[4]; // partition type

    // Entry set up, return
    return entry;
}

unsigned char *getVBR(PartitionEntry *entry, int fd) {
    if (entry == NULL) {
        return NULL;
    }

    unsigned char *vbr =
        (unsigned char *)malloc(sizeof(unsigned char) * SECTOR_SIZE);

    // Read VBR
    off_t seekerr = lseek(fd, entry->StartingSector * SECTOR_SIZE, SEEK_SET);
    if (seekerr < 0) {
        perror("lseek");
        return NULL;
    }
    ssize_t bytesRead = read(fd, vbr, SECTOR_SIZE);
    if (bytesRead < 0) {
        perror("read");
        return NULL;
    }

    // VBR read, return
    return vbr;
}

bool verifyNTFSVBR(unsigned char *vbr) {
    // Check bytes 3-11 for "NTFS    "
    char sig[9];
    for (int i = 0; i < 8; ++i) {
        sig[i] = vbr[i + 3];
    }
    sig[8] = '\0';

    // Verify NTFS signature
    return strcmp(sig, "NTFS    ") == 0;
}