/* === S Y N F I G ========================================================= */
/*!	\file trgt_openexr.cpp
**	\brief exr_trgt Target Module
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
**
** === N O T E S ===========================================================
**
** ========================================================================= */

/* === H E A D E R S ======================================================= */

#define SYNFIG_TARGET

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "trgt_openexr.h"
#include <synfig/synfig.h>
#include <ETL/stringf>
#include <cstdio>
#include <algorithm>
#include <functional>
#endif

/* === M A C R O S ========================================================= */

using namespace synfig;
using namespace std;
using namespace etl;

/* === G L O B A L S ======================================================= */

SYNFIG_TARGET_INIT(exr_trgt);
SYNFIG_TARGET_SET_NAME(exr_trgt,"openexr");
SYNFIG_TARGET_SET_EXT(exr_trgt,"exr");
SYNFIG_TARGET_SET_VERSION(exr_trgt,"1.0.4");
SYNFIG_TARGET_SET_CVS_ID(exr_trgt,"$Id$");

/* === M E T H O D S ======================================================= */

bool
exr_trgt::ready()
{
	return (bool)exr_file;
}

exr_trgt::exr_trgt(const char *Filename):
	multi_image(false),
	imagecount(0),
	filename(Filename),
	exr_file(0)
{
	buffer=0;
#ifndef USE_HALF_TYPE
	buffer_color=0;
#endif

	// OpenEXR uses linear gamma
	gamma().set_gamma(1.0);
}

exr_trgt::~exr_trgt()
{
	if(exr_file)
		delete exr_file;

	if(buffer) delete [] buffer;
#ifndef USE_HALF_TYPE
	if(buffer_color) delete [] buffer_color;
#endif
}

bool
exr_trgt::set_rend_desc(RendDesc *given_desc)
{
	//given_desc->set_pixel_format(PixelFormat((int)PF_RAW_COLOR));
	desc=*given_desc;
	imagecount=desc.get_frame_start();
	if(desc.get_frame_end()-desc.get_frame_start()>0)
		multi_image=true;
	else
		multi_image=false;
	return true;
}

bool
exr_trgt::start_frame(synfig::ProgressCallback *cb)
{
	int w=desc.get_w(),h=desc.get_h();

	String frame_name;

	if(exr_file)
		delete exr_file;
	if(multi_image)
	{
		String
			newfilename(filename),
			ext(find(filename.begin(),filename.end(),'.'),filename.end());
		newfilename.erase(find(newfilename.begin(),newfilename.end(),'.'),newfilename.end());

		newfilename+=etl::strprintf("%04d",imagecount)+ext;
		frame_name=newfilename;
		if(cb)cb->task(newfilename);
	}
	else
	{
		frame_name=filename;
		if(cb)cb->task(filename);
	}
	exr_file=new Imf::RgbaOutputFile(frame_name.c_str(),w,h,Imf::WRITE_RGBA,desc.get_pixel_aspect());
#ifndef USE_HALF_TYPE
	if(buffer_color) delete [] buffer_color;
	buffer_color=new Color[w];
#endif
	//if(buffer) delete [] buffer;
	//buffer=new Imf::Rgba[w];
	out_surface.set_wh(w,h);

	return true;
}

void
exr_trgt::end_frame()
{
	if(exr_file)
	{
		exr_file->setFrameBuffer(out_surface[0],1,desc.get_w());
		exr_file->writePixels(desc.get_h());

		delete exr_file;
	}

	exr_file=0;

	imagecount++;
}

Color *
exr_trgt::start_scanline(int i)
{
	scanline=i;
#ifndef USE_HALF_TYPE
	return reinterpret_cast<Color *>(buffer_color);
#else
	return reinterpret_cast<Color *>(out_surface[scanline]);
//	return reinterpret_cast<unsigned char *>(buffer);
#endif
}

bool
exr_trgt::end_scanline()
{
	if(!ready())
		return false;

#ifndef USE_HALF_TYPE
	int i;
	for(i=0;i<desc.get_w();i++)
	{
//		Imf::Rgba &rgba=buffer[i];
		Imf::Rgba &rgba=out_surface[scanline][i];
		Color &color=buffer_color[i];
		rgba.r=color.get_r();
		rgba.g=color.get_g();
		rgba.b=color.get_b();
		rgba.a=color.get_a();
	}
#endif

    //exr_file->setFrameBuffer(buffer,1,desc.get_w());
	//exr_file->writePixels(1);

	return true;
}
