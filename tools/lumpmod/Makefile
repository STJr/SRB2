# Makfile for SRB2 Lumpmod
# by Alam Arias et al.

SRC=lumpmod.c lump.c
OBJ=$(SRC:.c=.o)# replaces the .c from SRC with .o
EXE=lumpmod

.PHONY : all     # .PHONY ignores files named all
all: $(EXE)      # all is dependent on $(BIN) to be complete

$(EXE): $(OBJ) # $(EXE) is dependent on all of the files in $(OBJ) to exist
	$(CC) $(OBJ) $(LDFLAGS) -o $@

.PHONY : clean   # .PHONY ignores files named clean
clean:
	-$(RM) $(OBJ) $(EXE)
