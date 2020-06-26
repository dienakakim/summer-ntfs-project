#include "utility.hpp"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// `UString`-based `PartitionEntry` constructor.
// THROWS:
//  - std::invalid_argument("Expected ${PARTITION_ENTRY_SIZE} for entry
//  size, got ${actual_value}"): if partition entry size is not exactly
//  PARTITION_ENTRY_SIZE
//  - std::invalid_argument("Non-existent partition"): if partition
//  entry's type field is 0x0
PartitionEntry::PartitionEntry(const dkt::UString &entry) {
    // Copy entry
    this->SetEntry(entry);
}

// Entry string getter
dkt::UString PartitionEntry::GetEntry() const { return this->entry; }

// Entry string setter
void PartitionEntry::SetEntry(const dkt::UString &entry) {
    // Check if entry size is not exactly PARTITION_ENTRY_SIZE bytes long
    if (entry.size() != PARTITION_ENTRY_SIZE) {
        std::stringstream ss;
        ss << "Expected " << PARTITION_ENTRY_SIZE << " for entry size, got "
           << entry.size();
        throw std::invalid_argument(ss.str());
    }

    // Check if partition type is non-zero (0x0 is agreed upon as a non-existent
    // partition type)
    if (entry[4] == 0x0) {
        throw std::invalid_argument("Non-existent partition");
    }

    this->entry = entry;
}

// Bootable status
bool PartitionEntry::GetBootIndicator() const {
    return static_cast<bool>(
        entry[0]); // return if boot field is non-zero (0x80)
}

// Partition type
unsigned char PartitionEntry::GetPartitionType() const { return entry[4]; }

// Starting sector (VBR)
std::uint64_t PartitionEntry::GetStartingSector() const {
    return static_cast<std::uint64_t>(entry[11]) << 24 | entry[10] << 16 |
           entry[9] << 8 | entry[8];
}

// `NTFSPartitionEntry` constructor
NTFSPartitionEntry::NTFSPartitionEntry(const dkt::UString &entry)
    : PartitionEntry(entry) {
    if (!this->IsNTFSEntry()) {
        throw std::invalid_argument("Not an NTFS entry (byte 4 != 0x07)");
    }
}

// NTFS partition check
bool NTFSPartitionEntry::IsNTFSEntry() const { return entry[4] == 0x07; }

// `UString`-based `Sector` constructor.
// THROWS:
//  - std::invalid_argument("Expected ${SECTOR_SIZE} for sector size, got
//  ${actual_value}"): if sector size isn't exactly SECTOR_SIZE
Sector::Sector(const dkt::UString &sector) { this->SetSector(sector); }

dkt::UString Sector::GetSector() const { return this->SectorStr; }

void Sector::SetSector(const dkt::UString &sector) {
    // Check if sector size is SECTOR_SIZE
    if (sector.size() != SECTOR_SIZE) {
        std::stringstream ss;
        ss << "Expected " << SECTOR_SIZE << " for sector size, got "
           << sector.size();
        throw std::invalid_argument(ss.str());
    }

    // Set sector string to given string
    this->SectorStr = sector;
}

// `UString`-based `MBR` constructor.
// THROWS:
//  - std::invalid_argument("MBR ending must be 0x55AA"): if MBR string's ending
//  isn't 0x55AA (big endian)
MBR::MBR(const dkt::UString &MBR) : Sector(MBR) {
    if (!this->IsValidMBR()) {
        throw std::invalid_argument("MBR ending must be 0x55AA");
    }
}

// 0x55AA check
bool MBR::IsValidMBR() const {
    return SectorStr[SectorStr.size() - 2] == 0x55 &&
           SectorStr[SectorStr.size() - 1] == 0xAA;
}

// Partition entry parser. Returns an `EntryVector`
dkt::EntryVector MBR::ParseEntries() const {
    // Set up return vector
    dkt::EntryVector result;
    result.reserve(4); // size of at most 4 entries

    // Entry push back loop
    for (int i = 0; i < 4; ++i) {
        try {
            // Attempt to create partition. Might throw std::invalid_argument
            // because of non-existent partition
            PartitionEntry partition(dkt::UString(
                this->SectorStr.begin() + PARTITION_TABLE_OFFSET +
                    i * PARTITION_ENTRY_SIZE,
                this->SectorStr.begin() + PARTITION_TABLE_OFFSET +
                    (i + 1) * PARTITION_ENTRY_SIZE)); // might throw

            // If we get here, `PartitionEntry` created successfully, so
            // partition does exist
            // Add to result vector
            result.push_back(partition);
        } catch (std::invalid_argument &e) {
            // Do nothing
        }
    }

    // Parsing done
    return result;
}

// `UString`-based `NTFSVBR` constructor.
// THROWS:
//  - std::invalid_argument("VBR ending must be 0x55AA"): if VBR ending is not
//  0x55AA (big endian)
NTFSVBR::NTFSVBR(const dkt::UString &VBR) : MBR(VBR) { this->SetVBR(VBR); }

// VBR getter
dkt::UString NTFSVBR::GetVBR() const { return this->SectorStr; }

// VBR setter
void NTFSVBR::SetVBR(const dkt::UString &VBR) {
    // Check VBR for NTFS telltales
    // 0x55AA is already checked in MBR constructor,
    // so check if bytes 3-10 are "NTFS    "
    if (VBR[3] != 'N'
     || VBR[4] != 'T'
     || VBR[5] != 'F'
     || VBR[6] != 'S'
     || VBR[7] != ' '
     || VBR[8] != ' '
     || VBR[9] != ' '
     || VBR[10] != ' ') {
         throw std::invalid_argument("Bytes 3-10 are not equal to \"NTFS    \"");
     }

    // Set VBR
    this->SectorStr = VBR;
}

// Compute MFT LCN
std::uint64_t NTFSVBR::GetMFTLCN() const {
    return static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 7])
               << (8 * 7) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 6])
               << (8 * 6) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 5])
               << (8 * 5) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 4])
               << (8 * 4) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 3])
               << (8 * 3) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 2])
               << (8 * 2) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET + 1])
               << (8 * 1) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFT_OFFSET]);
}

// Compute MFTMirr LCN
std::uint64_t NTFSVBR::GetMFTMirrLCN() const {
    return static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 7])
               << (8 * 7) |
           static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 6])
               << (8 * 6) |
           static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 5])
               << (8 * 5) |
           static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 4])
               << (8 * 4) |
           static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 3])
               << (8 * 3) |
           static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 2])
               << (8 * 2) |
           static_cast<std::uint64_t>(
               this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET + 1])
               << (8 * 1) |
           static_cast<std::uint64_t>(this->SectorStr[NTFS_VBR_MFTMIRR_OFFSET]);
}