/********************************************************************************
*                                                                               *
*  D e v i c e   C o n t e x t   F o r       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2103 by Stephen J. Hardy.   All Rights Reserved.                *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: $                     *
********************************************************************************/

#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXId.h"
#include "FXVisual.h"
#include "FXFont.h"
#include "FXDrawable.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXRegion.h"
#include "FXDC.h"
#include "FXDCWindow.h"

#include "xincs_cairo.h"

#include "config.h"
#include "FXDCCairo.h"


/*
  Notes:
*/

#define DISPLAY(app) ((Display*)((app)->getDisplay()))

using namespace FX;

namespace FX
{




// Construct for expose event painting
FXDCCairo::FXDCCairo(FXDrawable* drawable,FXEvent* event):
	FXDCWindow(drawable, event)
{
	begin(drawable);
	cairo_rectangle(cc, rect.x, rect.y, rect.w, rect.h);
	cairo_clip(cc); 
}


// Construct for normal painting
FXDCCairo::FXDCCairo(FXDrawable* drawable):
	FXDCWindow(drawable)
{
	begin(drawable);
}


// Destruct
FXDCCairo::~FXDCCairo()
{
	end();
	FXDCWindow::end();
}


void * FXDCCairo::createServerSurface(const FXDrawable * d)
{
#ifndef WIN32
	return cairo_xlib_surface_create(
	          DISPLAY(getApp()),
	          (Drawable)d->id(),
	          (Visual *)visual->getVisual(),
	          d->getWidth(),
	          d->getHeight());
#else
	return cairo_win32_surface_create((HDC)d->GetDC());
#endif
}


// Begin locks in a drawable surface
void FXDCCairo::begin(FXDrawable *drawable)
{
	if(!drawable) {
		fxerror("FXDCCairo::begin: NULL drawable.\n");
	}
	if(!drawable->id()) {
		fxerror("FXDCCairo::begin: drawable not created yet.\n");
	}
	csurf = (cairo_surface_t *)createServerSurface(drawable);
	cc = cairo_create(csurf);
	tsurf = NULL;
	ssurf = NULL;
	ksurf = NULL;
	src = NONE;
	cr_dashstyle = LINE_SOLID;
	cr_linewidth = -1;
	sharp_offset = FALSE;
	cr_fillstyle = FILL_SOLID;
	cr_tile = NULL;
	pfd = NULL;
	layout = pango_cairo_create_layout(cc);
	cr_mask = FALSE;
	
	// Make default compatible with DCWindow...
	cairo_set_fill_rule(cc, CAIRO_FILL_RULE_EVEN_ODD);
	do_sharpen = TRUE;
	FXchar dd[2];
	dd[0] = 4;
	dd[1] = 4;
	setDashes(0, dd, 2);

}


// End unlock the drawable surface; restore it
void FXDCCairo::end()
{
	surface=NULL;
	cairo_destroy(cc);
	cc = NULL;
	cairo_surface_destroy(csurf);
	csurf = NULL;
	if (tsurf) cairo_surface_destroy(tsurf);
	tsurf = NULL;
	if (ssurf) cairo_surface_destroy(ssurf);
	ssurf = NULL;
	if (ksurf) cairo_surface_destroy(ksurf);
	ksurf = NULL;
	if (pfd) pango_font_description_free(pfd);
	pfd = NULL;
	g_object_unref(layout);
	layout = NULL;	
}


// Read back pixel
FXColor FXDCCairo::readPixel(FXint x,FXint y)
{
	// Cairo does not readily support reading back pixel data.  So, just transform
	// co-ordinates back to device space and call the base class.
	// Note that cairo device coordinate (0.5, 0.5) is the "pixel centre" so floor()
	// gives the appropriate nearest pixel in integer coordinates.
	double xx = x;
	double yy = y;
	cairo_user_to_device(cc, &xx, &yy);
	return FXDCWindow::readPixel((FXint)floor(xx), (FXint)floor(yy));
}


// Draw point
void FXDCCairo::drawPoint(FXint x,FXint y)
{
	// Single pixels are not a good fit for Cairo.  Replace with an appropriately centred
	// single pixel "square" in device space.  This function should not really be used
	// for non-identity transforms.
	if(!surface) {
		fxerror("FXDCCairo::drawPoint: DC not connected to drawable.\n");
	}
	double xx = 1.;
	double yy = 1.;
	cairo_device_to_user_distance(cc, &xx, &yy);
	cairo_rectangle(cc, x-xx/2, y-yy/2, xx, yy);
	paint(FALSE, TRUE);
}


// Draw points
void FXDCCairo::drawPoints(const FXPoint* points,FXuint npoints)
{
	if(!surface) {
		fxerror("FXDCCairo::drawPoints: DC not connected to drawable.\n");
	}
	for (FXuint n = 0; n < npoints; ++n)
		drawPoint(points[n].x, points[n].y);
}


// Draw points relative
void FXDCCairo::drawPointsRel(const FXPoint* points,FXuint npoints)
{
	if(!surface) {
		fxerror("FXDCCairo::drawPointsRel: DC not connected to drawable.\n");
	}
	if (!npoints)
		return;
	short x = points[0].x;
	short y = points[0].y;
	drawPoint(x, y);
	for (FXuint n = 1; n < npoints; ++n) {
		x += points[n].x;
		y += points[n].y;
		drawPoint(x, y);
	}
}


// Draw line
void FXDCCairo::drawLine(FXint x1,FXint y1,FXint x2,FXint y2)
{
	if(!surface) {
		fxerror("FXDCCairo::drawLine: DC not connected to drawable.\n");
	}
	sharpOffset(TRUE);
	cairo_move_to(cc, x1, y1);
	cairo_line_to(cc, x2, y2);
	sharpOffset(FALSE);
	paint();
}


// Add lines to path
FXbool FXDCCairo::pathLines(const FXPoint* points,FXuint npoints,FXbool for_stroke)
{
	if(!surface) {
		fxerror("FXDCCairo::drawLines: DC not connected to drawable.\n");
	}
	if (npoints < 2)
		return FALSE;
	FXuint cp = points[0] == points[npoints-1];
	sharpOffset(for_stroke);
	cairo_move_to(cc, points[0].x, points[0].y);
	for (FXuint n = 1; n < npoints-cp; ++n)
		cairo_line_to(cc, points[n].x, points[n].y);
	if (cp)
		cairo_close_path(cc);
	sharpOffset(FALSE);
	return (FXbool)cp;
}

// Draw lines
void FXDCCairo::drawLines(const FXPoint* points,FXuint npoints)
{
	pathLines(points, npoints);
	paint();
}


// Add lines relative to path
FXbool FXDCCairo::pathLinesRel(const FXPoint* points,FXuint npoints,FXbool for_stroke)
{
	if(!surface) {
		fxerror("FXDCCairo::drawLinesRel: DC not connected to drawable.\n");
	}
	if (npoints < 2)
		return FALSE;
	short x = points[0].x;
	short y = points[0].y;
	sharpOffset(for_stroke);
	cairo_move_to(cc, x, y);
	for (FXuint n = 1; n+1 < npoints; ++n) {
		x += points[n].x;
		y += points[n].y;
		cairo_line_to(cc, x, y);
	}
	x += points[npoints-1].x;
	y += points[npoints-1].y;
	sharpOffset(FALSE);
	if (points[0].x == x && points[0].y == y) {
		cairo_close_path(cc);
		return TRUE;
	}
	else {
		cairo_line_to(cc, x, y);
		return FALSE;
	}
}

// Draw lines relative
void FXDCCairo::drawLinesRel(const FXPoint* points,FXuint npoints)
{
	pathLinesRel(points, npoints);
	paint();
}


// Draw line segments
void FXDCCairo::drawLineSegments(const FXSegment* segments,FXuint nsegments)
{
	if(!surface) {
		fxerror("FXDCCairo::drawLineSegments: DC not connected to drawable.\n");
	}
	if (nsegments < 1)
		return;
	sharpOffset(TRUE);
	for (FXuint n = 0; n < nsegments; ++n) {
		cairo_move_to(cc, segments[n].x1, segments[n].y1);
		cairo_line_to(cc, segments[n].x2, segments[n].y2);
	}
	sharpOffset(FALSE);
	paint();
}


// Draw rectangle
void FXDCCairo::drawRectangle(FXint x,FXint y,FXint w,FXint h)
{
	if(!surface) {
		fxerror("FXDCCairo::drawRectangle: DC not connected to drawable.\n");
	}
	sharpOffset(TRUE);
	cairo_rectangle(cc, x, y, w, h);
	sharpOffset(FALSE);
	paint();
}


// Draw rectangles
void FXDCCairo::drawRectangles(const FXRectangle* rectangles,FXuint nrectangles)
{
	if(!surface) {
		fxerror("FXDCCairo::drawRectangles: DC not connected to drawable.\n");
	}
	if (nrectangles < 1)
		return;
	sharpOffset(TRUE);
	for (FXuint n = 0; n < nrectangles; ++n)
		cairo_rectangle(cc, rectangles[n].x, rectangles[n].y, rectangles[n].w, rectangles[n].h);
	sharpOffset(FALSE);

	paint();
}


// Draw round rectangle
void FXDCCairo::pathRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh,FXfloat F)
{
	if(!surface) {
		fxerror("FXDCCairo::pathRoundRectangle: DC not connected to drawable.\n");
	}
	if(ew+ew>w) ew=w>>1;
	if(eh+eh>h) eh=h>>1;
	
	// Approximate corner arcs using beziers with control points positioned along the
	// edges between the end of the straight sections and the projected corners.
	// the factor F is 0 for truncated corners, about 0.3 (default) for roughly circular/elliptical,
	// 1 or more for "pointy but rounded" corners.
	double x0 = x;
	double x1 = x + ew*(1.0-F);
	double x2 = x + ew;
	double x3 = x + w - ew;
	double x4 = x3 + ew*F;
	double x5 = x + w;
	double y0 = y;
	double y1 = y + eh*(1.0-F);
	double y2 = y + eh;
	double y3 = y + h - eh;
	double y4 = y3 + eh*F;
	double y5 = y + h;
	//sharpOffset(TRUE);	// Looks better without line sharpening, since rounded sections
							// look too fuzzy in comparison.
	cairo_move_to(cc, x3, y0);
	cairo_curve_to(cc, x4, y0, x5, y1, x5, y2);
	cairo_line_to(cc, x5, y3);
	cairo_curve_to(cc, x5, y4, x4, y5, x3, y5);
	cairo_line_to(cc, x2, y5);
	cairo_curve_to(cc, x1, y5, x0, y4, x0, y3);
	cairo_line_to(cc, x0, y2);
	cairo_curve_to(cc, x0, y1, x1, y0, x2, y0);
	cairo_close_path(cc);
	//sharpOffset(FALSE);
}

void FXDCCairo::drawRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh)
{
	pathRoundRectangle(x, y, w, h, ew, eh, 0.6);
	paint();
}

// Draw arc path only, Xlib style
void FXDCCairo::pathArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2, FXbool pie, FXbool for_stroke)
{
	if(!surface) {
		fxerror("FXDCCairo::pathArc: DC not connected to drawable.\n");
	}
	FXCLAMP(-360*64, ang2, 360*64);
	if (for_stroke) sharpOffset(TRUE);
	cairo_save(cc);
	cairo_translate(cc, x + w*0.5, y + h * 0.5);
	cairo_scale(cc, w*0.5, h*0.5);
	// Now in centre of 2x2 square.
	cairo_new_sub_path(cc);
	if (ang2 < 0)
		cairo_arc(cc, 0., 0., 1., DTOR*(ang1/-64.), DTOR*((ang1+ang2)/-64.));
	else
		cairo_arc_negative(cc, 0., 0., 1., DTOR*(ang1/-64.), DTOR*((ang1+ang2)/-64.));
	if (ang2 == -360*64 || ang2 == 360*64)
		cairo_close_path(cc);
	else if (pie)
		cairo_line_to(cc, 0., 0.);
	cairo_restore(cc);
	if (for_stroke) sharpOffset(FALSE);
}

// Draw arc
void FXDCCairo::drawArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2)
{
	pathArc(x, y, w, h, ang1, ang2, FALSE, TRUE);
	paint();
}



// Draw arcs
void FXDCCairo::drawArcs(const FXArc* arcs,FXuint narcs)
{
	if(!surface) {
		fxerror("FXDCCairo::drawArcs: DC not connected to drawable.\n");
	}
	for (FXuint n = 0; n < narcs; ++n)
		drawArc(arcs[n].x, arcs[n].y, arcs[n].w, arcs[n].h, arcs[n].a, arcs[n].b);
}


// Draw ellipse
void FXDCCairo::drawEllipse(FXint x,FXint y,FXint w,FXint h)
{
	drawArc(x,y,w,h,0,23040);
}


// Fill rectangle
void FXDCCairo::fillRectangle(FXint x,FXint y,FXint w,FXint h)
{
	if(!surface) {
		fxerror("FXDCCairo::fillRectangle: DC not connected to drawable.\n");
	}
	cairo_rectangle(cc, x, y, w, h);
	paint(FALSE, TRUE);
}


// Fill rectangles
void FXDCCairo::fillRectangles(const FXRectangle* rectangles,FXuint nrectangles)
{
	if(!surface) {
		fxerror("FXDCCairo::fillRectangles: DC not connected to drawable.\n");
	}
	if (nrectangles < 1)
		return;
	// The Xlib call is not the same as filling all the rectangles at once, so we are slightly
	// less efficient.
	for (FXuint n = 0; n < nrectangles; ++n) {
		cairo_rectangle(cc, rectangles[n].x, rectangles[n].y, rectangles[n].w, rectangles[n].h);
		paint(FALSE, TRUE);
	}
}


// Fill rounded rectangle
void FXDCCairo::fillRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh)
{
	pathRoundRectangle(x, y, w, h, ew, eh, 0.6);
	paint(FALSE, TRUE);
}


// Fill chord
void FXDCCairo::fillChord(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2)
{
	pathArc(x, y, w, h, ang1, ang2, FALSE, FALSE);
	paint(FALSE, TRUE);
}


// Fill chords
void FXDCCairo::fillChords(const FXArc* chords,FXuint nchords)
{
	for (FXuint n = 0; n < nchords; ++n)
		fillChord(chords[n].x, chords[n].y, chords[n].w, chords[n].h, chords[n].a, chords[n].b);
}


// Fill arc
void FXDCCairo::fillArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2)
{
	pathArc(x, y, w, h, ang1, ang2, TRUE, FALSE);
	paint(FALSE, TRUE);
}


// Fill arcs
void FXDCCairo::fillArcs(const FXArc* arcs,FXuint narcs)
{
	for (FXuint n = 0; n < narcs; ++n)
		fillArc(arcs[n].x, arcs[n].y, arcs[n].w, arcs[n].h, arcs[n].a, arcs[n].b);
}


// Fill ellipse
void FXDCCairo::fillEllipse(FXint x,FXint y,FXint w,FXint h)
{
	pathArc(x, y, w, h, 0, 23040, FALSE, FALSE);
	paint(FALSE, TRUE);
}


// Fill polygon
void FXDCCairo::fillPolygon(const FXPoint* points,FXuint npoints)
{
	fillComplexPolygon(points, npoints);
}


// Fill concave polygon
void FXDCCairo::fillConcavePolygon(const FXPoint* points,FXuint npoints)
{
	fillComplexPolygon(points, npoints);
}


// Fill complex polygon
void FXDCCairo::fillComplexPolygon(const FXPoint* points,FXuint npoints)
{
	if (pathLines(points, npoints, FALSE))
		cairo_close_path(cc);
	paint(FALSE, TRUE);
}


// Fill polygon relative
void FXDCCairo::fillPolygonRel(const FXPoint* points,FXuint npoints)
{
	fillComplexPolygonRel(points, npoints);
}


// Fill concave polygon relative
void FXDCCairo::fillConcavePolygonRel(const FXPoint* points,FXuint npoints)
{
	fillComplexPolygonRel(points, npoints);
}


// Fill complex polygon relative
void FXDCCairo::fillComplexPolygonRel(const FXPoint* points,FXuint npoints)
{
	if (pathLinesRel(points, npoints, FALSE))
		cairo_close_path(cc);
	paint(FALSE, TRUE);
}


// Set text font
void FXDCCairo::setFont(FXFont *fnt)
{
	if(!surface) {
		fxerror("FXDCCairo::setFont: DC not connected to drawable.\n");
	}
	if(!fnt) {
		fxerror("FXDCCairo::setFont: illegal or NULL font specified.\n");
	}
	//pfd = pango_font_description_from_string (FONT);
	if (fnt != font) {
		if (!pfd)
			pfd = pango_font_description_new();
		pango_font_description_set_family(pfd, fnt->getFamily().text());
		pango_font_description_set_style(pfd,
			fnt->getSlant() == FXFont::Italic ? PANGO_STYLE_ITALIC :
			fnt->getSlant() == FXFont::Oblique ? PANGO_STYLE_OBLIQUE :
			PANGO_STYLE_NORMAL);
		pango_font_description_set_weight(pfd,
			(PangoWeight)(fnt->getWeight() * 10));
		pango_font_description_set_stretch(pfd,
			fnt->getSetWidth() == FXFont::UltraCondensed ? PANGO_STRETCH_ULTRA_CONDENSED :
			fnt->getSetWidth() == FXFont::ExtraCondensed ? PANGO_STRETCH_EXTRA_CONDENSED :
			fnt->getSetWidth() == FXFont::Condensed ? PANGO_STRETCH_CONDENSED :
			fnt->getSetWidth() == FXFont::SemiCondensed ? PANGO_STRETCH_SEMI_CONDENSED :
			fnt->getSetWidth() == FXFont::SemiExpanded ? PANGO_STRETCH_SEMI_EXPANDED :
			fnt->getSetWidth() == FXFont::Expanded ? PANGO_STRETCH_EXPANDED :
			fnt->getSetWidth() == FXFont::ExtraExpanded ? PANGO_STRETCH_EXTRA_EXPANDED :
			fnt->getSetWidth() == FXFont::UltraExpanded ? PANGO_STRETCH_ULTRA_EXPANDED :
			PANGO_STRETCH_NORMAL);
		pango_font_description_set_size(pfd,
			fnt->getSize()*PANGO_SCALE/10);
		font = fnt;
	}
}


/*

	We use Pango for text layout.  FXFont objects are used only to specify the
	face, size etc.

*/

// Draw string with base line starting at x, y
void FXDCCairo::drawText(FXint x,FXint y,const FXchar* string,FXuint length)
{
	if(!surface) {
		fxerror("FXDCCairo::drawText: DC not connected to drawable.\n");
	}
	if(!font) {
		fxerror("FXDCCairo::drawText: no font selected.\n");
	}
	pango_layout_set_text(layout, string, length);
	pango_layout_set_font_description (layout, pfd);
	paintTextLayout(x, y, FALSE);
}


// Draw text starting at x, y over filled background
void FXDCCairo::drawImageText(FXint x,FXint y,const FXchar* string,FXuint length)
{
	if(!surface) {
		fxerror("FXDCCairo::drawImageText: DC not connected to drawable.\n");
	}
	if(!font) {
		fxerror("FXDCCairo::drawImageText: no font selected.\n");
	}
	pango_layout_set_text(layout, string, length);
	pango_layout_set_font_description (layout, pfd);
	paintTextLayout(x, y, TRUE);
}




// Draw string with base line starting at x, y
void FXDCCairo::drawText(FXint x,FXint y,const FXString& string)
{
	drawText(x,y,string.text(),string.length());
}


// Draw text starting at x, y over filled background
void FXDCCairo::drawImageText(FXint x,FXint y,const FXString& string)
{
	drawImageText(x,y,string.text(),string.length());
}


// Draw area
void FXDCCairo::drawArea(const FXDrawable* source,FXint sx,FXint sy,
				FXint sw,FXint sh,FXint dx,FXint dy)
{
	// This is only compatible with FXDCWindow when identity transforms.
	if(!surface) {
		fxerror("FXDCCairo::drawArea: DC not connected to drawable.\n");
	}
	if(!source || !source->id()) {
		fxerror("FXDCCairo::drawArea: illegal source specified.\n");
	}
	//XCopyArea(DISPLAY(getApp()),source->id(),surface->id(),(GC)ctx,sx,sy,sw,sh,dx,dy);
	cairo_surface_t * ss = (cairo_surface_t *)createServerSurface(source);
	cairo_save(cc);
	cairo_set_source_surface(cc, ss, dx-sx, dy-sy);
	cairo_rectangle(cc, dx, dy, sw, sh);
	cairo_fill(cc);
	cairo_restore(cc);
	cairo_surface_destroy(ss);
}


// Draw area stretched area from source
void FXDCCairo::drawArea(const FXDrawable* source,FXint sx,FXint sy,
				FXint sw,FXint sh,FXint dx,FXint dy,FXint dw,FXint dh)
{
	if(!surface) {
		fxerror("FXDCCairo::drawArea: DC not connected to drawable.\n");
	}
	if(!source || !source->id()) {
		fxerror("FXDCCairo::drawArea: illegal source specified.\n");
	}
	cairo_surface_t * ss = (cairo_surface_t *)createServerSurface(source);
	cairo_save(cc);
	cairo_translate(cc, dx, dy);
	cairo_pattern_t * p = cairo_pattern_create_for_surface(ss);
	cairo_matrix_t m;
	cairo_matrix_init_translate(&m, sx, sy);
	cairo_matrix_scale(&m, (double)sw/dw, (double)sh/dh);
	cairo_pattern_set_matrix(p, &m);
	cairo_set_source(cc, p);
	cairo_rectangle(cc, 0., 0., dw, dh);
	cairo_fill(cc);
	cairo_restore(cc);
	cairo_pattern_destroy(p);
	cairo_surface_destroy(ss);
}


// Draw image
void FXDCCairo::drawImage(const FXImage* image,FXint dx,FXint dy)
{
	if(!surface) {
		fxerror("FXDCCairo::drawImage: DC not connected to drawable.\n");
	}
	if(!image || !image->id()) {
		fxerror("FXDCCairo::drawImage: illegal image specified.\n");
	}
	drawArea(image, 0, 0, image->getWidth(), image->getHeight(), dx, dy);
}

static FXuint fxColorToARGB32(FXColor clr)
{
	// Writes 4 bytes at dest with Cairo ARGB32 format.
	// ARGB32 has pre-multiplied alpha in the color components.
	FXuint a = FXALPHAVAL(clr);
	FXuint r = (a * FXREDVAL(clr) + 128)/255;
	FXuint g = (a * FXGREENVAL(clr) + 128)/255;
	FXuint b = (a * FXBLUEVAL(clr) + 128)/255;
	return a<<24 | r<<16 | g<<8 | b;
}

static void fxClientBitmapToCairoA1(FXuint rows, 
		FXuint fstride, FXuchar * f, 
		FXuint cstride, FXuchar * c)
{
	for (FXuint row = 0; row < rows; ++row) {
		memcpy(c, f, fstride);
		c += cstride;
		f += fstride;
	}
}

static void fxClientBitmapToCairoARGB32(FXuint rows, FXuint w,
		FXuint fstride, FXuchar * f, 
		FXuint cstride, FXuint * c,
		FXColor fg, FXColor bg)
{
	//printf("fxClientBitmapToCairoARGB32: rows=%u w=%u fs=%u cs=%u\n",
	//	rows, w, fstride, cstride);
	FXuint fgc = fxColorToARGB32(fg);
	FXuint bgc = fxColorToARGB32(bg);
	FXuint * r;
	FXuchar pix;
	FXuint i;
	for (FXuint row = 0; row < rows; ++row) {
		r = c;
		for (i = 0; i < w>>3; ++i) {
			pix = f[i];
			for (FXuint j = 0; j < 8; ++j, pix >>= 1)
				*(r++) = pix&1 ? fgc : bgc;
		}
		if (w&7) {
			pix = f[i];
			for (FXuint j = 0; j < w&7; ++j, pix >>= 1)
				*(r++) = pix&1 ? fgc : bgc;
		}
		c = (FXuint *)((FXuchar *)c + cstride);
		f += fstride;
		//if (row == 248) return;
	}
}

static void fxClientFXColorToCairoA1(FXuint rows,
		FXuint fstride, FXColor * f, 
		FXuint cstride, FXuchar * c,
		FXuchar alpha_thresh,
		FXdouble lum_thresh)
{
	// Fill a Cairo A1 surface with 1 bit if FXColor alpha >= alpha_thresh,
	// and its NTSC luminescence is >= lum_thresh.
	FXuint cols = fstride>>2;
	FXuchar setbit;
	for (FXuint row = 0; row < rows; ++row) {
		for (FXuint i = 0; i < cols; ++i) {
			if (!(i&7))
				setbit = 1;
			else
				setbit <<= 1;
			if (FXALPHAVAL(f[i]) >= alpha_thresh &&
			    FXREDVAL(f[i])*0.3 + FXGREENVAL(f[i])*0.59 + FXBLUEVAL(f[i])*0.11 >= lum_thresh)
				c[i>>3] |= setbit;
			else
				c[i>>3] &= ~setbit;
		}
		c += cstride;
		f += fstride>>2;
	}
}


static void fxClientFXColorToCairoA1(FXuint rows,
		FXuint fstride, FXColor * f, 
		FXuint cstride, FXuchar * c,
		FXColor transparent)
{
	// Fill a Cairo A1 surface with 1 bit if FXColor is not 'transparent'.
	FXuint cols = fstride>>2;
	FXuchar setbit;
	for (FXuint row = 0; row < rows; ++row) {
		for (FXuint i = 0; i < cols; ++i) {
			if (!(i&7))
				setbit = 1;
			else
				setbit <<= 1;
			if (f[i] != transparent)
				c[i>>3] |= setbit;
			else
				c[i>>3] &= ~setbit;
		}
		c += cstride;
		f += fstride>>2;
	}
}


static void fxClientFXColorToCairoA8(FXuint rows,
		FXuint fstride, FXColor * f, 
		FXuint cstride, FXuchar * c)
{
	// Fill a Cairo A8 surface with alpha channel.
	FXuint cols = fstride>>2;
	for (FXuint row = 0; row < rows; ++row) {
		for (FXuint i = 0; i < cols; ++i) {
			c[i] = FXALPHAVAL(f[i]);
		}
		c += cstride;
		f += fstride>>2;
	}
}


static void fxClientFXColorToCairoARGB32(FXuint rows,
		FXuint fstride, FXColor * f,
		FXuint cstride, FXuint * c)
{
	FXuint cols = FXMIN(fstride, cstride)>>2;
	for (FXuint row = 0; row < rows; ++row) {
		for (FXuint i = 0; i < cols; ++i) {
			c[i] = fxColorToARGB32(f[i]);
		}
		c += cstride>>2;
		f += fstride>>2;
	}
}

static void fxClientFXColorTransparencyToCairoARGB32(FXuint rows,
		FXuint fstride, FXColor * f, 
		FXuint cstride, FXuint * c,
		FXColor transparent)
{
	// Copies ARGB data directly, unless it matches the given "transparent"
	// color, in which case it is made transparent black (0).
	FXuint cols = FXMIN(fstride, cstride)>>2;
	for (FXuint row = 0; row < rows; ++row) {
		for (FXuint i = 0; i < cols; ++i) {
			if (f[i] == transparent)
				c[i] = 0;
			else
				c[i] = fxColorToARGB32(f[i]);
		}
		c += cstride>>2;
		f += fstride>>2;
	}
}

static cairo_surface_t * fxToCairoClient(const FXBitmap * b)
{
	// Return Cairo image surface with CAIRO_FORMAT_A1 (i.e. 1 bit alpha)
	// If the bitmap does not have the pixel data in the client-side buffer,
	// then we temporarily cast away const and issue restore() to grab the
	// bitmap from the server.
	// FIXME: it might be better to use cairo_xlib_surface_create_for_bitmap() for b->id(),
	// and leave it server-side, but for now keep bitmaps (alpha masks) client-side.
	if (!b->getData()) {
		// This is expensive (FOX gets it one pixel at a time) but it's one-time-only.
		((FXBitmap *)b)->restore();	// This sets IMAGE_OWNED, so b's dtor will manage it.
	}
	FXuchar * data = b->getData();
	if (!data)
		return NULL;
	FXuint dstride = (b->getWidth()+7)>>3;
	// Data from fox is not multiple of 32 bits per row, so we need to copy :-(
	//FIXME: bug in pixman needs extra row allocated.
	cairo_surface_t * s = cairo_image_surface_create(CAIRO_FORMAT_A1, b->getWidth(), b->getHeight()+1);
	fxClientBitmapToCairoA1(b->getHeight(), dstride, data, 
		cairo_image_surface_get_stride(s), cairo_image_surface_get_data(s));
	cairo_surface_mark_dirty(s);
	return s;
}

static cairo_surface_t * fxToCairoClient(const FXBitmap * b, FXColor fg, FXColor bg)
{
	// Similar to the above, except creates a full ARGB32 surface with fg for bitmap pixels
	// which were 1, bg for zero pixels.
	if (!b->getData()) {
		((FXBitmap *)b)->restore();	// This sets IMAGE_OWNED, so b's dtor will manage it.
	}
	FXuchar * data = b->getData();
	if (!data)
		return NULL;
	FXuint dstride = (b->getWidth()+7)>>3;
	// Data from fox is not multiple of 32 bits per row, so we need to copy :-(
	//FIXME: bug in pixman needs extra row allocated.
	cairo_surface_t * s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, b->getWidth(), b->getHeight()+1);
	//printf("fxToCairoClient: %s\n", cairo_status_to_string(cairo_surface_status(s)));
	fxClientBitmapToCairoARGB32(b->getHeight(), b->getWidth(), dstride, data, 
		cairo_image_surface_get_stride(s), (FXuint *)cairo_image_surface_get_data(s),
		fg, bg);
	cairo_surface_mark_dirty(s);
	return s;
}

static cairo_surface_t * fxToCairoClient(const FXImage * b)
{
	// Copy client side FXColor pixels to an ARGB32 surface.
	if (!b->getData()) {
		((FXImage *)b)->restore();
	}
	FXColor * data = b->getData();
	if (!data)
		return NULL;
	FXuint dstride = b->getWidth()*sizeof(FXColor);
	//FIXME: bug in pixman needs extra row allocated.
	cairo_surface_t * s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, b->getWidth(), b->getHeight()+1);
	fxClientFXColorToCairoARGB32(b->getHeight(), dstride, data, 
		cairo_image_surface_get_stride(s), (FXuint *)cairo_image_surface_get_data(s));
	cairo_surface_mark_dirty(s);
	return s;
}

static cairo_surface_t * fxToCairoClient(const FXImage * b, FXColor transparent)
{
	// As above, but mask out matching color to transparent black.
	if (!b->getData()) {
		((FXImage *)b)->restore();
	}
	FXColor * data = b->getData();
	if (!data)
		return NULL;
	FXuint dstride = b->getWidth()*sizeof(FXColor);
	//FIXME: bug in pixman needs extra row allocated.
	cairo_surface_t * s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, b->getWidth(), b->getHeight()+1);
	fxClientFXColorTransparencyToCairoARGB32(b->getHeight(), dstride, data, 
		cairo_image_surface_get_stride(s), (FXuint *)cairo_image_surface_get_data(s),
		transparent);
	cairo_surface_mark_dirty(s);
	return s;
}

static cairo_surface_t * fxToCairoClientMask(const FXImage * b, FXColor transparent)
{
	// As above, but use FOX-compatible algorithm to obtain mask (alpha channel) from FXIcon.
	// Returns a Cairo A1 surface, or NULL.
	// NOTE: if passing an FXIcon, use icon->getTransparentColor() for the 'transparent' parameter.
	//       Otherwise, take a guess or just use 0.
	// If options&IMAGE_OPAQUE: mask is just a rectangle, so return NULL.
	// Else if options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS): mask determined by matching transparent color
	// Else image has an alpha channel.  Unfortunately, we can't tell whether the original alpha
	// channel is in the client buffer, since it might have been restored from the server side at
	// some point.  Thus, the best result cannot be achieved!  We return NULL as if all opaque.
	// [To get nice anti-aliased icons, need a new Icon class.]
	//FIXME: for now, we are ignoring the fact that the alpha channel may be screwed.  If IMAGE_KEEP
	// is set for the image, then this will work out nicely.
	if (b->getOptions() & IMAGE_OPAQUE ||
	    !(b->getData() || b->getOptions() & (IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)))
		return NULL;
	if (!b->getData())
		((FXImage *)b)->restore();
	FXColor * data = b->getData();
	if (!data)
		return NULL;
	FXuint dstride = b->getWidth()*sizeof(FXColor);
	//FIXME: bug in pixman needs extra row allocated.
	cairo_surface_t * s;
	if (b->getOptions() & (IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)) {
		s = cairo_image_surface_create(CAIRO_FORMAT_A1, b->getWidth(), b->getHeight()+1);
		fxClientFXColorToCairoA1(b->getHeight(), dstride, data,
			cairo_image_surface_get_stride(s), cairo_image_surface_get_data(s),
			transparent);
	}
	else {
		// Hopefully have proper alpha channel.
		s = cairo_image_surface_create(CAIRO_FORMAT_A8, b->getWidth(), b->getHeight()+1);
		fxClientFXColorToCairoA8(b->getHeight(), dstride, data,
			cairo_image_surface_get_stride(s), cairo_image_surface_get_data(s));
	}
	cairo_surface_mark_dirty(s);
	return s;
}

// Draw bitmap
void FXDCCairo::drawBitmap(const FXBitmap* bitmap,FXint dx,FXint dy)
{
	if(!surface) fxerror("FXDCCairo::drawBitmap: DC not connected to drawable.\n");
	if(!bitmap || !bitmap->id()) fxerror("FXDCCairo::drawBitmap: illegal bitmap specified.\n");
	//XCopyPlane(DISPLAY(getApp()),bitmap->id(),surface->id(),(GC)ctx,0,0,bitmap->width,bitmap->height,dx,dy,1);
/*
	XCopyPlane is like a FillOpaqueStippled using the source plane as a bitmap.  In this case the
	source plane is the bitmap itself.
	
	Note that use of fxToCairoClient() represents an inefficiency.  We need to develop
	a specialized FXImage/FXBitmap class which is more compatible with Cairo.  e.g. it could
	cache the cairo_surface_t.
*/
	cairo_surface_t * s = fxToCairoClient(bitmap, fg, bg);
	if (!s) {
		fxerror("FXDCCairo::drawBitmap: no client pixel buffer available.\n");
		return;
	}
	
	cairo_save(cc);
	cairo_set_source_surface(cc, s, dx, dy);
	cairo_rectangle(cc, dx, dy, bitmap->getWidth(), bitmap->getHeight());
	cairo_fill(cc);
	cairo_restore(cc);
	cairo_surface_destroy(s);	// Encounters mem corruption!  WTF?
	
}


// Draw a vanilla icon
void FXDCCairo::drawIcon(const FXIcon* icon,FXint dx,FXint dy)
{
	if(!surface) {
		fxerror("FXDCCairo::drawIcon: DC not connected to drawable.\n");
	}
	if(!icon || !icon->id() /*|| !icon->shape*/) {
		fxerror("FXDCCairo::drawIcon: illegal icon specified.\n");
	}
	cairo_surface_t * s = fxToCairoClientMask(icon, icon->getTransparentColor());
	// May return NULL if all  opaque.  Otherwise, s is alpha mask
	cairo_save(cc);
	cairo_surface_t * ss = (cairo_surface_t *)createServerSurface(icon);
	cairo_set_source_surface(cc, ss, dx, dy);
	if (s)
		cairo_mask_surface(cc, s, dx, dy);
	else {
		cairo_rectangle(cc, dx, dy, icon->getWidth(), icon->getHeight());
		cairo_fill(cc);
	}
	cairo_restore(cc);
	cairo_surface_destroy(ss);
	if (s) cairo_surface_destroy(s);
}


// Draw a shaded icon, like when it is selected
void FXDCCairo::drawIconShaded(const FXIcon* icon,FXint dx,FXint dy)
{
	if(!surface) {
		fxerror("FXDCCairo::drawIconShaded: DC not connected to drawable.\n");
	}
	if(!icon || !icon->id() /*|| !icon->shape */) {
		fxerror("FXDCCairo::drawIconShaded: illegal icon specified.\n");
	}
/*
	With Xlib, we blit the icon then stipple 50% coverage with the "selection background" color.
	The equivalent with Cairo is to set dest = 0.5 selback + 0.5 image as a belending op.
	Best imitation is by drawing the icon as usual, then overdrawing using a partially transparent
	color.
*/
	cairo_surface_t * s = fxToCairoClientMask(icon, icon->getTransparentColor());
	cairo_save(cc);
	cairo_surface_t * ss = (cairo_surface_t *)createServerSurface(icon);
	FXColor clr = getApp()->getSelbackColor();
	cairo_set_source_surface(cc, ss, dx, dy);
	if (s) {
		cairo_mask_surface(cc, s, dx, dy);
		cairo_set_source_rgba(cc, FXREDVAL(clr)/255., FXGREENVAL(clr)/255., 
									FXBLUEVAL(clr)/255., 0.5);
		cairo_mask_surface(cc, s, dx, dy);
	}
	else {
		cairo_rectangle(cc, dx, dy, icon->getWidth(), icon->getHeight());
		cairo_fill_preserve(cc);
		cairo_set_source_rgba(cc, FXREDVAL(clr)/255., FXGREENVAL(clr)/255., 
									FXBLUEVAL(clr)/255., 0.5);
		cairo_fill(cc);
	}
	cairo_restore(cc);
	cairo_surface_destroy(ss);
	if (s) cairo_surface_destroy(s);

}


// This draws a sunken icon
void FXDCCairo::drawIconSunken(const FXIcon* icon,FXint dx,FXint dy)
{
/*
	This uses the 'etch' mask to draw in white at offset(1,1) then 85% of "base color" at
	offset (0,0).
	There is no need, however, to be too literal.  We can draw a "sunken" icon (i.e. indicating
	non-selectability) using the CAIRO_OPERATOR_HSL_LUMINOSITY operator.  This retains
	the hue and saturation of the destination, setting just the luminosity of the icon.
	Optionally we can lower the image contrast by overlaying semi-transparent background color.
*/
	if(!surface) {
		fxerror("FXDCCairo::drawIconSunken: DC not connected to drawable.\n");
	}
	if(!icon || !icon->id() /*|| !icon->etch */) {
		fxerror("FXDCCairo::drawIconSunken: illegal icon specified.\n");
	}
	
	cairo_surface_t * s = fxToCairoClientMask(icon, icon->getTransparentColor());
	cairo_save(cc);
	cairo_surface_t * ss = (cairo_surface_t *)createServerSurface(icon);
	FXColor clr = getApp()->getBaseColor();
	
	// Place base color background
	cairo_set_source_rgba(cc, FXREDVAL(clr)/255., FXGREENVAL(clr)/255., 
								FXBLUEVAL(clr)/255., 1.0);
	if (s) {
		cairo_mask_surface(cc, s, dx, dy);
	}
	else {
		cairo_rectangle(cc, dx, dy, icon->getWidth(), icon->getHeight());
		cairo_fill(cc);
	}
	
	// Draw icon in matching monochrome
	cairo_set_operator(cc, CAIRO_OPERATOR_HSL_LUMINOSITY);
	cairo_set_source_surface(cc, ss, dx, dy);
	if (s) {
		cairo_mask_surface(cc, s, dx, dy);
	}
	else {
		cairo_rectangle(cc, dx, dy, icon->getWidth(), icon->getHeight());
		cairo_fill(cc);
	}
	
	// Reduce contrast by alpha-over
	cairo_set_source_rgba(cc, FXREDVAL(clr)/255., FXGREENVAL(clr)/255., 
								FXBLUEVAL(clr)/255., 0.7);
	if (s) {
		cairo_mask_surface(cc, s, dx, dy);
	}
	else {
		cairo_rectangle(cc, dx, dy, icon->getWidth(), icon->getHeight());
		cairo_fill(cc);
	}
	
	cairo_restore(cc);
	cairo_surface_destroy(ss);
	if (s) cairo_surface_destroy(s);

}


// Draw hash box
void FXDCCairo::drawHashBox(FXint x,FXint y,FXint w,FXint h,FXint b)
{
	XGCValues gcv;
	if(!surface) {
		fxerror("FXDCCairo::drawHashBox: DC not connected to drawable.\n");
	}
	// Draw 50% transparent black on the inside boundary of the given rectangle
	cairo_save(cc);
	cairo_set_line_width(cc, b);
	sharpOffset(TRUE);
	cairo_rectangle(cc, x+0.5*b, y+0.5*b, w-b, h-b);
	cairo_set_source_rgba(cc, 0., 0., 0., 0.5);
	cairo_stroke(cc);
	sharpOffset(FALSE);
	cairo_restore(cc);
}


// Draw focus rectangle
void FXDCCairo::drawFocusRectangle(FXint x,FXint y,FXint w,FXint h)
{
	XGCValues gcv;
	if(!surface) {
		fxerror("FXDCCairo::drawFocusRectangle: DC not connected to drawable.\n");
	}
	// Draw "single width" line in 70% transparent dark red around inside of focus rectangle.
	cairo_save(cc);
	cairo_set_line_width(cc, 1.5);
	cairo_rectangle(cc, x+0.75, y+0.75, w-1.5, h-1.5);
	cairo_set_source_rgba(cc, 0.7, 0., 0., 0.7);
	cairo_stroke(cc);
	cairo_restore(cc);
}

void FXDCCairo::sharpOffset(FXbool on)
{
	// Note that we have to do this while generating the path, since the path is
	// transformed as it is generated.  Can't defer to stroke, unfortunately.
	// This all depends on the transform and linewidth being set while the path is
	// being generated, which is not the best.
	if (!do_sharpen)
		return;
	if (on) {
		sharp_offset = FALSE;
		if (width & 1 || !width) {
			// Hack: if identity transform, and odd line width, then transform offset by 0.5
			// pixels.  This makes horizontal/vertical odd-pixel-width lines "sharp" because they are not
			// anti-aliased onto half adjacent lines.
			double xx = 1.;
			double yy = 1.;
			cairo_device_to_user_distance(cc, &xx, &yy);
			if (xx == 1. && yy == 1.) {
				sharp_offset = TRUE;
				cairo_translate(cc, 0.5, 0.5);
			}
		}
	}
	else {
		if (sharp_offset) {
			sharp_offset = FALSE;
			cairo_translate(cc, -0.5, -0.5);
		}
	}
}

void FXDCCairo::setSourceRGBA(FXColor clr)
{
	if (src != RGBA || clr != src_rgba) {
		cairo_set_source_rgba(cc, FXREDVAL(clr)/255., FXGREENVAL(clr)/255., 
									FXBLUEVAL(clr)/255., FXALPHAVAL(clr)/255.);
		src_rgba = clr;
		src = RGBA;
	}
}

void FXDCCairo::setSourceTile()
{
	if (src != TILE || tile != cr_tile
		|| cr_tx != tx || cr_ty != ty) {
		if (tile != cr_tile) {
			if (tsurf)
				cairo_surface_destroy(tsurf);
			tsurf = (cairo_surface_t *)createServerSurface(tile);
		}
		cairo_set_source_surface(cc, tsurf, tx, ty);
		cairo_pattern_t * pat = cairo_get_source(cc);
		cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
		cr_tx = tx;
		cr_ty = ty;
		cr_tile = tile;
		src = TILE;
	}
}

void FXDCCairo::setSource(FXbool alternative)
{
	// Set appropriate source for normal fill/stroke operations
	if (fill != cr_fillstyle) {
		// Recompute all
		src = NONE;
		cr_fillstyle = fill;
	}
	
	switch (cr_fillstyle) {
	default:
	case FILL_SOLID:
		setSourceRGBA(alternative ? bg : fg);
		break;
	case FILL_TILED:
		setSourceTile();
		break;
	case FILL_STIPPLED: //FIXME
		setSourceTile();
		break;
	case FILL_OPAQUESTIPPLED:
		setSourceTile();
		break;
	}
}


void FXDCCairo::paint(FXbool stroke, FXbool fill, FXbool preserve)
{
	// Perform cairo_stroke()/fill (or both) with special hacks to emulate FXDC.
	// Optionally preserve path.
	// By default, fill is done with bg color if also stroking, else fg.
	
	if (cr_mask) {
		cairo_push_group(cc);
		//FIXME: ideally we want to clip to the mask extent (cx,cy,w,h)
		// but we don't want to trash the current path.  Need a way of saving/restoring
		// the current path.
	}
	
	if (fill) {
		if (stroke)
			setSource(TRUE);
		else
			setSource();
		if (preserve || stroke)
			cairo_fill_preserve(cc);
		else
			cairo_fill(cc);
	}
	if (stroke) {
		if (width != cr_linewidth) {
			if (!width) {
				// Find equivalent of 1 device pixel width.  This only works if used under the
				// current transform.
				double xx = 1.;
				double yy = 1.;
				cairo_device_to_user_distance(cc, &xx, &yy);
				cairo_set_line_width(cc, FXMAX(xx, yy));
			}
			else
				cairo_set_line_width(cc, width);
			cr_linewidth = width;
		}
		if (dashlen && style == LINE_DOUBLE_DASH) {
			// Emulate "double dash" by stroking with permuted dashes in bg color.
			// A simpler approach would be to simply stroke the entire line without dashes,
			// but that would not be quite the same if alpha is considered.
			// (If not using CAP_BUTT then even this is not accurate with alpha).
			cairo_save(cc);
			setSourceRGBA(bg);
			double dd[32];
			for (FXuint i = 0; i < dashlen; ++i)
				dd[i] = dashpatp[i];
			cairo_set_dash(cc, dd, dashlen, dashoffp);
			cairo_stroke_preserve(cc);
			cairo_restore(cc);
		}
		if (style != cr_dashstyle) {
			if (style > LINE_SOLID) {
				double dd[32];
				for (FXuint i = 0; i < dashlen; ++i)
					dd[i] = dashpat[i];
				cairo_set_dash(cc, dd, dashlen, dashoff);
			}
			else
				cairo_set_dash(cc, NULL, 0, 0.);
			cr_dashstyle = style;
		}
		setSource();
		if (preserve)
			cairo_stroke_preserve(cc);
		else
			cairo_stroke(cc);
		
	}
	if (cr_mask) {
		cairo_pop_group_to_source(cc);
		cairo_mask_surface(cc, ksurf, cx, cy);
	}
}

void FXDCCairo::paintTextLayout(double x, double y, FXbool fillbg)
{
	if (cr_mask) {
		cairo_push_group(cc);
	}

	// x,y are starting baseline position
	cairo_save(cc);
	cairo_translate(cc, x, y);
	if (font->getAngle()) {
		cairo_rotate(cc, font->getAngle()/-64.*DTOR);
	}
	y = -pango_layout_get_baseline(layout)/PANGO_SCALE;
	if (fillbg) {
		PangoRectangle logext;
		pango_layout_get_extents(layout, NULL, &logext);
		cairo_save(cc);
		cairo_rectangle(cc, logext.x/PANGO_SCALE, y + logext.y/PANGO_SCALE,
							logext.width/PANGO_SCALE, logext.height/PANGO_SCALE);
		cairo_set_source_rgba(cc, FXREDVAL(bg)/255., FXGREENVAL(bg)/255.,
										FXBLUEVAL(bg)/255., FXALPHAVAL(bg)/255.);
		cairo_fill(cc);
		cairo_restore(cc);
	}
	cairo_move_to(cc, 0, y);
	setSource();
	pango_cairo_update_layout(cc, layout);
	pango_cairo_show_layout (cc, layout);
	cairo_restore(cc);
	if (cr_mask) {
		cairo_pop_group_to_source(cc);
		cairo_mask_surface(cc, ksurf, cx, cy);
	}
	
}





// Set foreground color
void FXDCCairo::setForeground(FXColor clr)
{
	fg=clr;
}


// Set background color
void FXDCCairo::setBackground(FXColor clr)
{
	bg=clr;
}


// Set dashes
void FXDCCairo::setDashes(FXuint dashoffset,const FXchar *dashpattern,FXuint dashlength)
{
	FXuint len, i;
/*
	From FOX API:
		Set dash pattern and dash offset.
		A dash pattern of [1 2 3 4] is a repeating pattern of 1 foreground pixel,
		2 background pixels, 3 foreground pixels, and 4 background pixels.
		The offset is where in the pattern the system will start counting.
		The maximum length of the dash pattern is 32.
	From man XSetDashes:
       The XSetDashes function sets the dash-offset and dash-list attributes for dashed line
       styles in the specified GC.  There must be at least one element in the specified
       dash_list, or a BadValue error results.  The initial and alternating elements (second,
       fourth, and so on) of the dash_list are the even dashes, and the others are the odd
       dashes.  Each element specifies a dash length in pixels.  All of the elements must be
       nonzero, or a BadValue error results.  Specifying an odd-length list is equivalent to
       specifying the same list concatenated with itself to produce an even-length list.

       The dash-offset defines the phase of the pattern, specifying how many pixels into the
       dash-list the pattern should actually begin in any single graphics request.  Dashing is
       continuous through path elements combined with a join-style but is reset to the dash-off-
       set between each sequence of joined lines.
    From Cairo doc:
		Sets the dash pattern to be used by cairo_stroke(). A dash pattern is specified by dashes, 
		an array of positive values. Each value provides the length of alternate "on" and "off" 
		portions of the stroke. The offset specifies an offset into the pattern at which the stroke 
		begins.

		Each "on" segment will have caps applied as if the segment were a separate sub-path. In 
		particular, it is valid to use an "on" length of 0.0 with CAIRO_LINE_CAP_ROUND or 
		CAIRO_LINE_CAP_SQUARE in order to distributed dots or squares along a path.

		Note: The length values are in user-space units as evaluated at the time of stroking. 
		This is not necessarily the same as the user space at the time of cairo_set_dash().

		If num_dashes is 0 dashing is disabled.

		If num_dashes is 1 a symmetric pattern is assumed with alternating on and off portions of 
		the size specified by the single value in dashes.
		
	Bringing it together:
		We ignore the FOX and XLib restriction on the dash length, and allow zero length of
		even dashes.  We retain the 32-entry limit.  dashoffset is a pixel offset, which we
		assume is in user coordinates for Cairo.
*/
	for(i=len=0; i<dashlength; i++) {
		dashpat[i]=dashpattern[i];
		len+=(FXuint)dashpattern[i];
	}
	if (i == 1)
		dashpat[dashlength++] = dashpat[0];
	for (i = 1; i < dashlength; ++i)
		dashpatp[i] = dashpat[i-1];
	dashpatp[0] = dashpat[i-1];
	dashlen=dashlength;
	dashoff=dashoffset%len;
	dashoffp = (dashoff + dashpatp[0]) % len;
}


// Set line width
void FXDCCairo::setLineWidth(FXuint linewidth)
{
	width=linewidth;
}


// Set line cap style
void FXDCCairo::setLineCap(FXCapStyle capstyle)
{
	if(!surface) {
		fxerror("FXDCCairo::setLineCap: DC not connected to drawable.\n");
	}
	cairo_set_line_cap(cc, 
		capstyle == CAP_ROUND ? CAIRO_LINE_CAP_ROUND :
		capstyle == CAP_PROJECTING ? CAIRO_LINE_CAP_SQUARE :
		CAIRO_LINE_CAP_BUTT);
	cap=capstyle;
}


// Set line join style
void FXDCCairo::setLineJoin(FXJoinStyle joinstyle)
{
	if(!surface) {
		fxerror("FXDCCairo::setLineJoin: DC not connected to drawable.\n");
	}
	// FIXME: need to add miter limit API.
	cairo_set_line_join(cc, 
		joinstyle == JOIN_MITER ? CAIRO_LINE_JOIN_MITER :
		joinstyle == JOIN_ROUND ? CAIRO_LINE_JOIN_ROUND :
		CAIRO_LINE_JOIN_BEVEL);
	join=joinstyle;
}


// Set line style
void FXDCCairo::setLineStyle(FXLineStyle linestyle)
{
	// No direct equivalent of "double dash".  Need to emulate this by permuting the
	// dash pattern and re-stroking.  This is done in paint().
	style=linestyle;
	
}


// Set fill style
void FXDCCairo::setFillStyle(FXFillStyle fillstyle)
{
	fill=fillstyle;
}


// Set polygon fill rule
void FXDCCairo::setFillRule(FXFillRule fillrule)
{
	if(!surface) {
		fxerror("FXDCCairo::setFillRule: DC not connected to drawable.\n");
	}
	cairo_set_fill_rule(cc, fillrule == RULE_WINDING ? CAIRO_FILL_RULE_WINDING : CAIRO_FILL_RULE_EVEN_ODD);
	rule=fillrule;
}


// Set raster function
void FXDCCairo::setFunction(FXFunction func)
{
	// This has no real equivalent in a vector-based renderer.  Save the desired op
	// just in case we can emulate in some situations.
	rop=func;
}


// Set tile pattern
void FXDCCairo::setTile(FXImage* image,FXint dx,FXint dy)
{
	// Tile is not part of cairo state.  It is used as a pattern source when painting
	// or stroking.
	tile=image;
	tx=dx;
	ty=dy;
}


// Set stipple bitmap
void FXDCCairo::setStipple(FXBitmap* bitmap,FXint dx,FXint dy)
{
	// Stippling is not efficient.
	stipple=bitmap;
	pattern=STIPPLE_NONE;
	tx=dx;
	ty=dy;
}


// Set stipple pattern
void FXDCCairo::setStipple(FXStipplePattern pat,FXint dx,FXint dy)
{
	if(pat>STIPPLE_CROSSDIAG) pat=STIPPLE_CROSSDIAG;
	//FXASSERT(getApp()->stipples[pat]);
/*
	gcv.stipple=getApp()->stipples[pat];
	gcv.ts_x_origin=dx;
	gcv.ts_y_origin=dy;
	XChangeGC(DISPLAY(getApp()),(GC)ctx,GCTileStipXOrigin|GCTileStipYOrigin|GCStipple,&gcv);
	if(dx) flags|=GCTileStipXOrigin;
	if(dy) flags|=GCTileStipYOrigin;
*/
	stipple=NULL;
	pattern=pat;
	tx=dx;
	ty=dy;
}


// Set clip region
void FXDCCairo::setClipRegion(const FXRegion& region)
{
	if(!cc) {
		fxerror("FXDCCairo::setClipRegion: DC not connected to drawable.\n");
	}
/*FIXME
	Punt on this one for now.  'Region' is a construct for rectangle mathematics.  Need a
	function to turn FXRegion into a path, which can be added using cairo_clip().
	Cairo actually has functions for this, but whether it is worth doing...?
	
	XSetRegion(DISPLAY(getApp()),(GC)ctx,(Region)region.region);///// Should intersect region and rect??
#ifdef HAVE_XFT_H
	XftDrawSetClip((XftDraw*)xftDraw,(Region)region.region);
#endif
	flags|=GCClipMask;
*/
}


// Set clip rectangle
void FXDCCairo::setClipRectangle(FXint x,FXint y,FXint w,FXint h)
{
	if(!cc) {
		fxerror("FXDCCairo::setClipRectangle: DC not connected to drawable.\n");
	}
	cairo_rectangle(cc, x, y, w, h);
	cairo_clip(cc);
}


// Set clip rectangle
void FXDCCairo::setClipRectangle(const FXRectangle& rectangle)
{
	if(!cc) {
		fxerror("FXDCCairo::setClipRectangle: DC not connected to drawable.\n");
	}
	cairo_rectangle(cc, rectangle.x, rectangle.y, rectangle.w, rectangle.h);
	cairo_clip(cc);
}


// Clear clip rectangle
void FXDCCairo::clearClipRectangle()
{
	if(!cc) {
		fxerror("FXDCCairo::clearClipRectangle: DC not connected to drawable.\n");
	}
	
	cairo_reset_clip(cc);
}


// Set clip mask
void FXDCCairo::setClipMask(FXBitmap* bitmap,FXint dx,FXint dy)
{
	if(!cc) {
		fxerror("FXDCCairo::setClipMask: DC not connected to drawable.\n");
	}
	if(!bitmap) {
		fxerror("FXDCCairo::setClipMask: illegal mask specified.\n");
	}
/*
	Cairo does not model masking this way (because of its vector model).
	What we can do is use the cairo_mask() function when we paint instead
	of cairo_paint(), however we need a temporary surface to become the
	source, with this surface having the contents of the current fill or
	stroke.  Cairo helps here with cairo_push_group() etc.
	
	Since this is relatively inefficient, and there are better ways to
	achieve the desired effect, this API should not be used.  Use path
	clipping instead.
	
	Even worse, it will not work as expected if transforms are used, since
	the mask location travels with the user space coords, rather than staying
	fixed to the location (and scale) at the point of this API call.
*/
	if (!cr_mask || bitmap != mask) {
		mask=bitmap;
		cr_mask = TRUE;
		if (ksurf)
			cairo_surface_destroy(ksurf);
		ksurf = fxToCairoClient(bitmap);
	}
	cx=dx;
	cy=dy;
}


// Clear clip mask
void FXDCCairo::clearClipMask()
{
	cr_mask=FALSE;
	cx=0;
	cy=0;
}


// Set clip child windows
void FXDCCairo::clipChildren(FXbool yes)
{
/*FIXME
	Not sure how to implement this.  We would have to use the Cairo device functions,
	but don't know if Cairo would keep the Xlib setting when we return to it.
	Since this is only relevant for layout managers (normal widgets don't have child
	windows) we punt on this for now.
	Note: Cairo Xlib surfaces default to clipping child windows.

	if(yes) {
		XSetSubwindowMode(DISPLAY(getApp()),(GC)ctx,ClipByChildren);
#ifdef HAVE_XFT_H
		XftDrawSetSubwindowMode((XftDraw*)xftDraw,ClipByChildren);
#endif
		flags&=~GCSubwindowMode;
	} else {
		XSetSubwindowMode(DISPLAY(getApp()),(GC)ctx,IncludeInferiors);
#ifdef HAVE_XFT_H
		XftDrawSetSubwindowMode((XftDraw*)xftDraw,IncludeInferiors);
#endif
		flags|=GCSubwindowMode;
	}
*/
}

} //namespace

