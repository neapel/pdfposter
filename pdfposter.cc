#include <stdio.h>
#include <math.h>
#include <PDFDoc.h>
#include <splash/SplashBitmap.h>
#include <SplashOutputDev.h>
#include <GlobalParams.h>


int main(int argc, char *argv[]) {

	globalParams = new GlobalParams();

	GooString *fileName = new GooString(argv[1]);
	PDFDoc *doc = new PDFDoc(fileName, NULL, NULL);

	SplashColor paperColor = {255, 255, 255};
	SplashOutputDev *splashOut = new SplashOutputDev( splashModeRGB8, 4, gFalse, paperColor );
	splashOut->startDoc(doc->getXRef());

	int w = (int) ceil(doc->getPageMediaWidth(1));
	int h = (int) ceil(doc->getPageMediaHeight(1));

	doc->displayPageSlice(splashOut, 1, 72, 72, 0, gTrue, gFalse, gFalse, 0, 0, w, h );
	splashOut->getBitmap()->writePNMFile(stdout);

	return 0;
}

