#ifndef __POSTER_H__
#define __POSTER_H__

#include <array>
#include <string>


struct rect {
	double x, y, w, h;

	rect( double x, double y, double w, double h ) : x(x), y(y), w(w), h(h) {}
	rect( double w, double h ) : x(0), y(0), w(w), h(h) {}
	rect() : x(0), y(0), w(0), h(0) {}

};




std::pair<std::string, rect> paper[] = {
	{"letter", rect(612, 792)},
	{"tabloid", rect(792, 1224)},
	{"ledger", rect(1224, 792)},
	{"legal", rect(612, 1008)},
	{"statement", rect(396, 612)},
	{"executive", rect(540, 720)},
	{"a0", rect(2384, 3371)},
	{"a1", rect(1685, 2384)},
	{"a2", rect(1190, 1684)},
	{"a3", rect(842, 1190)},
	{"a4", rect(596, 842)},
	{"a5", rect(420, 595)},
	{"b4", rect(729, 1032)},
	{"b5", rect(516, 729)},
	{"folio", rect(612, 936)},
	{"quarto", rect(610, 780)},
	{"10x14", rect(720, 1008)}
};





/*

*/

#endif
