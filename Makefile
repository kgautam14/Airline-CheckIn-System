.phony all:
all: ACS

ACS: ACS.c
	gcc ACS.c -o ACS -lpthread
.PHONY clean:
clean:
	-rm -rf *.o *.exe

