
CXXFLAGS += --std=c++0x -O2

packages = poppler-cairo
CXXFLAGS += `pkg-config --cflags $(packages)`
LDLIBS += `pkg-config --libs $(packages)`

# My version of poppler installed not everything and the .pc is broken...
CXXFLAGS += -I/usr/include/poppler
LDLIBS += /usr/local/lib/libpoppler-cairo.a


pdfposter : pdfposter.cc cmdline.c

cmdline.c cmdline.h : options.ggo
	gengetopt -i $< -F $*
