/********************************************************************************
*                                                                               *
*          Render stuff using FXDCWindow and FXDCCairo, for comparison          *
*                                                                               *
********************************************************************************/
#include "xincs_cairo.h"
#include "fx.h"
#include "FXDCCairo.h"

#include "FXPNGImage.h"
#include "FXPNGIcon.h"
#include "images.cpp"

// Main Window
class ScribbleWindow : public FXMainWindow
{

	// Macro for class hierarchy declarations
	FXDECLARE(ScribbleWindow)

private:

	FXHorizontalFrame *contents;                // Content frame
	FXVerticalFrame   *canvasFrame;             // Canvas frame
	FXVerticalFrame   *buttonFrame;             // Button frame
	FXCanvas          *canvasw;                 // Canvas to draw into with FXDCWindow
	FXCanvas          *canvasc;                 // Canvas to draw into with FXDCCairo
	
	FXColorWell * backfill;
	FXColorWell * fg;
	FXColorWell * bg;
	FXColorWell * selbg;
	FXColorWell * basec;
	
	int tt;		// Test type (ID_STROKE, ID_FILL, ID_IMAGE etc.)
	
	FXImage * leaves_250h;
	FXImage * car_250h;
	FXImage * wolf_250h;
	FXImage * leaves_500w;
	FXImage * car_500w;
	FXImage * wolf_500w;
	FXBitmap * wolf_bits;
	FXIcon * wolf_icon;
	FXImage * tile;
	
	FXFont * font_1;
	FXFont * font_2;
	FXFont * font_3;
	
protected:
	ScribbleWindow() {}

public:
	void doTest(FXCanvas * cnv, FXDCWindow & dc);
	void test_stroke(FXDCWindow & dc, int w, int h);
	void test_fill(FXDCWindow & dc, int w, int h);
	void test_image(FXDCWindow & dc, int w, int h);
	void test_text(FXDCWindow & dc, int w, int h);
	void test_clip(FXDCWindow & dc, int w, int h);

	// Message handlers
	long onPaintW(FXObject*,FXSelector,void*);
	long onPaintC(FXObject*,FXSelector,void*);
	long onCmdBtn(FXObject*,FXSelector,void*);

public:

	// Messages for our class
	enum {
		ID_CANVASW=FXMainWindow::ID_LAST,
		ID_CANVASC,
		ID_STROKE,
		ID_FILL,
		ID_IMAGE,
		ID_TEXT,
		ID_CLIP,
		ID_LAST
	};

public:

	// ScribbleWindow's constructor
	ScribbleWindow(FXApp* a);

	// Initialize
	virtual void create();

	virtual ~ScribbleWindow();
};



// Message Map for the Scribble Window class
FXDEFMAP(ScribbleWindow) ScribbleWindowMap[]= {

	//________Message_Type_____________________ID____________Message_Handler_______
	FXMAPFUNC(SEL_PAINT,    ScribbleWindow::ID_CANVASW, ScribbleWindow::onPaintW),
	FXMAPFUNC(SEL_PAINT,    ScribbleWindow::ID_CANVASC, ScribbleWindow::onPaintC),
	FXMAPFUNCS(SEL_COMMAND,  ScribbleWindow::ID_STROKE, 
	                         ScribbleWindow::ID_CLIP, ScribbleWindow::onCmdBtn),
};



// Macro for the ScribbleApp class hierarchy implementation
FXIMPLEMENT(ScribbleWindow,FXMainWindow,ScribbleWindowMap,ARRAYNUMBER(ScribbleWindowMap))


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



// Construct a ScribbleWindow
ScribbleWindow::ScribbleWindow(FXApp *a):
	FXMainWindow(a,"Test Cairo DC",NULL,NULL,DECOR_ALL,0,0,1500,1100)
{

	contents=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0);

	// LEFT pane to contain the canvas
	canvasFrame=new FXVerticalFrame(contents,
		FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,10,10,10,10);


	// Drawing canvas
	canvasw = new FXCanvas(canvasFrame,this,ID_CANVASW,
			FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL);
	canvasc = new FXCanvas(canvasFrame,this,ID_CANVASC,
			FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL);

	// RIGHT pane for the buttons
	buttonFrame=new FXVerticalFrame(contents,FRAME_SUNKEN|LAYOUT_FILL_Y|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,10,10,10,10);

	// Buttons
	new FXButton(buttonFrame,"&Stroke",NULL,this,ID_STROKE,BUTTON_NORMAL|LAYOUT_FILL_X,0,0,0,0,10,10,5,5);
	new FXButton(buttonFrame,"&Fill",NULL,this,ID_FILL,BUTTON_NORMAL|LAYOUT_FILL_X,0,0,0,0,10,10,5,5);
	new FXButton(buttonFrame,"&Image",NULL,this,ID_IMAGE,BUTTON_NORMAL|LAYOUT_FILL_X,0,0,0,0,10,10,5,5);
	new FXButton(buttonFrame,"&Text",NULL,this,ID_TEXT,BUTTON_NORMAL|LAYOUT_FILL_X,0,0,0,0,10,10,5,5);
	new FXButton(buttonFrame,"&Clip",NULL,this,ID_CLIP,BUTTON_NORMAL|LAYOUT_FILL_X,0,0,0,0,10,10,5,5);
	
	FXMatrix * mat = new FXMatrix(buttonFrame, 2, MATRIX_BY_COLUMNS);
	new FXLabel(mat, "Back fill");
	backfill = new FXColorWell(mat, FXRGB(255,255,255));
	new FXLabel(mat, "Background");
	bg = new FXColorWell(mat, FXRGB(128,128,128));
	new FXLabel(mat, "Foreground");
	fg = new FXColorWell(mat, FXRGB(0,0,0));
	new FXLabel(mat, "Base");
	basec = new FXColorWell(mat, a->getBaseColor(), NULL, 0, COLORWELL_NORMAL|COLORWELL_SOURCEONLY);
	new FXLabel(mat, "Sel. back");
	selbg = new FXColorWell(mat, a->getSelbackColor(), NULL, 0, COLORWELL_NORMAL|COLORWELL_SOURCEONLY);
	
	tt = ID_STROKE;
	
	leaves_250h = new FXPNGImage(getApp(), fall14250h);
	car_250h = new FXPNGImage(getApp(), car250h);
	wolf_250h = new FXPNGImage(getApp(), wolf250h);
	wolf_icon = new FXPNGIcon(getApp(), wolf250h, 0, IMAGE_KEEP);
	leaves_500w = new FXPNGImage(getApp(), fall14500w);
	car_500w = new FXPNGImage(getApp(), car500w);
	wolf_500w = new FXPNGImage(getApp(), wolf500w);
	tile = new FXPNGImage(getApp(), tile90x90);
	
	FXuchar * pix;
	FXMALLOC(&pix, FXuchar, (wolf_250h->getWidth()+7>>3)*wolf_250h->getHeight());
	fxClientFXColorToCairoA1(wolf_250h->getHeight(),
		wolf_250h->getWidth()<<2, wolf_250h->getData(), 
		wolf_250h->getWidth()+7>>3, pix,
		128,
		0);
	wolf_bits = new FXBitmap(getApp(), pix, BITMAP_KEEP|BITMAP_OWNED, wolf_250h->getWidth(), wolf_250h->getHeight());
	
	// FXFont(FXApp* a,const FXString& face,FXuint size,FXuint weight=FXFont::Normal,
	// FXuint slant=FXFont::Straight,FXuint encoding=FONTENCODING_DEFAULT,
	// FXuint setwidth=FXFont::NonExpanded,FXuint h=0);
	font_1 = new FXFont(a, "Helvetica", 30);
	font_2 = new FXFont(a, "Times", 12);
	font_3 = new FXFont(a, "Vivaldi", 100, FXFont::Normal, FXFont::Italic);
}


ScribbleWindow::~ScribbleWindow()
{
	delete leaves_250h;
	delete car_250h;
	delete wolf_250h;
	delete leaves_500w;
	delete car_500w;
	delete wolf_500w;
	delete wolf_bits;
	delete wolf_icon;
	delete tile;
	delete font_1;
	delete font_2;
	delete font_3;
}


// Create and initialize
void ScribbleWindow::create()
{
	leaves_250h->create();
	car_250h->create();
	wolf_250h->create();
	wolf_icon->create();
	leaves_500w->create();
	car_500w->create();
	wolf_500w->create();
	wolf_bits->create();
	tile->create();
	font_1->create();
	font_2->create();
	font_3->create();

	// Create the windows
	FXMainWindow::create();

	// Make the main window appear
	show(PLACEMENT_SCREEN);
}

static FXPoint points[13] = {
	{ 5, 0},
	{ 5, 6},
	{ 8, 2},
	{ 2, 2},
	{ 2, 3},
	{ 6, 3},
	{ 6, 4},
	{ 4, 4},
	{ 4, 0},
	{ 0, 4},
	{ 1, 1},
	{ 2, 6},
	{ 5, 0},
};

void ScribbleWindow::test_stroke(FXDCWindow & dc, int w, int h)
{
	// Test drawing points, lines, rectangles, arcs.
	FXuint i, j;
	FXint x, y, xx, yy;
	FXPoint p[13];
	FXArc arcs[4];
/*
	virtual void setForeground(FXColor clr);
	virtual void setBackground(FXColor clr);
	virtual void setDashes(FXuint dashoffset,const FXchar *dashpattern,FXuint dashlength);
	virtual void setLineWidth(FXuint linewidth=0);
	virtual void setLineCap(FXCapStyle capstyle=CAP_BUTT);
	virtual void setLineJoin(FXJoinStyle joinstyle=JOIN_MITER);
	virtual void setLineStyle(FXLineStyle linestyle=LINE_SOLID);
	
	virtual void drawPoint(FXint x,FXint y);
	virtual void drawPoints(const FXPoint* points,FXuint npoints);
	virtual void drawPointsRel(const FXPoint* points,FXuint npoints);
	virtual void drawLine(FXint x1,FXint y1,FXint x2,FXint y2);
	virtual void drawLines(const FXPoint* points,FXuint npoints);
	virtual void drawLinesRel(const FXPoint* points,FXuint npoints);
	virtual void drawLineSegments(const FXSegment* segments,FXuint nsegments);
	virtual void drawRectangle(FXint x,FXint y,FXint w,FXint h);
	virtual void drawRectangles(const FXRectangle* rectangles,FXuint nrectangles);
	virtual void drawRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh);
	virtual void drawArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
	virtual void drawArcs(const FXArc* arcs,FXuint narcs);
	virtual void drawEllipse(FXint x,FXint y,FXint w,FXint h);
*/
	dc.setForeground(fg->getRGBA());
	dc.setBackground(bg->getRGBA());
	for (i = 0; i < 10; ++i) {
		x = 10;
		y = 10 + i*20;
		xx = 100;
		yy = y + i*3;
		dc.setLineWidth(i);
		dc.drawLine(x, y, xx, yy);
	}
	dc.setLineCap(CAP_ROUND);
	for (i = 0; i < 10; ++i) {
		x = 220;
		y = 10 + i*20;
		xx = 110;
		yy = y + i*3;
		dc.setLineWidth(i);
		dc.drawLine(x, y, xx, yy);
	}
	dc.setLineCap(CAP_PROJECTING);
	for (i = 0; i < 10; ++i) {
		x = 10+i*20;
		y = 250;
		xx = x - i*2;
		yy = 350;
		dc.setLineWidth(i);
		dc.drawLine(x, y, xx, yy);
	}
	dc.setLineWidth(10);
	dc.setLineCap(CAP_ROUND);
	for (i = 0; i < 12; ++i)
		p[i] = points[i]*20 + FXPoint(250, 20);
	for (i = 0; i < 3; ++i) {
		dc.setLineJoin(i==0?JOIN_MITER:i==1?JOIN_ROUND:JOIN_BEVEL);
		dc.drawLines(p, 12);
		for (j = 0; j < 12; ++j)
			p[j] += FXPoint(0, 150);
	}
	for (i = 0; i < 13; ++i)
		p[i] = points[i]*20 + FXPoint(400, 20);
	// As above, but 1st and last points the same, to close path
	for (i = 0; i < 3; ++i) {
		dc.setLineJoin(i==0?JOIN_MITER:i==1?JOIN_ROUND:JOIN_BEVEL);
		dc.drawLines(p, 13);
		for (j = 0; j < 13; ++j)
			p[j] += FXPoint(0, 150);
	}
	
	for (i = 0; i < 12; ++i)
		p[i] = points[i]*20 + FXPoint(550, 20);
	dc.drawPoints(p, 13);
	
	for (i = 0; i < 12; ++i)
		p[i] += FXPoint(0, 150);
	dc.drawLineSegments((const FXSegment*)p, 6);
	for (i = 0; i < 12; ++i)
		p[i] += FXPoint(0, 150);
	dc.setLineCap(CAP_PROJECTING);
	dc.drawLineSegments((const FXSegment*)p, 6);
	
	dc.setLineJoin(JOIN_MITER);
	for (i = 0; i < 4; ++i) {
		dc.setLineWidth(i*3+1);
		dc.drawRectangle(700+i*20, 20+i*20, 100, 50);
	}
	dc.drawHashBox(700+i*20, 20+i*20, 100, 50, 6);
	++i;
	dc.drawFocusRectangle(700+i*20, 20+i*20, 100, 50);
	for (i = 0; i < 4; ++i) {
		dc.setLineWidth(i*3+1);
		dc.drawRoundRectangle(700+i*20, 200+i*40, 100, 50, 20, 20);
	}
	
	for (i = 0; i < 4; ++i) {
		arcs[i].x = 900;
		arcs[i].y = 20 + i*100;
		arcs[i].w = 100;
		arcs[i].h = 80;
	}
	arcs[0].a = 64 * 45; arcs[0].b = 64 * 270;
	arcs[1].a = 64 * 45; arcs[1].b = 64 * -270;
	arcs[2].a = 64 * -30; arcs[2].b = 64 * 270;
	arcs[3].a = 64 * -30; arcs[3].b = 64 * -270;
	dc.setLineCap(CAP_BUTT);
	dc.setLineWidth(4);
	dc.drawArcs(arcs, 4);
	
	for (i = 0; i < 4; ++i) {
		dc.drawEllipse(1020, 20+i*100, 100-i*20, 20+i*20);
	}
	
	dc.setLineWidth(10);
	dc.setLineCap(CAP_BUTT);
	FXchar dd[2];
	dd[0] = 20;
	dd[1] = 10;
	dc.setDashes(0, dd, 2);
	dc.setLineStyle(LINE_ONOFF_DASH);
	for (i = 0; i < 12; ++i)
		p[i] = points[i]*20 + FXPoint(1150, 20);
	for (i = 0; i < 3; ++i) {
		dc.setLineJoin(i==0?JOIN_MITER:i==1?JOIN_ROUND:JOIN_BEVEL);
		if (i == 1)
			dc.setLineStyle(LINE_DOUBLE_DASH);
		if (i == 2)
			dc.setLineStyle(LINE_ONOFF_DASH);
		dc.drawLines(p, 12);
		for (j = 0; j < 12; ++j)
			p[j] += FXPoint(0, 150);
	}

	
}

void ScribbleWindow::test_fill(FXDCWindow & dc, int w, int h)
{
	FXuint i, j;
	FXint x, y, xx, yy;
	FXPoint p[13];
	FXArc arcs[4];
/*
	virtual void setForeground(FXColor clr);
	virtual void setFillStyle(FXFillStyle fillstyle=FILL_SOLID);
  FILL_SOLID,                     /// Fill with solid color
  FILL_TILED,                     /// Fill with tiled bitmap
  FILL_STIPPLED,                  /// Fill where stipple mask is 1
  FILL_OPAQUESTIPPLED             /// Fill with foreground where mask is 1, background otherwise
	virtual void setFillRule(FXFillRule fillrule=RULE_EVEN_ODD);
	virtual void setFunction(FXFunction func=BLT_SRC);
	virtual void setTile(FXImage* tile,FXint dx=0,FXint dy=0);
	virtual void setStipple(FXBitmap *stipple,FXint dx=0,FXint dy=0);
	virtual void setStipple(FXStipplePattern stipple,FXint dx=0,FXint dy=0);

	virtual void fillRectangle(FXint x,FXint y,FXint w,FXint h);
	virtual void fillRectangles(const FXRectangle* rectangles,FXuint nrectangles);
	virtual void fillRoundRectangle(FXint x,FXint y,FXint w,FXint h,FXint ew,FXint eh);
	virtual void fillChord(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
	virtual void fillChords(const FXArc* chords,FXuint nchords);
	virtual void fillArc(FXint x,FXint y,FXint w,FXint h,FXint ang1,FXint ang2);
	virtual void fillArcs(const FXArc* arcs,FXuint narcs);
	virtual void fillEllipse(FXint x,FXint y,FXint w,FXint h);
	virtual void fillComplexPolygon(const FXPoint* points,FXuint npoints);
	virtual void fillComplexPolygonRel(const FXPoint* points,FXuint npoints);
*/
	dc.setTile(tile, 0, 0);
	dc.setFillStyle(FILL_TILED);
	dc.fillRectangle(0, 0, w, h);
	dc.setFillStyle(FILL_SOLID);

	dc.setForeground(fg->getRGBA());
	dc.setBackground(bg->getRGBA());

	for (i = 0; i < 4; ++i) {
		dc.setForeground(i & 1 ? fg->getRGBA() : bg->getRGBA());
		dc.fillRectangle(20+i*20, 20+i*20, 100, 50);
	}
	for (i = 0; i < 4; ++i) {
		dc.setForeground(i & 1 ? bg->getRGBA() : fg->getRGBA());
		dc.fillRoundRectangle(20+i*20, 200+i*40, 100, 50, 20, 20);
	}
	
	dc.setForeground(fg->getRGBA());
	for (i = 0; i < 4; ++i) {
		arcs[i].x = 200;
		arcs[i].y = 20 + i*100;
		arcs[i].w = 100;
		arcs[i].h = 80;
	}
	arcs[0].a = 64 * 45; arcs[0].b = 64 * 270;
	arcs[1].a = 64 * 45; arcs[1].b = 64 * -270;
	arcs[2].a = 64 * -30; arcs[2].b = 64 * 270;
	arcs[3].a = 64 * -30; arcs[3].b = 64 * -270;
	dc.fillChords(arcs, 4);
	for (i = 0; i < 4; ++i)
		arcs[i].x = 400;
	dc.fillArcs(arcs, 4);
	
	for (i = 0; i < 4; ++i) {
		dc.fillEllipse(600, 20+i*100, 100-i*20, 20+i*20);
	}

	for (i = 0; i < 12; ++i)
		p[i] = points[i]*20 + FXPoint(800, 20);
	for (i = 0; i < 2; ++i) {
		dc.setFillRule(i==0?RULE_EVEN_ODD:RULE_WINDING);
		dc.fillComplexPolygon(p, 12);
		for (j = 0; j < 12; ++j)
			p[j] += FXPoint(0, 150);
	}
	
	
}

void ScribbleWindow::test_image(FXDCWindow & dc, int w, int h)
{
/*
	virtual void drawArea(const FXDrawable* source,FXint sx,FXint sy,
			FXint sw,FXint sh,FXint dx,FXint dy);
	virtual void drawArea(const FXDrawable* source,FXint sx,FXint sy,
			FXint sw,FXint sh,FXint dx,FXint dy,FXint dw,FXint dh);
	virtual void drawImage(const FXImage* image,FXint dx,FXint dy);
	virtual void drawBitmap(const FXBitmap* bitmap,FXint dx,FXint dy);
	virtual void drawIcon(const FXIcon* icon,FXint dx,FXint dy);
	virtual void drawIconShaded(const FXIcon* icon,FXint dx,FXint dy);
	virtual void drawIconSunken(const FXIcon* icon,FXint dx,FXint dy);
	virtual void drawText(FXint x,FXint y,const FXString& string);
	virtual void drawText(FXint x,FXint y,const FXchar* string,FXuint length);
	virtual void drawImageText(FXint x,FXint y,const FXString& string);
	virtual void drawImageText(FXint x,FXint y,const FXchar* string,FXuint length);
*/
	dc.setForeground(fg->getRGBA());
	dc.setBackground(bg->getRGBA());
	dc.drawImage(leaves_250h, 0, 0);
	dc.drawArea(leaves_250h, 0, 0, leaves_250h->getWidth(), leaves_250h->getHeight(), 
		0, 250, leaves_250h->getWidth(), leaves_250h->getHeight()/3);
	#ifdef CAR
	dc.drawArea(car_250h, 0, 0, car_250h->getWidth(), car_250h->getHeight(), 
		400, 0, car_250h->getWidth(), car_250h->getHeight());
	dc.drawArea(car_250h, 50, 50, car_250h->getWidth()-100, car_250h->getHeight()-100, 
		400, 250, car_250h->getWidth(), car_250h->getHeight());
	#else
	dc.drawIcon(wolf_icon, 400,0);
	dc.drawIconShaded(wolf_icon, 400,250);
	dc.drawIconSunken(wolf_icon, 600,0);
	#endif
	dc.drawArea(wolf_250h, 0, 0, wolf_250h->getWidth(), wolf_250h->getHeight(),
		1000, 0);
	//dc.drawArea(wolf_250h, 50, 50, wolf_250h->getWidth(), wolf_250h->getHeight(),
	//	1000, 250);
	dc.drawBitmap(wolf_bits, 1000, 250);
}


void ScribbleWindow::test_text(FXDCWindow & dc, int w, int h)
{
	dc.setForeground(fg->getRGBA());
	dc.setBackground(bg->getRGBA());
	dc.setFont(font_1);
	dc.setLineWidth(0);
	FXuint x = 30;
	FXuint y = 60;
	dc.drawLine(x-10, y, x, y);
	dc.drawText(x, y, "Hello World", 11);
	y = 100;
	dc.setFont(font_2);
	FXString s("This is an example of longer\ntext split over several\nlines using \u03a0\u03b1\u03bd\u8a9e");
	dc.drawText(x, y, s);
	y = 400;
	dc.setFont(font_3);
	dc.setTile(tile, 0, 0);
	dc.setFillStyle(FILL_TILED);
	s = "Fancy Stuff in \u03a0\u03b1\u03bd\u8a9e!";
	dc.drawImageText(x, y, s);
	x = 400;
	y = 30;
	dc.setFont(font_1);
	dc.setFillStyle(FILL_SOLID);
	dc.setForeground(FXRGB(0,0,180));
	s = "Pango (\u03a0\u03b1\u03bd\u8a9e) is a text layout\n"
	    "engine library which works with\n"
	    "HarfBuzz shaping engine for\n"
	    "displaying multi-language text.";
	dc.drawImageText(x, y, s);
}

void ScribbleWindow::test_clip(FXDCWindow & dc, int w, int h)
{
	dc.setClipMask(wolf_bits, 30, 30);
	dc.setForeground(fg->getRGBA());
	dc.setBackground(bg->getRGBA());
	dc.setFont(font_2);
	FXString s(
"Running, hunting, trying, surviving, and dying.\n"
"The wolves have no home anymore, thats why thery are crying.\n"
"For miles and miles howls are heard discussing their fates.\n"
"Humans don't care for this animal; wolves are forgotten then killed.\n"
"Humans don't know that a wolf has no hates.\n"
"That's why a wolf doesn't end up at Hell's gates.\n"
"Dark night color blue; wolves' images are relived on 14 Karat gold plates.\n"
"The round moon is the only light that they see.\n"
"Humans keep on destroying a beautiful home.\n"
"Wolves aren't dumb; they know they have no place to roam.\n"
"At night is the call of the wild.\n"
"Something pure on this Earth; like a new born child.\n"
"Cool air, water, and snow once demolished their fears.\n"
"There is nothing out there anymore; nothing but tears.\n"
"An innocent dream, turned black in the darkest clear...\n"
"For fire now dwells on the western land.\n"
"A newborn wolf pup sees his life has been altered.\n"
"No soft land to comfort your paws-only land of concrete and people.\n"
"Civilization to you.\n"
"No home no hope No life, but great sadness and fear.\n"
"Why can't the kingdom of salvation take them home.\n"
"-Anonymous");
	dc.drawImageText(30, 30, s);

}


void ScribbleWindow::doTest(FXCanvas * cnv, FXDCWindow & dc)
{
	// Test using only the FXDCWindow API
	int w = cnv->getWidth();
	int h = cnv->getHeight();
	dc.setForeground(backfill->getRGBA());
	dc.fillRectangle(0, 0, w, h);
	
	switch (tt) {
	case ID_STROKE:
		test_stroke(dc, w, h);
		break;
	case ID_FILL:
		test_fill(dc, w, h);
		break;
	case ID_IMAGE:
		test_image(dc, w, h);
		break;
	case ID_TEXT:
		test_text(dc, w, h);
		break;
	case ID_CLIP:
		test_clip(dc, w, h);
		break;
	}
}



long ScribbleWindow::onPaintW(FXObject*,FXSelector,void* ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	FXDCWindow dc(canvasw,ev);
	doTest(canvasw,dc);
	return 1;
}

long ScribbleWindow::onPaintC(FXObject*,FXSelector,void* ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	FXDCCairo dc(canvasc,ev);
	doTest(canvasc,dc);
	return 1;
}

long ScribbleWindow::onCmdBtn(FXObject*,FXSelector sel,void* ptr)
{
	tt = FXSELID(sel);
	{
		FXDCWindow dcw(canvasw);
		doTest(canvasw,dcw);
	}
	{
		FXDCCairo dcc(canvasc);
		doTest(canvasc,dcc);
	}
	return 1;
}



// Here we begin
int main(int argc,char *argv[])
{

	// Make application
	FXApp application("FXDCCairo","FoxTest");

	// Start app
	application.init(argc,argv);

	// Scribble window
	new ScribbleWindow(&application);

	// Create the application's windows
	application.create();

	// Run the application
	return application.run();
}




