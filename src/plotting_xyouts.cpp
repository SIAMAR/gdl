/***************************************************************************
                       plotting.cpp  -  GDL routines for plotting
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002-2011 by Marc Schellens et al.
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"
#include "plotting.hpp"
#include "math_utl.hpp"
#include <plplot/plplotP.h>

namespace lib {

  using namespace std;

  void xyouts( EnvT* e)
  {
    SizeT nParam = e->NParam(1);
    DDoubleGDL* yVal, *xVal;
    DStringGDL* strVal;
    SizeT xEl, yEl,strEl;
    if(nParam == 1) 
      {
	//string only... 
	  e->Throw("String only, not implemented");
      }
    else if(nParam == 3)
      {
	xVal = e->GetParAs< DDoubleGDL>(0);
	xEl = xVal->N_Elements();
	yVal = e->GetParAs< DDoubleGDL>(1);
	yEl = yVal->N_Elements();
	strVal=e->GetParAs<DStringGDL>(2);
	strEl=strVal->N_Elements();
      }
    else
      {
	e->Throw("Not enough parameters. Either 1 parameter or 3 "
		 "parameters valid.");
      }
    //ok...
    DLong minEl = (xEl < yEl)? xEl:yEl;
    minEl=(minEl < strEl)? minEl:strEl;
    
    DFloat xMarginL, xMarginR,yMarginB, yMarginT; 
    get_axis_margin("X", xMarginL, xMarginR);
    get_axis_margin("Y", yMarginB, yMarginT);
    
    DDouble xStart, xEnd, yStart, yEnd;
    bool xLog, yLog;
    DDouble minVal, maxVal;

    get_axis_crange("X", xStart, xEnd);
    get_axis_crange("Y", yStart, yEnd);
    get_axis_type("X", xLog);
    get_axis_type("Y", yLog);


    /*    DLong background = p_background;
    static int cix=e->KeywordIx("COLOR");
    BaseGDL* color_arr=e->GetKW(cix);
    DLongGDL* l_color_arr;
    if(color_arr != NULL)
      {
	l_color_arr=static_cast<DLongGDL*>
	  (color_arr->Convert2(LONG, BaseGDL::COPY));
	if(color_arr->N_Elements() < minEl && color_arr->N_Elements() > 1)
	  e->Throw( "Array "+e->GetParString(cix)+
		    " does not have enough elements for COLOR keyword.");
      }
    DLong color = p_color;

    if(color_arr != NULL)  
      if(color_arr->N_Elements() >= 1) 
	color=(*l_color_arr)[0];
    */
    GDLGStream* actStream = GetPlotStream( e); 
    
// pen thickness for axis
actStream->wid( 0);


    //start drawing
    //    actStream->Background( background);
    //    actStream->Color( color);
    //    gkw_background(e, actStream);
    gkw_color(e, actStream);

    PLFLT xMR, xML, yMB, yMT;
    CheckMargin( e, actStream,
		 xMarginL, 
		 xMarginR, 
		 yMarginB, 
		 yMarginT,
		 xMR, xML, yMB, yMT);

    bool mapSet=false;
#ifdef USE_LIBPROJ4
    // Map Stuff (xtype = 3)
    LPTYPE idata;
    XYTYPE odata;

    get_mapset(mapSet);

    if ( mapSet) {
      ref = map_init();
      if ( ref == NULL) {
	e->Throw( "Projection initialization failed.");
      }
    }
#endif

    DDouble *sx, *sy;
    DFloat *wx, *wy;
    GetSFromPlotStructs(&sx, &sy);
    GetWFromPlotStructs(&wx, &wy);

    int toto=0;

    if(e->KeywordSet("DEVICE")) {
      PLFLT xpix, ypix;
      PLINT xleng, yleng, xoff, yoff;
      actStream->gpage(xpix, ypix,xleng, yleng, xoff, yoff);
      xStart=0; xEnd=xleng;
      yStart=0; yEnd=yleng;
      xLog = false; yLog = false;
      actStream->NoSub();
      actStream->vpor(0, 1, 0, 1);
   } else if(e->KeywordSet("NORMAL")) {
      xStart = 0;
      xEnd   = 1;
      yStart = 0;
      yEnd   = 1;
      actStream->NoSub();
      actStream->vpor(0, 1, 0, 1);
      xLog = false; yLog = false;
    } else {
      toto=1;
      actStream->NoSub();
      if (xLog || yLog) actStream->vpor(wx[0], wx[1], wy[0], wy[1]);
      else actStream->vpor(0, 1, 0, 1); // (to be merged with the condition on DataCoordLimits...)
    }

    // Determine data coordinate limits
    // These are computed from window and scaling axis system
    // variables because map routines change these directly.

    //    if (e->KeywordSet("NORMAL") || e->KeywordSet("DATA")) {
    if (e->KeywordSet("DATA") || (toto == 1)) {
      DataCoordLimits(sx, sy, wx, wy, &xStart, &xEnd, &yStart, &yEnd, (xLog || yLog));
    }

    minVal=yStart; maxVal=yEnd;

    //CLIPPING
    DLong noclip=1;
    e->AssureLongScalarKWIfPresent( "NOCLIP", noclip);
    if(noclip == 0)
      {
	static int clippingix = e->KeywordIx( "CLIP"); 
	DDoubleGDL* clippingD = e->IfDefGetKWAs<DDoubleGDL>( clippingix);
	if( clippingD != NULL)
	    Clipping( clippingD, xStart, xEnd, minVal, maxVal);
      }

    if( yLog)
      {
	if( yStart <= 0.0) yStart = 0.0; else yStart = log10( yStart);
	if( yEnd   <= 0.0) return; else yEnd = log10( yEnd);
      }

    // SA: following a patch from Joanna (3029409) TODO: this is repeated in PLOTS POLYFILL and XYOUTS
    if ( xEnd - xStart == 0 || yEnd - yStart == 0 || isnan(xStart) || isnan(yStart) ) {
      actStream->wind( 0, 1, 0, 1 ); 
    } else {
      actStream->wind( xStart, xEnd, yStart, yEnd);
    }

    PLFLT x,y;
    string out;

    //orientation
    static int oix=e->KeywordIx("ORIENTATION");
    BaseGDL* orient=e->GetKW(oix);
    DDoubleGDL* d_orient;
    PLFLT p_orient, p_orient_x, p_orient_y;
    p_orient=0.0;
    p_orient_x=1.0;
    p_orient_y=0.0;

    if(orient != NULL)
      {
	d_orient=static_cast<DDoubleGDL*>
	  (orient->Convert2(DOUBLE, BaseGDL::COPY));
	if(orient->N_Elements() < minEl && orient->N_Elements() > 1)
	  e->Throw( "Array "+e->GetParString(oix)+
		    " does not have enough elements for ORIENTATION keyword.");
	p_orient=(*d_orient)[0];
	  while(p_orient < 0) p_orient+=360.0;
	  while(p_orient > 360.0) p_orient-=360.0;
      }

    p_orient_x=1.0*cos(p_orient*0.0174533);
    p_orient_y=1.0*sin(p_orient*0.0174533);

    //ALIGNMENT
    DDouble alignment = 0.0;
    e->AssureDoubleScalarKWIfPresent( "ALIGNMENT", alignment);

    //CHARSIZE
    DFloat charsize;
    gkw_charsize(e, actStream, charsize);


    // WIDTH keyword
    static int widthIx = e->KeywordIx( "WIDTH");
    bool kwWidth = e->KeywordPresent(widthIx);
    PLFLT width = 0.;

    // SA: plplot uses a "reference point" that "lies along a line passing 
    //     through the string at half the height of a capital letter"
    //     getting character height so it can be later used to move the 
    //     "reference point" half character height lower (tracker item no. 2982623)
    PLFLT charheight;
    {
      PLFLT nullf, htmm;
      plgchr(&nullf, &htmm); // height of a letter in millimetres
      PLINT htpc = plP_mmpcy(htmm); // height of a letter in physical coordinates
      PLINT nulli, p_iymin, p_iymax;
      plP_gphy(&nulli, &nulli, &p_iymin, &p_iymax); // physical device limits in physical coordinates
      PLFLT wy, wy0;
      plcalc_world(0., htpc / double(p_iymax - p_iymin), &nullf, &wy, &nulli); // wy = height of a letter in world coordinates
      plcalc_world(0., 0., &nullf, &wy0, &nulli); // wy = height of a letter in world coordinates
      charheight = wy - wy0;
    }

    if(minEl == 1)
      {
	x=static_cast<PLFLT>((*xVal)[0]);
	y=static_cast<PLFLT>((*yVal)[0]);

	if( yLog) if( y <= 0.0) goto skip; else y = log10( y);
	if( xLog) if( x <= 0.0) goto skip; else x = log10( x);

#ifdef USE_LIBPROJ4
	if (mapSet && !e->KeywordSet("NORMAL")) {
	  idata.lam = x * DEG_TO_RAD;
	  idata.phi = y * DEG_TO_RAD;
	  odata = PJ_FWD(idata, ref);
	  x = odata.x;
	  y = odata.y;
	}
#endif

        y += .5 * charheight;

	out=(*strVal)[0];
	actStream->ptex(x,y,p_orient_x, p_orient_y,alignment,out.c_str());
        if (kwWidth) width = plstrl(out.c_str());
      }
    else
      {
	for(int i=0; i<minEl;++i)
	  {
	    x=static_cast<PLFLT>((*xVal)[i]);
	    y=static_cast<PLFLT>((*yVal)[i]);

	    if( yLog) if( y <= 0.0) continue; else y = log10( y);
	    if( xLog) if( x <= 0.0) continue; else x = log10( x);

#ifdef USE_LIBPROJ4
	    if (mapSet && !e->KeywordSet("NORMAL")) {
	      idata.lam = x * DEG_TO_RAD;
	      idata.phi = y * DEG_TO_RAD;
	      odata = PJ_FWD(idata, ref);
	      x = odata.x;
	      y = odata.y;
	      if (!isfinite(x) || !isfinite(y)) continue;
	    }
#endif
	    if(orient != NULL)
	      if(orient->N_Elements() > 1) 
		{
		  p_orient=(*d_orient)[i];
		  while(p_orient < 0) p_orient+=360.0;
		  while(p_orient > 360.0) p_orient-=360.0;
		  p_orient_x=1.0*cos(p_orient*0.0174533);
		  p_orient_y=1.0*sin(p_orient*0.0174533);
		}

	    /*	    if(color_arr != NULL)  
	      if(color_arr->N_Elements() > 1)
		actStream->Color((*l_color_arr)[i]);
	    */
	    out=(*strVal)[i];
            y += .5 * charheight;
	    actStream->ptex(x,y,p_orient_x, p_orient_y,alignment,out.c_str());
            if (kwWidth) width = max(plstrl(out.c_str()), width);
	  }
      }
    
  skip:
    actStream->flush();
    if (kwWidth)
    {
      // SA: we should return value of width in "normalized coordinate units"
      //     width contains output from plstrl() expressed in millimetres
      //     plP_mmpcx() converts it into physical coordinates
      //     plP_gphy() gives "physical device limits in physical coordinates"
      PLINT p_ixmin, p_ixmax, p_iymin, p_iymax;
      plP_gphy(&p_ixmin, &p_ixmax, &p_iymin, &p_iymax);
      e->SetKW(widthIx, new DFloatGDL(plP_mmpcx(width)/double(p_ixmax - p_ixmin)));
    }
  }

} // namespace