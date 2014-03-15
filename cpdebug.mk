#Add-on Makefile for wxDev-C++ project file
ifdef ComSpec
COMSPEC=$(ComSpec)
endif
ifdef COMSPEC
OBJCOPY=objcopy.exe
OBJDUMP=objdump.exe
GZIP?=gzip.exe
else
OBJCOPY=objcopy
OBJDUMP=objdump
GZIP?=gzip
endif
DBGNAME=$(BIN).debug
OBJDUMP_OPTS?=--wide --source --line-numbers
GZIP_OPTS?=-9 -f -n
GZIP_OPT2=$(GZIP_OPTS) --rsyncable
UPX?=upx
UPX_OPTS?=--best --preserve-build-id
UPX_OPTS+=-q

all-after:
	$(OBJDUMP) $(OBJDUMP_OPTS) "$(BIN)" > "$(DBGNAME).txt"
	$(OBJCOPY) $(BIN) $(DBGNAME)
	$(OBJCOPY) --strip-debug $(BIN)
	-$(OBJCOPY) --add-gnu-debuglink=$(DBGNAME) $(BIN)
	-$(GZIP) $(GZIP_OPTS) $(DBGNAME).txt
ifndef COMSPEC
	$(GZIP) $(GZIP_OPT2) $(DBGNAME).txt
endif
	-$(UPX) $(UPX_OPTS) $(BIN)

