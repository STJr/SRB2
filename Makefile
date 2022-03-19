ifdef SILENT
MAKEFLAGS+=--no-print-directory
endif

all :

% ::
	@$(MAKE) -C src $(MAKECMDGOALS)
