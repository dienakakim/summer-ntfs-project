# UTD Summer 2020 Software Project

## ... on NTFS file recovery

This is my submission for the given programming assignments for an NTFS file recovery project in Summer 2020 at UT Dallas. Mostly written in C++, occasionally C, hopefully these demonstrate my, for lack of a better term, humble understanding of the innerworkings of NTFS.

These programs are expected to be compiled and run on a **Linux** system.

### Program1

Assuming the given disk is in MBR (`msdos`) partition format, the program will list all VBRs on the given disk, noting which partitions are formatted in NTFS.

### Program2

This follows up on Program1 by parsing the actual NTFS VBRs and finding the address of `$MFT` and `$MFTMirr`.
