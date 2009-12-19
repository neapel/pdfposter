#include <stdio.h>
#include <math.h>
#include <PDFDoc.h>
#include <splash/SplashBitmap.h>
#include <CairoOutputDev.h>
#include <GlobalParams.h>
#include <cairo-pdf.h>

#include "poster.h"


void 




int main(int argc, char *argv[]) {

	globalParams = new GlobalParams();

	PDFDoc *doc = new PDFDoc( new GooString(argv[1]), NULL, NULL );

	CairoOutputDev *out = new CairoOutputDev();
	out->startDoc( doc->getXRef(), doc->getCatalog() );

	double out_width = 595, out_height = 842;
	cairo_surface_t *surf = cairo_pdf_surface_create( argv[2], out_width, out_height );
	cairo_t *cr = cairo_create( surf );

	//out->startProfile();
	out->setCairo( cr );


	int w = (int) ceil(doc->getPageMediaWidth(1));
	int h = (int) ceil(doc->getPageMediaHeight(1));

	doc->displayPageSlice(out, 1, 72, 72, 0, gTrue, gFalse, gFalse, 0, 0, w, h );
	//splashOut->getBitmap()->writePNMFile(stdout);

	out->setCairo(NULL);
	//out->endProfile();


	cairo_surface_destroy(surf);
	cairo_destroy(cr);

	return 0;
}

