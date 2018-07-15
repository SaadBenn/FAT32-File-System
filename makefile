PROGRAM_NAME = fat32

OBJECT_FILES = $(PROGRAM_NAME).o
CFLAGS = -g

$(PROGRAM_NAME) : $(OBJECT_FILES)
	gcc $(CFLAGS) -o $@ $(OBJECT_FILES)

$(PROGRAM_NAME).o : $(PROGRAM_NAME).c fat32.h fat_fs.h
	gcc $(CFLAGS) -c $<

clean :
	$(RM) $(PROGRAM_NAME)
	$(RM) $(OBJECT_FILES)
	$(RM) *~ *.bak