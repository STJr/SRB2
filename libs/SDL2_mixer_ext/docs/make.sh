#!/bin/bash
texi2html --header --glossary --menu SDL_mixer_ext.texinfo

# Translates Texinfo source documentation to HTML.
# Option   Type  Default  Description
# --build-date=i
#    use the given unix time as build date showing UTC timezone when it is used
# --conf-dir=s            append $s to the init file directories
# --css-include=s         use css file $s
# --css-ref=s             generate reference to the CSS URL $s
# --debug=i  0            output HTML with debuging information
# --document-language=s    use $s as document language
# --error-limit=i  1000   quit after NUM errors (default 1000).
# --footnote-style=s      output footnotes separate|end
# --header!   1           output navigation headers for each section
# --help:i                print help and exit
# --I=s                   append $s to the @include search path
# --ifdocbook!            expand ifdocbook and docbook sections
# --ifhtml!               expand ifhtml and html sections
# --ifinfo!               expand ifinfo
# --ifplaintext!          expand ifplaintext sections
# --iftex!                expand iftex and tex sections
# --ifxml!                expand ifxml and xml sections
# --init-file=s           load init file $s
# --l2h!                  if set, uses latex2html for @math and @tex
# --macro-expand=s        output macro expanded source in <file>
# --menu!   1             output Texinfo menus
# --no-expand=s           Don't expand the given section of texinfo source
# --no-validate!   0      suppress node cross-reference validation
# --node-files!   0       produce one file per node for cross references
# --number-sections!   1  use numbered sections
# --output=s              output goes to $s (directory if split)
# --P=s                   prepend $s to the @include search path
# --prefix=s              use as prefix for output files, instead of <docname>
# --short-ext!   0        use "htm" extension for output HTML files
# --short-ref!            if set, references are without section numbers
# --split=s
#    split document on section|chapter|node else no splitting
# --toc-file=s            use $s as ToC file, instead of <docname>_toc.html
# --toc-links!   0        create links from headings to toc entries
# --top-file=s            use $s as top file, instead of <docname>.html
# --transliterate-file-names!   1  produce file names in ASCII transliteration
# --use-nodes!            use nodes for sectionning
# --verbose!              print progress info to stdout
# --version               print version and exit
#Note: 'Options' may be abbreviated. -- prefix may be replaced by a single -.
#'Type' specifications mean:
# <none>| !    no argument: variable is set to 1 on -foo (or, to 0 on -nofoo)
#    =s | :s   mandatory (or, optional)  string argument
#    =i | :i   mandatory (or, optional)  integer argument
#
#Send bugs and suggestions to <texi2html-bug@nongnu.org>
#

