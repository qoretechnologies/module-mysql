AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/qore-mysql.h \
	src/qore-mysql-module.h

EXTRA_DIST = COPYING.LGPL COPYING.GPL ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/db-test.q \
	test/mysql-native-bulk-load.qtest \
	test/sql-stmt.q \
	qore-mysql-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

if COND_DOXYGEN
DOX = mysql
DOXYGEN_OUTPUT = docs/mysql/html

DOXYFILES = docs/doxyfile docs/mainpage.dox
DOXYFILES += docs/guide-mysqlreleasenotes.dox docs/guide-mysqlstoredprocs.dox docs/guide-mysqlbind.dox docs/guide-mysqlerrorinfo.dox docs/guide-mysqltrans.dox docs/guide-mysqloptions.dox docs/guide-mysqlnativebulkload.dox
DOXYFILES_SRC = docs/doxyfile.tmpl docs/footer_template.html
QDX = qdx
DX_CLEANFILES = ${DOXYFILES}

docs/mysql/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(QDX) --post $(DOXYGEN_OUTPUT)/*html
	$(QDX) --post $(DOXYGEN_OUTPUT)/search/*html

docs/mainpage.dox: docs/mainpage.dox.tmpl
	$(QDX) -d $< $@

docs/doxyfile: docs/doxyfile.tmpl
	sed -e s/__VERSION__/${VERSION}/ -e s:__USERMODDIR__:${usermoddir}: $< > $@

docs/guide-mysqlreleasenotes.dox: docs/guide-mysqlreleasenotes.dox.tmpl
	$(QDX) -d $< $@

docs/guide-mysqlstoredprocs.dox: docs/guide-mysqlstoredprocs.dox.tmpl
	$(QDX) -d $< $@

docs/guide-mysqlbind.dox: docs/guide-mysqlbind.dox.tmpl
	$(QDX) -d $< $@

docs/guide-mysqlerrorinfo.dox: docs/guide-mysqlerrorinfo.dox.tmpl
	$(QDX) -d $< $@

docs/guide-mysqltrans.dox: docs/guide-mysqltrans.dox.tmpl
	$(QDX) -d $< $@

docs/guide-mysqloptions.dox: docs/guide-mysqloptions.dox.tmpl
	$(QDX) -d $< $@

docs/guide-mysqlnativebulkload.dox: docs/guide-mysqlnativebulkload.dox.tmpl
	$(QDX) -d $< $@


doxygen-doc: $(DOXYGEN_OUTPUT)

EXTRA_DIST += $(DOXYGEN_OUTPUT)

html-local: $(DOXYGEN_OUTPUT)

install-html-local:
	for dox in $(DOX); do $(MKDIR_P) $(DESTDIR)$(datadir)/$(PACKAGE)/docs/$$dox/html/search; done
	for dir in $(DOXYGEN_OUTPUT); do \
	   for f in $$dir/*; do if [ -f $$f ]; then $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir; fi; done; \
	   for f in $$dir/search/*; do $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir/search; done; \
	done

clean-local:
	$(RM) -rf $(DOXYGEN_OUTPUT)

uninstall-local:
	$(RM) -rf $(DESTDIR)$(datadir)/$(PACKAGE)/docs

mostlyclean-local:
	rm -rf ${DOXYGEN_OUTPUT} ${DX_CLEANFILES}

else
doxygen-doc:
	echo no doxygen support
endif

# Documentation guide sources shipped with the module.
EXTRA_DIST += docs/guide-mysqlreleasenotes.dox.tmpl \
	docs/guide-mysqlstoredprocs.dox.tmpl \
	docs/guide-mysqlbind.dox.tmpl \
	docs/guide-mysqlerrorinfo.dox.tmpl \
	docs/guide-mysqltrans.dox.tmpl \
	docs/guide-mysqloptions.dox.tmpl \
	docs/guide-mysqlnativebulkload.dox.tmpl
