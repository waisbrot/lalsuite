BUILT_SOURCES =
sysconf_DATA =
MOSTLYCLEANFILES =
EXTRA_DIST =
include $(top_srcdir)/gnuscripts/lalsuite_user_env.am

ACLOCAL_AMFLAGS = -I gnuscripts

if HAVE_GIT_REPO
# Override automake's default rule for rebuilding aclocal.m4, to ensure that
# ./configure is successfully recreated after certain build system changes which
# require other autotools (e.g. autoheader) to be re-run. See Redmine issue #728.
$(ACLOCAL_M4): $(am__aclocal_m4_deps)
	$(am__cd) $(srcdir) && $(SHELL) ./00boot
endif # HAVE_GIT_REPO

EXTRA_DIST += \
	README \
	README.bugs \
	README.contrib \
	lalapps.spec \
	$(DATA)

SUBDIRS = \
	debian \
	src \
	doc \
	man

user_environment = \
	set $(PACKAGE)_PREFIX $(prefix); \
	set $(PACKAGE)_DATADIR $(pkgdatadir); \
	prepend MANPATH $(mandir); \
	prepend PATH $(bindir); \
	prepend PYTHONPATH $(pyexecdir) $(pythondir); \
	$(END_OF_LIST)

all-local:
	@echo "";\
	echo "==================================================";\
	echo "LALApps has now been successfully built.";\
	echo "Now run 'make install' to install LALApps.";\
	echo "==================================================";\
	echo ""

install-exec-local:
	@echo "";\
	echo "==================================================";\
	echo "LALApps has now been successfully installed.";\
	echo "Now you may want to setup your environment:";\
	echo "";\
	echo "* Bourne shell [bash] users: please add the line";\
	echo "  . $(sysconfdir)/lalapps-user-env.sh";\
	echo "  to your .profile file.";\
	echo "";\
	echo "* C-shell [tcsh] users: please add the line";\
	echo "  source $(sysconfdir)/lalapps-user-env.csh";\
	echo "  to your .login file.";\
	echo "==================================================";\
	echo ""
