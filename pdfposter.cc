#include <poppler/PDFDoc.h>
#include <poppler/CairoOutputDev.h>

int main( int argc, char **argv ) {
	auto doc = new PDFDoc( GooString(argv[1]), NULL, NULL );

	auto out = new CairoOutputDev();
	out->setCairo (cairo_t *cr);

	out->startDoc( doc->getXRef() );

	doc->displayPageSlice( out, 0, xr, yr, gFalse, gFalse, gFalse, x, y, w, h );


}

/*
splashOut = new SplashOutputDev(mono ? splashModeMono1 :
                                    gray ? splashModeMono8 :
                                             splashModeRGB8, 4,
                                  gFalse, paperColor);
  splashOut->startDoc(doc->getXRef());
  if (useCropBox) {
      pg_w = doc->getPageCropWidth(pg);
      pg_h = doc->getPageCropHeight(pg);
    } else {
      pg_w = doc->getPageMediaWidth(pg);
      pg_h = doc->getPageMediaHeight(pg);
    }
savePageSlice(doc, splashOut, pg, x, y, w, h, pg_w, pg_h, ppmFile);
static void savePageSlice(PDFDoc *doc,
                   SplashOutputDev *splashOut, 
                   int pg, int x, int y, int w, int h, 
                   double pg_w, double pg_h, 
                   char *ppmFile) {
  if (w == 0) w = (int)ceil(pg_w);
  if (h == 0) h = (int)ceil(pg_h);
  w = (x+w > pg_w ? (int)ceil(pg_w-x) : w);
  h = (y+h > pg_h ? (int)ceil(pg_h-y) : h);
  doc->displayPageSlice(splashOut, 
    pg, x_resolution, y_resolution, 
    0,
    !useCropBox, gFalse, gFalse,
    x, y, w, h
  );

  SplashBitmap *bitmap = splashOut->getBitmap();
  
  if (ppmFile != NULL) {
    if (png) {
      bitmap->writeImgFile(splashFormatPng, ppmFile);
    } else if (jpeg) {
      bitmap->writeImgFile(splashFormatJpeg, ppmFile);
    } else {
      bitmap->writePNMFile(ppmFile);
    }
  } else {
    if (png) {
      bitmap->writeImgFile(splashFormatPng, stdout);
    } else if (jpeg) {
      bitmap->writeImgFile(splashFormatJpeg, stdout);
    } else {
      bitmap->writePNMFile(stdout);
    }
  }
}

*/
