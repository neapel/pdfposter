#include <stdio.h>
#include <math.h>
#include <PDFDoc.h>
#include <splash/SplashBitmap.h>
#include <CairoOutputDev.h>
#include <GlobalParams.h>
#include <cairo-pdf.h>
#include <list>
#include <iostream>

#include "poster.h"


struct tile : rect {
	double margin, bleed, paper_w, paper_h, scale;
	rect cut;

	tile( rect c, double m, double b, double w, double h, double s )
	 : cut(c), margin(m), bleed(b), paper_w(w), paper_h(h), scale(s) {}

	void preview( cairo_t *cr ) {
		cairo_rectangle( cr, cut.x, cut.y, cut.w, cut.h );
		cairo_set_source_rgb( cr, 1, 0, 0 );
		cairo_set_line_width( cr, 1 );
		cairo_stroke( cr );
	}
};


struct poster {
	PDFDoc *pdf;
	std::list<tile> slices;
	rect in_size, bb;
	int page;


	poster( PDFDoc *pdf ) : pdf(pdf) {
		page = 1;
		in_size = rect( pdf->getPageMediaWidth(page), pdf->getPageMediaHeight(page) );
	}


	void slice( double out_w, double out_h, double scale = 2, double margin = 30, double bleed = 30 ) {
		double border = margin + bleed;
		double box_w = (out_w - 2 * border) / scale, box_h = (out_h - 2 * border) / scale;

		double in_w = in_size.w - 2 * border, in_h = in_size.h - 2 * border;

		// Choose Configuration that wastes less space
		bool normal = true;
		int normal_pages = ceil(in_w / box_w) * ceil(in_h / box_h);
		int rotated_pages = ceil(in_w / box_h) * ceil(in_h / box_w);
		if( normal_pages == rotated_pages ) {
			double normal_waste = (ceil(in_w / box_w) * box_w - in_w) + (ceil(in_h / box_h) * box_h - in_h);
			double rotated_waste = (ceil(in_w / box_w) * box_w - in_w) + (ceil(in_h / box_h) * box_h - in_h);
			normal = normal_waste <= rotated_waste;
		} else normal = normal_pages < rotated_pages;

		// Swap
		if( !normal ) {
			std::swap( out_w, out_h );
			std::swap( box_w, box_h );
		}

		// Make tiles
		int xt = (int)ceil(in_w / box_w);
		int yt = (int)ceil(in_h / box_h);
		double xshift = (in_w + 2 * border - box_w * xt) / 2;
		double yshift = (in_h + 2 * border - box_h * yt) / 2;

		for( int x = 0 ; x < xt ; x++ )
			for( int y = 0 ; y < yt ; y++ )
				slices.push_back( tile(
					rect(xshift + x * box_w, yshift + y * box_h, box_w, box_h),
					margin, bleed, out_w, out_h
				) );

		bb = tile_bounds();
	}



	void render_document_slice( cairo_t *cr, rect t ) {
		CairoOutputDev *out = new CairoOutputDev();
		out->startDoc( pdf->getXRef(), pdf->getCatalog() );
		out->setCairo( cr );
		cairo_save( cr );
		pdf->displayPageSlice( out, page, /*resX*/72, /*resY*/72, /*rotate*/0,
			/*use media box*/gTrue, /*crop*/gFalse, /*printing*/gTrue,
			/*box*/t.x, t.y, t.w, t.h );
		cairo_restore( cr );
		delete out;
	}

	void render( cairo_t *cr ) {
		for( auto I = slices.begin() ; I != slices.end() ; I++ ) {
			std::cerr << "slice " << I->cut.x << " " << I->cut.y << " " << I->cut.w << " " << I->cut.h << std::endl;
			cairo_pdf_surface_set_size( cairo_get_target( cr ), I->paper_w, I->paper_h );
			
			double border = I->margin + I->bleed;
			
			cairo_rectangle( cr, I->margin, I->margin, I->paper_w - 2 * I->margin, I->paper_h - 2 * I->margin );
			cairo_save( cr );
			cairo_clip( cr );
			cairo_translate( cr, -border, -border );

			cairo_scale( cr, I->scale, I->scale );


			render_document_slice( cr, I->cut );

			cairo_rectangle( cr, 0, 0, I->cut.w, I->cut.h );
			cairo_stroke( cr );

			cairo_restore( cr );
			cairo_show_page( cr );
		}
	}

	void render( std::string outname ) {
		cairo_surface_t *surf = cairo_pdf_surface_create( outname.c_str(), 0, 0 );
		cairo_t *cr = cairo_create( surf );
		render( cr );
		cairo_surface_destroy( surf );
		cairo_destroy( cr );
	}

	void render_preview( cairo_t *cr ) {
		cairo_save( cr );
		cairo_translate( cr, -bb.x, -bb.y );
		render_document_slice( cr, in_size );
		cairo_rectangle( cr, 0, 0, in_size.w, in_size.h );
		cairo_set_source_rgb( cr, 0, 0, 1 );
		cairo_set_line_width( cr, 3 );
		cairo_stroke( cr );
		for( auto I = slices.begin() ; I != slices.end() ; I++ )
			I->preview( cr );

		cairo_restore( cr );
	}

	void render_preview( std::string outname ) {
		cairo_surface_t *surf = cairo_pdf_surface_create( outname.c_str(), bb.w, bb.h );
		cairo_t *cr = cairo_create( surf );
		render_preview( cr );
		cairo_surface_destroy( surf );
		cairo_destroy( cr );
	}

	void render_with_preview( std::string outname ) {
		cairo_surface_t *surf = cairo_pdf_surface_create( outname.c_str(), bb.w, bb.h );
		cairo_t *cr = cairo_create( surf );
		render_preview( cr );
		cairo_show_page( cr );
		render( cr );
		cairo_surface_destroy( surf );
		cairo_destroy( cr );
	}

	rect tile_bounds() {
		rect bb = in_size;
		for( auto I = slices.begin() ; I != slices.end() ; I++ ) {
			double b = I->margin + I->bleed;
			bb.x = std::min( bb.x, I->cut.x - b );
			bb.y = std::min( bb.y, I->cut.y - b );
			bb.w = std::max( bb.w, I->cut.x + I->cut.w + b );
			bb.h = std::max( bb.h, I->cut.y + I->cut.h + b );
		}
		bb.w -= bb.x;
		bb.h -= bb.y;
		return bb;
	}

};






int main(int argc, char *argv[]) {

	globalParams = new GlobalParams();

	PDFDoc *doc = new PDFDoc( new GooString(argv[1]), NULL, NULL );

	poster p( doc );
	p.slice( 595 * 1, 842, 2, 10, 0 );
	p.render_with_preview( std::string(argv[2]) );

	return 0;
}

