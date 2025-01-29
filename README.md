# FAT32-File-System
## Summary of FAT32-File-System
This program acts as a mini FAT32 shell, allowing users to interact with a disk image through file navigation, metadata retrieval, and file extraction. It demonstrates key file system concepts, such as boot sector parsing, FAT table traversal, and directory entry reading. While primarily read-only, it provides a foundation for further FAT32 file system manipulation, including writing and directory management. 

## Key Features of FAT32-File-System
### FAT32 Boot Sector Parsing

Reads important FAT32 metadata such as:

-Bytes per sector

-Sectors per cluster

-Number of FAT tables

-Size of each FAT table

-Root directory cluster

-Filesystem flags

-These values are used to compute file system offsets.

### Command Parsing and Execution

-Reads user input and tokenizes it using whitespace delimiters.

-Supports multiple built-in commands to interact with the FAT32 file system.

### FAT32 File System Navigation

-Uses cluster-based addressing to traverse directories.

-Converts file names to FAT32 8.3 format.

-Reads directory entries and extracts file metadata.

### File Retrieval (get command)

-Finds the requested file in the FAT32 directory.

-Reads cluster chains using the File Allocation Table (FAT).

-Copies the file’s content from the disk image to a new local file.

### Error Handling

-Displays errors for invalid commands, missing files, and incorrect usage.

-Ensures only one FAT32 image is open at a time.

-Prevents access to invalid file system images.

## Built-in Commands

### open <filename>
-Opens a FAT32 disk image.

-Reads and stores essential FAT32 boot sector data.

### close
-Closes the currently opened disk image.

### ls
-Lists files and directories in the root directory.

### stat <filename>

-Displays attributes of a specified file, including size and starting cluster.

### get <filename>
-Extracts a file from the FAT32 disk image and saves it to the local system.

### put <filename> 
-Uploads a file to the FAT32 file system.

-Displays key FAT32 metadata from the boot sector.
### exit / quit
-Terminates the shell.
