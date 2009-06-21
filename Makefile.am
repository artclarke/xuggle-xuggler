include $(abs_top_builddir)/mk/Makefile.global
if CAPTIVES_ENABLED
  CAPTIVES=captive
endif
SUBDIRS=$(CAPTIVES) csrc test/csrc
EXTRA_DIST= \
  src \
  test/src \
  share \
  generated \
  ivy.xml \
  build.xml \
  mk/buildtools/buildhelper.xml \
  mk/buildtools/Doxyfile \
  mk/buildtools/Valgrind.suppressions \
  mk/buildtools/build.properties \
  mk/buildtools/buildhelper.xml \
  mk/buildtools/guess_os.sh \
  mk/buildtools/ivysettings.xml \
  mk/buildtools/memcheck \
  mk/buildtools/revision.sh \
  mk/buildtools/install-build.sh \
  mk/buildtools/run \
  mk/buildtools/runtests \
  mk/buildtools/lib \
  lib \
  test/fixtures

DOC_DIR=$(abs_top_srcdir)/doc/cpp/api

if HAVE_DOXYGEN
doc: all
	$(MKDIR_P) $(DOC_DIR)
	cd $(abs_top_srcdir) && $(DOXYGEN) $(abs_top_srcdir)/mk/buildtools/Doxyfile
else
.PHONY : doc
doc:
	@echo "You must have 'doxygen' installed on your system to generate"
	@echo "the documentation.  Try installing it, and running"
	@echo "'configure' again."
endif

dist-hook:
	rm -rf `find $(distdir) -name '.svn'`
	rm -rf `find $(distdir) -name '.class'`
	$(MKDIR_P) $(distdir)/doc
	$(abs_top_srcdir)/mk/buildtools/revision.sh $(abs_top_srcdir) $(abs_top_srcdir)/LastRevisionBuilt.txt $(distdir)/LastRevisionBuilt.txt
