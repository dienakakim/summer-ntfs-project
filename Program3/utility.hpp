#ifndef SUMMER_NTFS_PROJECT_PROGRAM3_UTILITY_HPP_
#define SUMMER_NTFS_PROJECT_PROGRAM3_UTILITY_HPP_

// Standard library
#include <cstdint> // for standard types
#include <vector>  // std::vector

// Self-defined
#include "Constants.hpp"

// Class declarations
class PartitionEntry;
class NTFSPartitionEntry;
class MBR;
class NTFSVBR;

// Type aliases
namespace dkt {
typedef std::vector<unsigned char> UString;      // vector of `unsigned char`s
typedef std::vector<PartitionEntry> EntryVector; // vector of `PartitionEntry`s
typedef std::vector<NTFSPartitionEntry>
    NTFSEntryVector;                    // vector of `NTFSPartitionEntry`s
typedef std::vector<NTFSVBR> VBRVector; // vector of `NTFSVBR`s
} // namespace dkt

// Class definitions

// General partition entry
class PartitionEntry {
  protected:
    // Data fields
    dkt::UString entry; // stored entry

  public:
    // Constructors
    PartitionEntry(const dkt::UString &); // string-based entry constructor

    // Methods
    dkt::UString GetEntry() const;
    void SetEntry(const dkt::UString &);
    bool GetBootIndicator() const;
    unsigned char GetPartitionType() const;
    std::uint64_t GetStartingSector() const;
};

// NTFS partition entry -- extends `PartitionEntry`
class NTFSPartitionEntry : public PartitionEntry {
  public:
    // Constructors
    NTFSPartitionEntry(const dkt::UString &); // string-based

    // Methods
    bool IsNTFSEntry() const; // NTFS check
};

// Disk sector
class Sector {
  protected:
    dkt::UString SectorStr;

  public:
    Sector(const dkt::UString &);
    dkt::UString GetSector() const;
    void SetSector(const dkt::UString &);
};

// MBR
class MBR : public Sector {
  public:
    // Constructors
    MBR(const dkt::UString &);

    // Methods
    bool IsValidMBR() const;               // check ending of MBR for 0x55AA
    dkt::EntryVector ParseEntries() const; // get `PartitionEntry`s
};

// NTFS VBR
class NTFSVBR : public MBR {
  public:
    // Constructors
    NTFSVBR(const dkt::UString &);

    // Methods
    dkt::UString GetVBR() const;
    void SetVBR(const dkt::UString &);
    std::uint64_t GetMFTLCN() const; // retrieve MFT sector from extended BPB
    std::uint64_t
    GetMFTMirrLCN() const; // retrieve MFTMirr sector from extended BPB
};

// Function prototypes (none)

#endif