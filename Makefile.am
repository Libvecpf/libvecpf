ACLOCAL_AMFLAGS = -I m4

SUBDIRS = man

# Check if system headers were defined during configure
if SYSHEADERS_SET
include_sysheaders=-I$(sysheaders)
else
include_sysheaders=
endif

noinst_HEADERS = vecpf.h
lib_LTLIBRARIES = libvecpf.la
libvecpf_la_SOURCES = vecpf.c
libvecpf_la_CFLAGS = $(include_sysheaders)

# Define which check programs should be built
check_PROGRAMS =
if TEST_SHARED
check_PROGRAMS += test_vecpf_shared
endif
if TEST_STATIC
check_PROGRAMS += test_vecpf_static
endif
TESTS = $(check_PROGRAMS)

if TEST_SHARED
test_vecpf_shared_SOURCES = test_vecpf.c
test_vecpf_shared_DEPENDENCIES = libvecpf.la
test_vecpf_shared_LDFLAGS = -Wl,--no-as-needed -lvecpf
endif

if TEST_STATIC
test_vecpf_static_SOURCES = test_vecpf.c
test_vecpf_static_DEPENDENCIES = libvecpf.la
test_vecpf_static_LDFLAGS = -all-static -Wl,-u,__libvecpf_init
test_vecpf_static_LDADD = libvecpf.la
endif

dist_doc_DATA = README LICENSE ChangeLog.md

.PHONY: deb
deb:
	dpkg-buildpackage

ChangeLog.md:
	$(am__cd) $(top_srcdir) && ./generate-changelog.sh \
	  > $(abs_top_builddir)/$@
