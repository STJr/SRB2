#
# Warn about old build directories and offer to purge.
#

_old:=$(wildcard $(addprefix ../bin/,FreeBSD Linux \
		Linux64 Mingw Mingw64 SDL dummy) ../objs ../dep)

ifdef _old
$(foreach v,$(_old),$(info $(abspath $(v))))
$(info )
$(info These directories are no longer\
       required and should be removed.)
$(info You may remove them manually or\
       by using 'make distclean')
$(error )
endif
