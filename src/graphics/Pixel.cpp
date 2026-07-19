#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "bzlib.h"
#include "Pixel.h"
#include "resampler/Resampler.h"

char * generate_gradient(pixel * colours, float * points, int pointcount, int size)
{
	char * newdata = (char*)malloc(size * 3);
	memset(newdata, 0, size*3);
	//Sort the Colours and Points
	for (int i = (pointcount - 1); i > 0; i--)
	{
		for (int j = 1; j <= i; j++)
		{
			if (points[j-1] > points[j])
			{
				float temp = points[j-1];
				points[j-1] = points[j];
				points[j] = temp;

				pixel ptemp = colours[j-1];
				colours[j-1] = colours[j];
				colours[j] = ptemp;
			}
		}
	}
	int i = 0;
	int j = 1;
	float poss = points[i];
	float pose = points[j];
	for (int cp = 0; cp < size; cp++)
	{
		float cpos = (float)cp / (float)size, ccpos, cccpos;
		if (cpos > pose && j+1 < pointcount)
		{
			poss = points[++i];
			pose = points[++j];
		}
		ccpos = cpos - poss;
		cccpos = ccpos / (pose - poss);
		if (cccpos > 1.0f)
			cccpos = 1.0f;
		newdata[(cp*3)] = (char)(PIXR(colours[i])*(1.0f-cccpos) + PIXR(colours[j])*(cccpos));
		newdata[(cp*3)+1] = (char)(PIXG(colours[i])*(1.0f-cccpos) + PIXG(colours[j])*(cccpos));
		newdata[(cp*3)+2] = (char)(PIXB(colours[i])*(1.0f-cccpos) + PIXB(colours[j])*(cccpos));
	}
	return newdata;
}

void * ptif_pack(pixel *src, int w, int h, int *result_size)
{
	int datalen = (w*h)*3;
	unsigned char *red_chan = (unsigned char*)calloc(1, w*h);
	unsigned char *green_chan = (unsigned char*)calloc(1, w*h);
	unsigned char *blue_chan = (unsigned char*)calloc(1, w*h);
	unsigned char *data = (unsigned char*)malloc(((w*h)*3)+8);
	unsigned char *result = (unsigned char*)malloc(((w*h)*3)+8);

	for (int cx = 0; cx<w; cx++)
	{
		for (int cy = 0; cy<h; cy++)
		{
			red_chan[w*(cy)+(cx)] = PIXR(src[w*(cy)+(cx)]);
			green_chan[w*(cy)+(cx)] = PIXG(src[w*(cy)+(cx)]);
			blue_chan[w*(cy)+(cx)] = PIXB(src[w*(cy)+(cx)]);
		}
	}

	memcpy(data, red_chan, w*h);
	memcpy(data+(w*h), green_chan, w*h);
	memcpy(data+((w*h)*2), blue_chan, w*h);
	free(red_chan);
	free(green_chan);
	free(blue_chan);

	result[0] = 'P';
	result[1] = 'T';
	result[2] = 'i';
	result[3] = 1;
	result[4] = w;
	result[5] = w>>8;
	result[6] = h;
	result[7] = h>>8;

	int i = (w*h)*3-8;
	if (BZ2_bzBuffToBuffCompress((char *)(result+8), (unsigned *)&i, (char *)data, datalen, 9, 0, 0) != BZ_OK)
	{
		free(data);
		free(result);
		return NULL;
	}

	*result_size = i+8;
	free(data);
	return result;
}

pixel * ptif_unpack(void *datain, int size, int *w, int *h)
{
	if (size<16)
	{
		printf("Image empty\n");
		return NULL;
	}
	unsigned char *data = (unsigned char*)datain;
	if (!(data[0]=='P' && data[1]=='T' && data[2]=='i'))
	{
		printf("Image header invalid\n");
		return NULL;
	}

	int width = data[4]|(data[5]<<8);
	int height = data[6]|(data[7]<<8);
	int i = (width*height)*3;
	unsigned char *undata = (unsigned char*)calloc(1, (width*height)*3);
	unsigned char *red_chan = (unsigned char*)calloc(1, width*height);
	unsigned char *green_chan = (unsigned char*)calloc(1, width*height);
	unsigned char *blue_chan = (unsigned char*)calloc(1, width*height);
	pixel *result = (pixel*)calloc(width*height, PIXELSIZE);

	int resCode = BZ2_bzBuffToBuffDecompress((char *)undata, (unsigned *)&i, (char *)(data+8), size-8, 0, 0);
	if (resCode)
	{
		printf("Decompression failure, %d\n", resCode);
		free(red_chan);
		free(green_chan);
		free(blue_chan);
		free(undata);
		free(result);
		return NULL;
	}
	if (i != (width*height)*3)
	{
		printf("Result buffer size mismatch, %d != %d\n", i, (width*height)*3);
		free(red_chan);
		free(green_chan);
		free(blue_chan);
		free(undata);
		free(result);
		return NULL;
	}
	memcpy(red_chan, undata, width*height);
	memcpy(green_chan, undata+(width*height), width*height);
	memcpy(blue_chan, undata+((width*height)*2), width*height);

	for (int cx = 0; cx < width; cx++)
	{
		for (int cy = 0; cy < height; cy++)
		{
			result[width*(cy)+(cx)] = PIXRGB(red_chan[width*(cy)+(cx)], green_chan[width*(cy)+(cx)], blue_chan[width*(cy)+(cx)]);
		}
	}

	*w = width;
	*h = height;
	free(red_chan);
	free(green_chan);
	free(blue_chan);
	free(undata);
	return result;
}

pixel * resample_img_nn(pixel * src, int sw, int sh, int rw, int rh)
{
	pixel *q = (pixel*)malloc(rw*rh*PIXELSIZE);
	for (int y = 0; y < rh; y++)
		for (int x = 0; x < rw; x++)
			q[rw*y+x] = src[sw*(y*sh/rh)+(x*sw/rw)];
	return q;
}

pixel * resample_img_high(pixel *src, int sw, int sh, int rw, int rh)
{
	unsigned char * source = (unsigned char*)src;
	int sourceWidth = sw, sourceHeight = sh;
	int resultWidth = rw, resultHeight = rh;
	int sourcePitch = sourceWidth*PIXELSIZE, resultPitch = resultWidth*PIXELSIZE;
	// Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
	const float filter_scale = 0.75f;
	const char* pFilter = "lanczos12";


	Resampler * resamplers[PIXELCHANNELS];
	float * samples[PIXELCHANNELS];

	//Resampler for each colour channel
	if (sourceWidth <= 0 || sourceHeight <= 0 || resultWidth <= 0 || resultHeight <= 0)
		return NULL;
	resamplers[0] = new Resampler(sourceWidth, sourceHeight, resultWidth, resultHeight, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, NULL, NULL, filter_scale, filter_scale);
	samples[0] = new float[sourceWidth];
	for (int i = 1; i < PIXELCHANNELS; i++)
	{
		resamplers[i] = new Resampler(sourceWidth, sourceHeight, resultWidth, resultHeight, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(), resamplers[0]->get_clist_y(), filter_scale, filter_scale);
		samples[i] = new float[sourceWidth];
	}

	unsigned char * resultImage = new unsigned char[resultHeight * resultPitch];
	std::fill(resultImage, resultImage + (resultHeight*resultPitch), 0);

	//Resample time
	int resultY = 0;
	for (int sourceY = 0; sourceY < sourceHeight; sourceY++)
	{
		unsigned char * sourcePixel = &source[sourceY * sourcePitch];

		//Move pixel components into channel samples
		for (int c = 0; c < PIXELCHANNELS; c++)
		{
			for (int x = 0; x < sourceWidth; x++)
			{
				samples[c][x] = sourcePixel[(x*PIXELSIZE)+c] * (1.0f/255.0f);
			}
		}

		//Put channel sample data into resampler
		for (int c = 0; c < PIXELCHANNELS; c++)
		{
			if (!resamplers[c]->put_line(&samples[c][0]))
			{
				printf("Out of memory!\n");
				return NULL;
			}
		}

		//Perform resample and Copy components from resampler result samples to image buffer
		for ( ; ; )
		{
			int comp_index;
			for (comp_index = 0; comp_index < PIXELCHANNELS; comp_index++)
			{
				const float* resultSamples = resamplers[comp_index]->get_line();
				if (!resultSamples)
					break;

				unsigned char * resultPixel = &resultImage[(resultY * resultPitch) + comp_index];

				for (int x = 0; x < resultWidth; x++)
				{
					int c = (int)(255.0f * resultSamples[x] + .5f);
					if (c < 0) c = 0; else if (c > 255) c = 255;
					*resultPixel = (unsigned char)c;
					resultPixel += PIXELSIZE;
				}
			}
			if (comp_index < PIXELCHANNELS)
				break;

			resultY++;
		}
	}

	//Clean up
	for(int i = 0; i < PIXELCHANNELS; i++)
	{
		delete resamplers[i];
		delete[] samples[i];
	}

	return (pixel*)resultImage;
}

pixel * resample_img_orig(pixel *src, int sw, int sh, int rw, int rh)
{
	bool stairstep = false;
	if(rw < sw || rh < sh)
	{
		float fx = (float)(((float)sw)/((float)rw));
		float fy = (float)(((float)sh)/((float)rh));

		int fxint, fyint;
		double fxintp_t, fyintp_t;

		float fxf = modf(fx, &fxintp_t), fyf = modf(fy, &fyintp_t);
		fxint = fxintp_t;
		fyint = fyintp_t;

		if(((fxint & (fxint-1)) == 0 && fxf < 0.1f) || ((fyint & (fyint-1)) == 0 && fyf < 0.1f))
			stairstep = true;
	}

	int y, x, fxceil, fyceil;
	//int i,j,x,y,w,h,r,g,b,c;
	pixel *q = NULL;
	if(rw == sw && rh == sh){
		//Don't resample
		q = new pixel[rw*rh];
		std::copy(src, src+(rw*rh), q);
	} else if(!stairstep) {
		float fx, fy, fyc, fxc;
		double intp;
		pixel tr, tl, br, bl;
		q = new pixel[rw*rh];
		//Bilinear interpolation for upscaling
		for (y=0; y<rh; y++)
			for (x=0; x<rw; x++)
			{
				fx = ((float)x)*((float)sw)/((float)rw);
				fy = ((float)y)*((float)sh)/((float)rh);
				fxc = modf(fx, &intp);
				fyc = modf(fy, &intp);
				fxceil = (int)ceil(fx);
				fyceil = (int)ceil(fy);
				if (fxceil>=sw) fxceil = sw-1;
				if (fyceil>=sh) fyceil = sh-1;
				tr = src[sw*(int)floor(fy)+fxceil];
				tl = src[sw*(int)floor(fy)+(int)floor(fx)];
				br = src[sw*fyceil+fxceil];
				bl = src[sw*fyceil+(int)floor(fx)];
				q[rw*y+x] = PIXRGB(
					(int)(((((float)PIXR(tl))*(1.0f-fxc))+(((float)PIXR(tr))*(fxc)))*(1.0f-fyc) + ((((float)PIXR(bl))*(1.0f-fxc))+(((float)PIXR(br))*(fxc)))*(fyc)),
					(int)(((((float)PIXG(tl))*(1.0f-fxc))+(((float)PIXG(tr))*(fxc)))*(1.0f-fyc) + ((((float)PIXG(bl))*(1.0f-fxc))+(((float)PIXG(br))*(fxc)))*(fyc)),
					(int)(((((float)PIXB(tl))*(1.0f-fxc))+(((float)PIXB(tr))*(fxc)))*(1.0f-fyc) + ((((float)PIXB(bl))*(1.0f-fxc))+(((float)PIXB(br))*(fxc)))*(fyc))
					);
			}
	} else {
		//Stairstepping
		float fx, fy, fyc, fxc;
		double intp;
		pixel tr, tl, br, bl;
		int rrw = rw, rrh = rh;
		pixel * oq;
		oq = new pixel[sw*sh];
		std::copy(src, src+(sw*sh), oq);
		rw = sw;
		rh = sh;
		while(rrw != rw && rrh != rh){
			if(rw > rrw)
				rw *= 0.7;
			if(rh > rrh)
				rh *= 0.7;
			if(rw <= rrw)
				rw = rrw;
			if(rh <= rrh)
				rh = rrh;
			q = new pixel[rw*rh];
			//Bilinear interpolation
			for (y=0; y<rh; y++)
				for (x=0; x<rw; x++)
				{
					fx = ((float)x)*((float)sw)/((float)rw);
					fy = ((float)y)*((float)sh)/((float)rh);
					fxc = modf(fx, &intp);
					fyc = modf(fy, &intp);
					fxceil = (int)ceil(fx);
					fyceil = (int)ceil(fy);
					if (fxceil>=sw) fxceil = sw-1;
					if (fyceil>=sh) fyceil = sh-1;
					tr = oq[sw*(int)floor(fy)+fxceil];
					tl = oq[sw*(int)floor(fy)+(int)floor(fx)];
					br = oq[sw*fyceil+fxceil];
					bl = oq[sw*fyceil+(int)floor(fx)];
					q[rw*y+x] = PIXRGB(
						(int)(((((float)PIXR(tl))*(1.0f-fxc))+(((float)PIXR(tr))*(fxc)))*(1.0f-fyc) + ((((float)PIXR(bl))*(1.0f-fxc))+(((float)PIXR(br))*(fxc)))*(fyc)),
						(int)(((((float)PIXG(tl))*(1.0f-fxc))+(((float)PIXG(tr))*(fxc)))*(1.0f-fyc) + ((((float)PIXG(bl))*(1.0f-fxc))+(((float)PIXG(br))*(fxc)))*(fyc)),
						(int)(((((float)PIXB(tl))*(1.0f-fxc))+(((float)PIXB(tr))*(fxc)))*(1.0f-fyc) + ((((float)PIXB(bl))*(1.0f-fxc))+(((float)PIXB(br))*(fxc)))*(fyc))
						);
				}
			delete[] oq;
			oq = q;
			sw = rw;
			sh = rh;
		}
	}
	return q;
}

pixel * resample_img(pixel *src, int sw, int sh, int rw, int rh, bool highQualityResample)
{
	if (highQualityResample)
		return resample_img_high(src, sw, sh, rw, rh);
	else
		return resample_img_orig(src, sw, sh, rw, rh);
}

pixel * rescale_img(pixel *src, int sw, int sh, int *qw, int *qh, int f)
{
	int w = (sw+f-1)/f;
	int h = (sh+f-1)/f;
	pixel *q = (pixel*)malloc(w*h*PIXELSIZE);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
		{
			int r = 0, g = 0, b = 0, c = 0;
			for (int j = 0; j < f; j++)
				for (int i = 0; i < f; i++)
					if (x*f+i<sw && y*f+j<sh)
					{
						pixel p = src[(y*f+j)*sw + (x*f+i)];
						if (p)
						{
							r += PIXR(p);
							g += PIXG(p);
							b += PIXB(p);
							c ++;
						}
					}
			if (c > 1)
			{
				r = (r+c/2)/c;
				g = (g+c/2)/c;
				b = (b+c/2)/c;
			}
			q[y*w+x] = PIXRGB(r, g, b);
		}
	*qw = w;
	*qh = h;
	return q;
}
