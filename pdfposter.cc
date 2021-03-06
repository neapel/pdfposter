#include <stdio.h>
#include <math.h>
#include <PDFDoc.h>
#include <splash/SplashBitmap.h>
#include <CairoOutputDev.h>
#include <GlobalParams.h>
#include <cairo-pdf.h>
#include <list>
#include <iostream>


#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
#include <sstream>

#include "cmdline.h"

using namespace std;



struct rect {
	double x, y, w, h;

	rect( double x, double y, double w, double h ) : x(x), y(y), w(w), h(h) {}
	rect( double w, double h ) : x(0), y(0), w(w), h(h) {}
	rect() : x(0), y(0), w(0), h(0) {}

};




struct tile : rect {
	double margin, bleed, paper_w, paper_h, scale;
	bool top, right, bottom, left;
	rect cut;

	tile( rect c, double m, double b, double w, double h, double s, bool bt, bool br, bool bb, bool bl )
	 : cut(c), margin(m), bleed(b), paper_w(w), paper_h(h), scale(s),
	   top(bt), right(br), bottom(bb), left(bl) {}

	void preview( cairo_t *cr ) {
		cairo_rectangle( cr, cut.x, cut.y, cut.w, cut.h );
		cairo_set_source_rgb( cr, 1, 0, 0 );
		cairo_set_line_width( cr, 1 );
		cairo_stroke( cr );
	}
};


struct poster {
	PDFDoc *pdf;
	list<tile> slices;
	rect in_size, bb;
	int page;
	double mark_size;


	poster( PDFDoc *pdf ) : pdf(pdf), mark_size( 15 ) {
		page = 1;
		in_size = rect( pdf->getPageMediaWidth(page), pdf->getPageMediaHeight(page) );
	}


	void slice( double out_w, double out_h, double scale, double margin, double bleed, enum_orient strat ) {
		margin += mark_size;
		double border = margin + bleed;
		double box_w = (out_w - 2 * border) / scale, box_h = (out_h - 2 * border) / scale;

		double in_w = in_size.w /*- 2 * border / scale */, in_h = in_size.h /*- 2 * border / scale */;

		// Choose Configuration
		bool normal = true;

		double epsilon = 0.01;

		cerr << "normal: " << (in_w/box_w) << "x" << (in_h/box_h) << endl
		     << "rotated: " << (in_w/box_h) << "x" << (in_h/box_w) << endl;

		int normal_pages = ceil(in_w / box_w - epsilon) * ceil(in_h / box_h - epsilon);
		int rotated_pages = ceil(in_w / box_h - epsilon) * ceil(in_h / box_w - epsilon);
		double normal_waste = (ceil(in_w / box_w) * box_w - in_w) + (ceil(in_h / box_h) * box_h - in_h);
		double rotated_waste = (ceil(in_w / box_w) * box_w - in_w) + (ceil(in_h / box_h) * box_h - in_h);

		cerr << "normal: "<< normal_waste << endl << "rotated: " << rotated_waste << endl;

		switch( strat ) {
			// Minimize free space
			case orient_arg_waste:
				normal = normal_waste == rotated_waste
					? normal_pages < rotated_pages
					: normal_waste < rotated_waste;
				break;

			// Minimize page count
			case orient_arg_count:
				normal = normal_pages == rotated_pages
					? normal_waste < rotated_waste
					: normal_pages < rotated_pages;
				break;

			// Portrait
			case orient_arg_portrait:
				normal = box_w < box_h;
				break;

			// Landscape
			case orient_arg_landscape:
				normal = box_h < box_w;
				break;
		}

		// Swap
		if( !normal ) {
			swap( out_w, out_h );
			swap( box_w, box_h );
		}

		// Make tiles
		int xt = (int)ceil(in_w / box_w - epsilon);
		int yt = (int)ceil(in_h / box_h - epsilon);
		double xshift = (in_size.w - box_w * xt) / 2;
		double yshift = (in_size.h - box_h * yt) / 2;

		for( int x = 0 ; x < xt ; x++ )
			for( int y = 0 ; y < yt ; y++ )
				slices.push_back( tile(
					rect(xshift + x * box_w, yshift + y * box_h, box_w, box_h),
					margin, bleed, out_w, out_h, scale,
					y == 0, x == xt - 1, y == yt - 1, x == 0
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
		for( auto I = slices.begin(), i = 1 ; I != slices.end() ; I++, i++ ) {
			cerr << "rendering slice " << i << " of " << slices.size() << '\r' << flush;
			
			double w = I->paper_w, h = I->paper_h;
			double m = I->margin, b = I->bleed;
			double s = I->scale;
			double bo = m + b;
			cairo_pdf_surface_set_size( cairo_get_target( cr ), w, h );
			
			// Draw Page, clipped
			cairo_save( cr );
			cairo_rectangle( cr, m, m, w - 2 * m, h - 2 * m );
			cairo_clip( cr );
			cairo_translate( cr, bo, bo );
			cairo_scale( cr, s, s );
			render_document_slice( cr, I->cut );
			cairo_restore( cr );

			// Cut Marks
			if( mark_size > 0 ) {
				const double bx0 = max( bo, min( w - bo, bo - I->cut.x * s ) ),
				             bx1 = max( bo, min( w - bo, bo + (in_size.w - I->cut.x) * s ) ),
				             by0 = max( bo, min( h - bo, bo - I->cut.y * s ) ),
				             by1 = max( bo, min( h - bo, bo + (in_size.h - I->cut.y) * s ) );
				
				// Top left
				cairo_save( cr );
				cairo_translate( cr, bx0, by0 );
				mark( cr, b );
				cairo_restore( cr );

				// Top right
				cairo_save( cr );
				cairo_scale( cr, -1, 1 );
				cairo_translate( cr, -bx1, by0 );
				mark( cr, b );
				cairo_restore( cr );

				// Bottom left
				cairo_save( cr );
				cairo_scale( cr, 1, -1 );
				cairo_translate( cr, bx0, -by1 );
				mark( cr, b );
				cairo_restore( cr );

				// Bottom right
				cairo_save( cr );
				cairo_scale( cr, -1, -1 );
				cairo_translate( cr, -bx1, -by1 );
				mark( cr, b );
				cairo_restore( cr );

				cairo_set_source_rgb( cr, 0, 0, 0 );
				cairo_fill( cr );
			}

			// Next page
			cairo_show_page( cr );
		}
	}


	void mark( cairo_t *cr, double b ) {
		double m = mark_size;
		double s = m / 5;

		cairo_move_to( cr, -b, 0 );
		cairo_line_to( cr, -m - b, +s );
		cairo_line_to( cr, -m - b + s, 0 );
		cairo_line_to( cr, -m - b, -s );
		cairo_close_path( cr );

		cairo_move_to( cr, 0, -b );
		cairo_line_to( cr, +s, -m - b );
		cairo_line_to( cr, 0, -m - b + s );
		cairo_line_to( cr, -s, -m - b );
		cairo_close_path( cr );
	}

	void render( string outname ) {
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

	void render_preview( string outname ) {
		cairo_surface_t *surf = cairo_pdf_surface_create( outname.c_str(), bb.w, bb.h );
		cairo_t *cr = cairo_create( surf );
		render_preview( cr );
		cairo_surface_destroy( surf );
		cairo_destroy( cr );
	}

	void render_with_preview( string outname ) {
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
			bb.x = min( bb.x, I->cut.x - b );
			bb.y = min( bb.y, I->cut.y - b );
			bb.w = max( bb.w, I->cut.x + I->cut.w + b );
			bb.h = max( bb.h, I->cut.y + I->cut.h + b );
		}
		bb.w -= bb.x;
		bb.h -= bb.y;
		return bb;
	}

};





double parse_length( string s ) {
	double l;
	string e;
	istringstream(s) >> l >> e;

	if( e.size() == 0 || e == "pt" ) return l;

	if( e == "in" ) return l * 72;
	if( e == "ft" ) return l * 864;

	const double pm = 72 / 25.4;
	if( e == "mm" ) return l * pm;
	if( e == "cm" ) return l * (pm * 10);
	if( e == "m" ) return l * (pm * 1000);

	cerr << "Unknown unit '" << s << "' please use one of pt, in, ft, mm, cm or m" << endl;
	exit(-4);
}

rect parse_box( string s ) {
	size_t mid = s.find_first_of("x*,:; ");
	if( mid == string::npos ) return rect();
	return rect(
		parse_length( s.substr( 0, mid ) ),
		parse_length( s.substr( mid + 1 ) )
	);
}


rect find_paper( string n ) {

	vector< pair<string, string> > sizes = {
		// DIN/ISO Sizes
		{"A40", "1682mmx2378mm"},
		{"A20", "1189mmx1682mm"}, {"B20", "1414mmx2000mm"},
		{"A0", "841mmx1189mm"}, {"B0", "1000mmx1414mm"}, {"C0", "917mmx1297mm"}, {"D0", "771mmx1091mm"},
		{"A1", "594mmx841mm"}, {"B1", "707mmx1000mm"}, {"C1", "648mmx917mm"}, {"D1", "545mmx771mm"},
		{"A2", "420mmx594mm"}, {"B2", "500mmx707mm"}, {"C2", "458mmx648mm"}, {"D2", "385mmx545mm"},
		{"A3", "297mmx420mm"}, {"B3", "353mmx500mm"}, {"C3", "324mmx458mm"}, {"D3", "272mmx385mm"},
		{"A4", "210mmx297mm"}, {"B4", "250mmx353mm"}, {"C4", "229mmx324mm"}, {"D4", "192mmx272mm"},
		{"A5", "148mmx210mm"}, {"B5", "176mmx250mm"}, {"C5", "162mmx229mm"}, {"D5", "136mmx192mm"},
		{"A6", "105mmx148mm"}, {"B6", "125mmx176mm"}, {"C6", "114mmx162mm"}, {"D6", "96mmx136mm"},
		{"A7", "74mmx105mm"}, {"B7", "88mmx125mm"}, {"C7", "81mmx114mm"}, {"D7", "68mmx96mm"},
		{"A8", "52mmx74mm"}, {"B8", "62mmx88mm"}, {"C8", "57mmx81mm"},
		{"A9", "37mmx52mm"}, {"B9", "44mmx62mm"}, {"C9", "40mmx57mm"},
		{"A10", "26mmx37mm"}, {"B10", "31mmx44mm"}, {"C10", "28mmx40mm"},
		// ANSI Sizes
		{"Invoice", "140mmx216mm"},
		{"Executive", "184mmx267mm"},
		{"Legal", "216mmx356mm"},
		{"Letter", "216mmx279mm"}, {"A", "216mmx279mm"},
		{"Ledger","279mmx432mm"}, {"Tabloid","279mmx432mm"}, {"B","279mmx432mm"},
		{"Broadsheet", "432mmx559mm"}, {"C", "432mmx559mm"},
		{"D", "559mmx864mm"},
		{"E", "864mmx1118"},
		{"F", "711mmx1116"},
		// Canadian Sizes
		{"P6", "107mmx140mm"},
		{"P5", "140mmx215mm"},
		{"P4", "215mmx280mm"},
		{"P3", "280mmx430mm"},
		{"P2", "430mmx560mm"},
		{"P1", "560mmx860mm"}
	};

	for( auto I = sizes.begin() ; I != sizes.end() ; I++ )
		if( boost::iequals( I->first, n ) ) return parse_box( I->second );

	rect box = parse_box(n);
	if( box.w == 0 && box.h == 0 ) {
		cerr << "No known paper format '" << n << "', please specify box size as '<length>x<length>' or use one of:" << endl;
		for( auto I = sizes.begin() ; I != sizes.end() ; I++ )
			cerr << I->first << " ";
		cerr << endl;
		exit(-6);
	}

	return box;

}


int main(int argc, char *argv[]) {

	gengetopt_args_info info;
	if( cmdline_parser( argc, argv, &info ) != 0 )
		return -1;

	if( info.inputs_num != 2 ) {
		cerr << "Please specify input- and output filenames." << endl;
		return -2;
	}

	string input( info.inputs[0] ), output( info.inputs[1] );

	// Open PDF
	globalParams = new GlobalParams();

	PDFDoc *doc = new PDFDoc( new GooString( input.c_str() ), NULL, NULL );
	if( !doc ) {
		cerr << "The PDF file <" << input << "> could not be read..." << endl;
		return -3;
	}

	poster my_poster( doc );


	// Parse options
	double bleed = parse_length( string( info.cut_arg ) );
	double margin = parse_length( string( info.white_arg ) );
	rect paper = find_paper( string( info.media_arg ) );
	rect target = my_poster.in_size;
	if( info.poster_given ) target = find_paper( string( info.poster_arg ) );

	if( target.w == 0 ) target.w = target.h * my_poster.in_size.w / my_poster.in_size.h;
	else if( target.h == 0 ) target.h = target.w * my_poster.in_size.h / my_poster.in_size.w;

	double scale = min( target.w / my_poster.in_size.w, target.h / my_poster.in_size.h );
	if( info.scale_given ) scale = info.scale_arg;

	if( info.nomarks_flag ) my_poster.mark_size = 0;

	my_poster.slice( paper.w, paper.h, scale, margin, bleed, info.orient_arg );

	if( info.guide_flag ) my_poster.render_with_preview( output );
	else my_poster.render( output );


	return 0;
}

