#ifndef SUMMER_NTFS_PROJECT_PROGRAM3_CONSTANTS_HPP_
#define SUMMER_NTFS_PROJECT_PROGRAM3_CONSTANTS_HPP_

// Error codes
const int SUCCESS = 0;
const int ARGUMENT_EXPECTED = 1;
const int OPEN_ERROR = 2;
const int READ_ERROR = 3;
const int LSEEK_ERROR = 4;
const int GPT_FORMATTED = 5;

// Magic numbers
const int SECTOR_SIZE = 512;            // sector size
const int PARTITION_TABLE_OFFSET = 446; // partition table offset
const int PARTITION_TABLE_SIZE = 64;    // partition table size
const int PARTITION_ENTRY_SIZE = 16;    // partition entry size
const int NTFS_VBR_MFT_OFFSET = 0x30; // MFT starting sector, starting from VBR
const int NTFS_VBR_MFTMIRR_OFFSET =
    0x38;                   // MFTMirr starting sector, starting from VBR
const int CLUSTER_SIZE = 8; // cluster size, in sectors

#endif