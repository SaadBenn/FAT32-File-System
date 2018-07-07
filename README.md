# FAT-32-File-System
Write a program that reads a FAT 32-formatted drive. For this assignment we will use disk image - though you are encouraged to read a USB flash drive as a raw device (ie. /dev/disk3).

Your program requires 2 functions:

Print information about the drive. The command, assuming your program is named fat32, would be ./fat32 imagename info. Print out the following:

Drive name
Free space on the drive in kB
The amount of usable storage on the drive (not free, but usable space)
The cluster size in number of sectors, and in KB.
