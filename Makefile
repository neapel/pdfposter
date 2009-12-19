
packages = poppler-cairo

CXXFLAGS += `pkg-config --cflags $(packages)` -g3 -I/usr/include/poppler --std=c++0x
LDLIBS += `pkg-config --libs $(packages)` /usr/local/lib/libpoppler-cairo.a

pdfposter : pdfposter.cc
