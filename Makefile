
packages = poppler cairo

CXXFLAGS += `pkg-config --cflags $(packages)` -g3
LDLIBS += `pkg-config --libs $(packages)`

pdfposter : pdfposter.cc
