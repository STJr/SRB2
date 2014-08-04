EXE=psp-prxsign
SRC=main.c cmac.c
OBJ=$(SRC:.c=.o)# replaces the .c from SRC with .o

OPENSSL_PKGCONFIG?=openssl
OPENSSL_CFLAGS?=$(shell pkg-config $(OPENSSL_PKGCONFIG) --cflags)
OPENSSL_LDFLAGS?=$(shell pkg-config $(OPENSSL_PKGCONFIG) --libs)

CFLAGS+=$(OPENSSL_CFLAGS)
LDFLAGS+=$(OPENSSL_LDFLAGS)

.PHONY : all     # .PHONY ignores files named all

all: $(EXE)      # all is dependent on $(BIN) to be complete


$(EXE): $(OBJ) # $(EXE) is dependent on all of the files in $(OBJ) to exist
	$(CC) $^ $(LDFLAGS) -o $@

.PHONY : clean   # .PHONY ignores files named clean
clean:
	-$(RM) $(OBJ) $(EXE)
