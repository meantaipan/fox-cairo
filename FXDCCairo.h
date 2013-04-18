/********************************************************************************
*                                                                               *
*  D e v i c e   C o n t e x t   F o r   PangeCairo graphics         *
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
* $Id: $                        *
********************************************************************************/
#ifndef FXDCCAIRO_H
#define FXDCCAIRO_H

#ifndef FXDC_H
#include "FXDC.h"
#endif

namespace FX
{


class FXApp;
class FXDrawable;
class FXImage;
class FXBitmap;
class FXIcon;
class FXFont;
class FXVisual;


/**
* Cairo Device Context
*
* The Cairo Device Context allows drawing into an FXDrawable, such as an
* on-screen window (FXWindow and derivatives) or an off-screen image (FXImage
* and its derivatives).
* Usage is an extension of FXDCWindow, except uses Cairo context instead of
* Xlib (or GDI).  Also uses Pango for text layout.
* Note that we derive from FXDCWindow, to make it easier to call old code
* which was written for Xlib/GDI.  Rendering is not designed to be pixel-for-
* pixel identical to FXDCWindow, but should usually be an improvement.
*
* Note that Cairo uses float coordinates, whereas FXDC API uses int.  Since this is
* restrictive (particularly when transforms are used), subclass FXDCCairoEx 
* provides all floating point parameters.
*
* Since Pango is used for text, multi-line text and UTF-8 is handled correctly
* (although only the default PangoLayout settings are available at this level)
* and will thus look different than FXDCWindow for anything but the simplest text.
* drawImageText() also computes the background box correctly.  With FXDCWindow,
* the font must have had create() called, however this class allows FXFont's
* without this restriction.  Pango has its own way of finding fonts, and
* arbitrary rotation and scaling is always available.
*
* Note that there are a *lot* of differences between a vector-based renderer
* (like Cairo) and a pixel-based renderer (like Xlib/GDI).  The following
* things from the FXDC abstract API are to be avoided because of poor efficiency
* or non-implementation:
*  Aything to do with FXBitmap or FXIcon e.g. stipple, drawIcon etc.
*  setFunction() - raster ops are a hack for bit planes - not implemented
*  setClipRegion() - don't implement region math
*  setClipMask() - uses bitmask
*  clipChildren() - too low-level, not implemented.  Always clips child windows.
*  readPixel(), drawPoint() - pixel things are bad at the best of times.
*  setLineStyle() - double-dash works, but is inefficient in this class.
*  setFillStyle() - stippling is to be avoided.  Try to use alpha blending,
*      or use tiling instead of opaque stipple.
* We really need an FXVectorImage and FXVectorIcon class to complement the
* pixel versions.  For now, it is best to keep the client-side data if possible
* by using the IMAGE_KEEP option.  Otherwise, it tries to get the data back from
* the server, but that's a lot of unnecessary protocol overhead.
*
* This class is intended as a base class for DC's which add more access to the
* Cairo API.  In itself, this class restricts all public access to just that
* part of Cairo which implements the FXDC API.  The only exception is
* setLineSharpen().
* (TODO:)
* If the application wants to make more use of Cairo or Pango, use FXDCCairoEx.
* For better results with images and icons, try FXVectorImage and FXVectorIcon.
*/
class FXAPI FXDCCairo : public FXDCWindow
{
protected:

#if HAVE_CAIRO_H
	cairo_t * cc;				  // Cairo drawing context
	cairo_surface_t * csurf;	  // Cairo drawing surface
	cairo_surface_t * tsurf;	  // Cached tile surface
	cairo_surface_t * ssurf;	  // Cached stipple surface
	cairo_surface_t * ksurf;	  // Cached clip mask surface (A1 or A8)
	PangoFontDescription * pfd;   // Cached Pango objects
	PangoLayout * layout;
#else
	// Avoid any header dependency for application code
	void * cc;
	void * csurf;
	void * tsurf;
	void * ssurf;
	void * ksurf;
	void * pfd;
	void * layout;
#endif
	enum {
		NONE,
		RGBA,
		TILE,
		STIPPLE,
	} src;
	FXColor src_rgba;
	// Permuted dash pattern+offset for double dash
 	FXchar  dashpatp[32];
	FXuint  dashoffp;
	FXLineStyle cr_dashstyle;
	FXuint cr_linewidth;
	FXbool sharp_offset;
	FXFillStyle cr_fillstyle;
	FXImage * cr_tile;
	double cr_tx;
	double cr_ty;
	FXbool cr_mask;
	double cr_cx;
	double cr_cy;
	FXbool do_sharpen;
	
	virtual void * createServerSurface(const FXDrawable * d);
	void sharpOffset(FXbool on);
	void setSourceRGBA(FXColor clr);
	void setSourceTile();
	virtual void setSource(FXbool alternative = FALSE);
	virtual void paint(FXbool stroke=TRUE, FXbool fill=FALSE, FXbool preserve=FALSE);
	virtual void paintTextLayout(double x, double y, FXbool fillbg=FALSE);
	
	// Some path construction methods to emulate FXDCWindow semantics...
	
	// Add lines to path.  Returns TRUE if closed path i.e. first and last point the same.
	virtual FXbool pathLines(const FXPoint* points,FXuint npoints,FXbool for_stroke=TRUE);
	virtual FXbool pathLinesRel(const FXPoint* points,FXuint npoints,FXbool for_stroke=TRUE);
	// Add arc path, Xlib style.  If pie is true, then a final line to arc centre is added unless the
	// arc is exactly 360 degrees. (Note that units are still in Xlib style 1/64 degree).
	virtual void pathArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2, 
					FXbool pie=FALSE,FXbool for_stroke=TRUE);
	// Add path of rounded rectangle with ellipse with ew and ellipse height eh and pointiness
	// factor F (F=0.6 for roughly circular/elliptical)
	virtual void pathRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh,FXfloat F);
	



private:
	FXDCCairo();
	FXDCCairo(const FXDCCairo&);
	FXDCCairo &operator=(const FXDCCairo&);
public:

	/// Construct for painting in response to expose;
	/// This sets the clip rectangle to the exposed rectangle
	FXDCCairo(FXDrawable* drawable,FXEvent* event);

	/// Construct for normal drawing;
	/// This sets clip rectangle to the whole drawable
	FXDCCairo(FXDrawable* drawable);

	/// Begin locks in a drawable surface
	void begin(FXDrawable *drawable);

	/// End unlock the drawable surface
	void end();

	/// Read back pixel
	virtual FXColor readPixel(FXint x,FXint y);

	/// Draw points
	virtual void drawPoint(FXint x,FXint y);
	virtual void drawPoints(const FXPoint* points,FXuint npoints);
	virtual void drawPointsRel(const FXPoint* points,FXuint npoints);

	/// Draw lines
	virtual void drawLine(FXint x1,FXint y1,FXint x2,FXint y2);
	virtual void drawLines(const FXPoint* points,FXuint npoints);
	virtual void drawLinesRel(const FXPoint* points,FXuint npoints);
	virtual void drawLineSegments(const FXSegment* segments,FXuint nsegments);

	/// Draw rectangles
	virtual void drawRectangle(FXint x,FXint y,FXint w,FXint h);
	virtual void drawRectangles(const FXRectangle* rectangles,FXuint nrectangles);

	/// Draw rounded rectangle with ellipse with ew and ellipse height eh
	virtual void drawRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh);

	/// Draw arcs
	virtual void drawArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
	virtual void drawArcs(const FXArc* arcs,FXuint narcs);

	/// Draw ellipse
	virtual void drawEllipse(FXint x,FXint y,FXint w,FXint h);

	/// Filled rectangles
	virtual void fillRectangle(FXint x,FXint y,FXint w,FXint h);
	virtual void fillRectangles(const FXRectangle* rectangles,FXuint nrectangles);

	/// Filled rounded rectangle with ellipse with ew and ellips height eh
	virtual void fillRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh);

	/// Fill chord
	virtual void fillChord(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
	virtual void fillChords(const FXArc* chords,FXuint nchords);

	/// Draw arcs
	virtual void fillArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
	virtual void fillArcs(const FXArc* arcs,FXuint narcs);

	/// Fill ellipse
	virtual void fillEllipse(FXint x,FXint y,FXint w,FXint h);

	/// Filled polygon
	virtual void fillPolygon(const FXPoint* points,FXuint npoints);
	virtual void fillConcavePolygon(const FXPoint* points,FXuint npoints);
	virtual void fillComplexPolygon(const FXPoint* points,FXuint npoints);

	/// Filled polygon with relative points
	virtual void fillPolygonRel(const FXPoint* points,FXuint npoints);
	virtual void fillConcavePolygonRel(const FXPoint* points,FXuint npoints);
	virtual void fillComplexPolygonRel(const FXPoint* points,FXuint npoints);

	/// Draw hashed box.  Uses alpha blend-over instead of stippling.
	virtual void drawHashBox(FXint x,FXint y,FXint w,FXint h,FXint b=1);

	/// Draw focus rectangle.  Uses alpha blend-over (reddish line) instead of
	/// stippling.
	virtual void drawFocusRectangle(FXint x,FXint y,FXint w,FXint h);

	/// Draw area from source
	virtual void drawArea(const FXDrawable* source,FXint sx,FXint sy,
			FXint sw,FXint sh,FXint dx,FXint dy);

	/// Draw area stretched area from source
	virtual void drawArea(const FXDrawable* source,FXint sx,FXint sy,
			FXint sw,FXint sh,FXint dx,FXint dy,FXint dw,FXint dh);

	/// Draw image
	virtual void drawImage(const FXImage* image,FXint dx,FXint dy);

	/// Draw bitmap
	virtual void drawBitmap(const FXBitmap* bitmap,FXint dx,FXint dy);

	/// Draw icon
	virtual void drawIcon(const FXIcon* icon,FXint dx,FXint dy);
	virtual void drawIconShaded(const FXIcon* icon,FXint dx,FXint dy);
	virtual void drawIconSunken(const FXIcon* icon,FXint dx,FXint dy);

	/// Draw string with base line starting at x, y
	virtual void drawText(FXint x,FXint y,const FXString& string);
	virtual void drawText(FXint x,FXint y,const FXchar* string,FXuint length);

	/// Draw text starting at x, y over filled background
	virtual void drawImageText(FXint x,FXint y,const FXString& string);
	virtual void drawImageText(FXint x,FXint y,const FXchar* string,FXuint length);

	/// Set foreground/background drawing color
	virtual void setForeground(FXColor clr);
	virtual void setBackground(FXColor clr);

	/// Set dash pattern
	virtual void setDashes(FXuint dashoffset,const FXchar *dashpattern,FXuint dashlength);

	/// Set line width
	virtual void setLineWidth(FXuint linewidth=0);

	/// Set line cap style
	virtual void setLineCap(FXCapStyle capstyle=CAP_BUTT);

	/// Set line join style
	virtual void setLineJoin(FXJoinStyle joinstyle=JOIN_MITER);

	/// Set line style
	virtual void setLineStyle(FXLineStyle linestyle=LINE_SOLID);

	/// Set fill style
	virtual void setFillStyle(FXFillStyle fillstyle=FILL_SOLID);

	/// Set fill rule
	virtual void setFillRule(FXFillRule fillrule=RULE_EVEN_ODD);

	/// Set blit function.  Not reasonable to emulate bit-wise operators in Cairo.
	/// This function does nothing useful.
	virtual void setFunction(FXFunction func=BLT_SRC);

	/// Set the tile
	virtual void setTile(FXImage* tile,FXint dx=0,FXint dy=0);

	/// Set the stipple pattern
	virtual void setStipple(FXBitmap *stipple,FXint dx=0,FXint dy=0);

	/// Set the stipple pattern
	virtual void setStipple(FXStipplePattern stipple,FXint dx=0,FXint dy=0);

	/// Set clip region.  Not implemented.
	virtual void setClipRegion(const FXRegion& region);

	/// Set clip rectangle
	virtual void setClipRectangle(FXint x,FXint y,FXint w,FXint h);

	/// Set clip rectangle
	virtual void setClipRectangle(const FXRectangle& rectangle);

	/// Clear clipping
	virtual void clearClipRectangle();

	/// Set clip mask
	virtual void setClipMask(FXBitmap* mask,FXint dx=0,FXint dy=0);

	/// Clear clip mask
	virtual void clearClipMask();

	/// Set font to draw text with.  Font does not have to be create()d for this
	/// class, since a Pango font descriptor is created from the font attributes
	/// so we don't need any server-side data.
	virtual void setFont(FXFont *fnt);

	/// Clip against child windows.  Null implementation; Cairo defaults to clipping
	/// against children.
	virtual void clipChildren(FXbool yes);

	/// Destructor
	virtual ~FXDCCairo();
	
	/// This causes fine lines to appear "sharper".  Defaults to 'on' when constructed.
	/// If 'on', then this causes a half pixel offset in x and y when generating the
	/// path for stroked graphics, but only for an identity transform and when the line width
	/// is set to an odd integer.  
	/// Since Cairo considers integer pixel coordinates to be *between* pixel sampling
	/// points, the half pixel offset will mean that an odd width line will cover exactly that
	/// width of pixels rather than the default Cairo behavior of covering 2 pixel rows with 
	/// 50% alpha.
	/// Compared with Xlib/GDI, without this setting fine lines will appear "fuzzy" and some
	/// applications may not like that look.  It is, however, a hack to fit vector graphics
	/// in a pixellated world.  The default for FXDCCairoEx is *not* to set this since it
	/// adds some overhead, requires the line width to be known while generating the path,
	/// and does not help with non-identity transforms.
	void setLineSharpen(FXbool on = TRUE) { do_sharpen = on; }
	
};

}

#endif
