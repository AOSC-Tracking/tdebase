KDE_CPPFLAGS = -DTQT_NO_CAST_ASCII

INCLUDES = $(all_includes)

kde_module_LTLIBRARIES = tdeio_filter.la

tdeio_filter_la_SOURCES = filter.cpp
tdeio_filter_la_LIBADD = $(LIB_TDESYCOCA)
tdeio_filter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = filter.h

METASOURCES = AUTO

protocoldir = $(kde_servicesdir)

if include_BZIP2
BZIP2FILES=bzip.protocol bzip2.protocol
endif
if include_XZ
XZPROTOCOL=xz.protocol
endif
if include_LZMA
LZMAPROTOCOL=lzma.protocol
endif
if include_LZIP
LZIPPROTOCOL=lzip.protocol
endif

protocol_DATA = gzip.protocol $(BZIP2FILES) $(XZPROTOCOL) $(LZMAPROTOCOL) $(LZIPPROTOCOL)

