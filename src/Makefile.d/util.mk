#
# Utility macros for the rest of the Makefiles.
#

Ifnot=$(if $(1),$(3),$(2))
Ifndef=$(call Ifnot,$($(1)),$(2),$(3))

# Match and expand a list of variables by pattern.
Wildvar=$(foreach v,$(filter $(1),$(.VARIABLES)),$($(v)))

# Read a list of words from file and prepend each with the
# directory of the file.
_cat=$(shell $(cat) $(call Windows_path,$(1)))
List=$(addprefix $(dir $(1)),$(call _cat,$(1)))

# Convert path separators to backslash on Windows.
Windows_path=$(if $(WINDOWSHELL),$(subst /,\,$(1)),$(1))

define Propogate_flags =
opts+=$$($(1)_CFLAGS)
libs+=$$($(1)_LDFLAGS)
endef

# Set library's _CFLAGS and _LDFLAGS from some command.
# Automatically propogates the flags too.
# 1: variable prefix (e.g. CURL)
# 2: start of command (e.g. curl-config)
# --- optional ----
# 3: CFLAGS command arguments, default '--cflags'
# 4: LDFLAGS command arguments, default '--libs'
# 5: common command arguments at the end of command
define Configure =
$(1)_CFLAGS?=$$(shell $(2) $(or $(3),--cflags) $(5))
$(1)_LDFLAGS?=$$(shell $(2) $(or $(4),--libs) $(5))
$(call Propogate_flags,$(1))
endef

# Configure library with pkg-config. The package name is
# taken from a _PKGCONFIG variable.
# 1: variable prefix
#
#     LIBGME_PKGCONFIG=libgme
#     $(eval $(call Use_pkg_config,LIBGME))
define Use_pkg_config =
$(call Configure,$(1),$(PKG_CONFIG),,,$($(1)_PKGCONFIG))
endef

# Check disabling flag and configure package in one step
# according to delimited argument.
# (There is only one argument, but it split by slash.)
# 1/: short form library name (uppercase). This is
#     prefixed with 'NO' and 'HAVE_'. E.g. NOGME, HAVE_GME
# /2: package name (e.g. libgme)
# /3: variable prefix
#
# The following example would check if NOGME is not
# defined before attempting to define LIBGME_CFLAGS and
# LIBGME_LDFLAGS as with Use_pkg_config.
#
#     $(eval $(call Check_pkg_config,GME/libgme/LIBGME))
define Check_pkg_config =
_p:=$(subst /, ,$(1))
_v1:=$$(word 1,$$(_p))
_v2:=$$(or $$(word 3,$$(_p)),$$(_v1))
ifndef NO$$(_v1)
$$(_v2)_PKGCONFIG?=$$(word 2,$$(_p))
$$(eval $$(call Use_pkg_config,$$(_v2)))
opts+=-DHAVE_$$(_v1)
endif
endef

#     $(call Prefix,gcc)
Prefix=$(if $(PREFIX),$(PREFIX)-)$(1)

Echo=
Echo_name=
Print=

ifndef SILENT
Echo=@echo $(1)
ifndef ECHO
ifndef NOECHOFILENAMES
Echo_name=$(call Echo,-- $(1) ...)
endif
endif
ifndef MAKE_RESTARTS
ifndef destructive
Print=$(info $(1))
endif
endif
endif

.=$(call Ifndef,ECHO,@)
