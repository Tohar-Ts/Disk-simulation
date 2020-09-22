# General

This program is a simulation of a file managment system in a disk according to unix system.
With this is program it is possible to do all the usual actions of file managment with direct entries and single directory.
It's possible to create, open, close and delete files, to write and read from and to files, to format the disk, and to view the disk's contents.

The program works with a UI as will be explained later.

# Classes

1. fsInode - simulates unix Inode. Keeps details about direct entries, single indirects.
2. fileDescriptor - connects a file to its Inode.
3. fsDisk - the main program. Simulates the disk and its actions.

# Important Functions in fsDisk

1. listAll - pritns the disk contents.
2. fsFormat - allows the user to format the disk, recieves block size and number of direct entries.
3. CreateFile - create a new file.
4. openFile - open a file that has been created.
5. closeFile - closes a file.
6. WriteToFile - write to direct entries on the disk, when the direct entries are full - write to a single indirect using managment block.
7. ReadFromFile - read the minimal length - file length or user's length.

# Helper Functions

1. DecToBinary - convert from int to binary in a single char.

# User Interface

There are 8 options:

0 - Format the disk and exit.
1 - View disk contents.
2 - Format the disk-> Follows input: block size, direct entries
3 - Create a new file-> Follows input: File's name.
4 - Open file-> Follows input: File's name.
5 - Close file -> Follows input: File's fd.
6 - Write to file -> Follows input: File's fd, string.
7 - Read from file -> Follows input: File's fd, length to read
8 - Delete File -> Follows input: File's name.


# Compile the program:

Press ctrl+shift+b and than press enter twice.
or in terminal- gcc sim_disk.cpp main.cpp -o main

# Run the program:

press ctrl+f5.	
in terminal- after copmpiling the program write ./main. and press Enter.
