#
# Flags to put a sock in GCC!
#

# See the versions list in detect.mk
# This will define all version flags going backward.
# Yes, it's magic.
define _predecessor =
ifdef GCC$(firstword $(1))
GCC$(lastword $(1)):=1
endif
endef
_n:=$(words $(gcc_versions))
$(foreach v,$(join $(wordlist 2,$(_n),- $(gcc_versions)),\
	$(addprefix =,$(wordlist 2,$(_n),$(gcc_versions)))),\
	$(and $(findstring =,$(v)),\
	$(eval $(call _predecessor,$(subst =, ,$(v))))))

# -W -Wno-unused
WFLAGS:=-Wall -Wno-trigraphs
ifndef GCC295
#WFLAGS+=-Wno-packed
endif
ifndef RELAXWARNINGS
 WFLAGS+=-W
#WFLAGS+=-Wno-sign-compare
ifndef GCC295
 WFLAGS+=-Wno-div-by-zero
endif
#WFLAGS+=-Wsystem-headers
WFLAGS+=-Wfloat-equal
#WFLAGS+=-Wtraditional
 WFLAGS+=-Wundef
ifndef GCC295
 WFLAGS+=-Wendif-labels
endif
ifdef GCC41
 WFLAGS+=-Wshadow
endif
#WFLAGS+=-Wlarger-than-%len%
 WFLAGS+=-Wpointer-arith -Wbad-function-cast
ifdef GCC45
#WFLAGS+=-Wc++-compat
endif
 WFLAGS+=-Wcast-qual
ifndef NOCASTALIGNWARN
 WFLAGS+=-Wcast-align
endif
 WFLAGS+=-Wwrite-strings
ifndef ERRORMODE
#WFLAGS+=-Wconversion
ifdef GCC43
 #WFLAGS+=-Wno-sign-conversion
endif
endif
 WFLAGS+=-Wsign-compare
ifdef GCC91
 WFLAGS+=-Wno-error=address-of-packed-member
endif
ifdef GCC45
 WFLAGS+=-Wlogical-op
endif
 WFLAGS+=-Waggregate-return
ifdef HAIKU
ifdef GCC41
 #WFLAGS+=-Wno-attributes
endif
endif
#WFLAGS+=-Wstrict-prototypes
ifdef GCC40
 WFLAGS+=-Wold-style-definition
endif
 WFLAGS+=-Wmissing-prototypes -Wmissing-declarations
ifdef GCC40
 WFLAGS+=-Wmissing-field-initializers
endif
 WFLAGS+=-Wmissing-noreturn
#WFLAGS+=-Wmissing-format-attribute
#WFLAGS+=-Wno-multichar
#WFLAGS+=-Wno-deprecated-declarations
#WFLAGS+=-Wpacked
#WFLAGS+=-Wpadded
#WFLAGS+=-Wredundant-decls
 WFLAGS+=-Wnested-externs
#WFLAGS+=-Wunreachable-code
 WFLAGS+=-Winline
ifdef GCC43
 WFLAGS+=-funit-at-a-time
 WFLAGS+=-Wlogical-op
endif
ifndef GCC295
 WFLAGS+=-Wdisabled-optimization
endif
endif
WFLAGS+=-Wformat-y2k
ifdef GCC71
WFLAGS+=-Wno-error=format-overflow=2
endif
WFLAGS+=-Wformat-security
ifndef GCC29
#WFLAGS+=-Winit-self
endif
ifdef GCC46
WFLAGS+=-Wno-suggest-attribute=noreturn
endif

ifdef NOLDWARNING
LDFLAGS+=-Wl,--as-needed
endif

ifdef ERRORMODE
WFLAGS+=-Werror
endif

ifdef GCC43
 #WFLAGS+=-Wno-error=clobbered
endif
ifdef GCC44
 WFLAGS+=-Wno-error=array-bounds
endif
ifdef GCC46
 WFLAGS+=-Wno-error=suggest-attribute=noreturn
endif
ifdef GCC54
 WFLAGS+=-Wno-logical-op -Wno-error=logical-op
endif
ifdef GCC61
 WFLAGS+=-Wno-tautological-compare -Wno-error=tautological-compare
endif
ifdef GCC71
 WFLAGS+=-Wimplicit-fallthrough=4
endif
ifdef GCC81
 WFLAGS+=-Wno-error=format-overflow
 WFLAGS+=-Wno-error=stringop-truncation
 WFLAGS+=-Wno-error=stringop-overflow
 WFLAGS+=-Wno-format-overflow
 WFLAGS+=-Wno-stringop-truncation
 WFLAGS+=-Wno-stringop-overflow
 WFLAGS+=-Wno-error=multistatement-macros
endif

ifdef NONX86
  ifdef X86_64 # yeah that SEEMS contradictory
  opts+=-march=nocona
  endif
else
  ifndef GCC29
  opts+=-msse3 -mfpmath=sse
  else
  opts+=-mpentium
  endif
endif

ifdef DEBUGMODE
ifdef GCC48
opts+=-Og
else
opts+=-O0
endif
endif

ifdef VALGRIND
ifdef GCC46
WFLAGS+=-Wno-error=unused-but-set-variable
WFLAGS+=-Wno-unused-but-set-variable
endif
endif

# Lua
ifdef GCC43
ifndef GCC44
WFLAGS+=-Wno-logical-op
endif
endif
