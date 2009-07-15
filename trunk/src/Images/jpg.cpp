#include "jpg.h"
//#include <malloc.h>
//#include <string.h>
//#include <math.h>
#include <string.h>
#include "..\\malloc.h"
#define SOI 0xD8
#define EOI 0xD9

#define SOF0 0xC0
#define SOS  0xDA
#define APP0 0xE0
#define COM 0xFE
#define DNL 0xDC
#define DRI 0xDD
#define DQT 0xDB
#define DHT 0xC4


#define DAC 0xCC

struct HuffmanTable {
	int numCodes[16];
	unsigned short minCode[16];
	unsigned short maxCode[16];
	unsigned char values[256];
};


struct Section {
	float v[64];
};

struct Component {
	int id, vertFactor, horizFactor, qt;
};

#define PI 3.141592654

struct ComponentScan {
	int id, AC, DC;
};

int reorder[] = { 0, 1, 5, 6,14,15,27,28,
				  2, 4, 7,13,16,26,29,42,
				  3, 8,12,17,25,30,41,43,
				  9,11,18,24,31,40,44,53,
				 10,19,23,32,39,45,52,54,
				 20,22,33,38,46,51,55,60,
				 21,34,37,47,50,56,59,61,
				 35,36,48,49,57,58,62,63};

inline void __fastcall MoveJunk(int len, int &offset, int &pos, unsigned int &ndata, unsigned char *data, int bytes) {
	offset += bytes;
	ndata <<= bytes;
	while (offset>=8) {
		if (pos>=len) {
			offset-=8;
			pos++;
		}
		else if (data[pos]==0xFF) {
			if (pos+1>=len) {
				// Help!
				offset = 0;
				return;
			}
			else if (data[pos+1]==0) {
				offset-=8;
				ndata += (data[pos]<<offset);
				pos+=2;
			}
			else if (data[pos+1] == 0xd0 || data[pos+1] == 0xd1 ||
					 data[pos+1] == 0xd2 || data[pos+1] == 0xd3 ||
					 data[pos+1] == 0xd4 || data[pos+1] == 0xd5 ||
					 data[pos+1] == 0xd6 || data[pos+1] == 0xd7 ||
					 data[pos+1] == 0x01 || data[pos+1] == EOI) pos+=2;
			else {
				pos+=2;
				if (pos+1>=len) {
					// help!
					continue;
				}
				int len2 = Flip(&data[pos]);
				pos+=len2;
			}
		}
		else {
			offset-=8;
			ndata += (data[pos++]<<offset);
		}
	}
}



template <class PixelFormat>
void ResampleBilinear(GenericImage<PixelFormat> *in, GenericImage<PixelFormat> *out) {
	float scalex = (in->width)/(float)(out->width);
	float scaley = (in->height)/(float)(out->height);
	float xoffset = 0.5f - 0.5f * scalex;
	float yoffset = 0.5f - 0.5f * scaley;
	int u, v1;
	int x, y;
	float rx, ry;
	float yWeight[2];
	float *xWeights = (float*) alloca(sizeof(float)*out->width*2 + sizeof(int)*out->width);
	int  *xvals = (int*) &xWeights[out->width*2];

	for (x=0; x<(int)out->width; x++) {
		rx = x*scalex-xoffset;
		if (rx<0) rx = 0;
		if (rx>in->height-1) {
			u = in->height-1;
			rx = (float) u;
			xvals[x] = u;
			xWeights[2*x]   = 1;
			xWeights[2*x+1] = 0;
		}
		else {
			u = (int) rx;
			xvals[x] = u;
			xWeights[2*x]   = (u+1)-rx;
			xWeights[2*x+1] = 1-xWeights[2*x];
		}
	}
	PixelFormat *p = out->pixels;

	for (y=0; y<(int)out->height; y++) {
		ry = y*scaley-yoffset;
		if (ry<0) {
			v1=0;
			yWeight[0]=1;
			yWeight[1] = 0;
		}
		else if (ry>in->height-1) {
			//ry = (float)(in->height-1);
			v1 = (in->height-1)*in->width;
			yWeight[0]=1;
		}
		else {
			v1 = (int)ry;
			yWeight[0] = (v1+1)-ry;
			yWeight[1] = 1-yWeight[0];
			v1 *= in->width;
		}
		PixelFormat *pixels = &in->pixels[v1];
		PixelFormat *pixels2 = &pixels[in->width];
		if (yWeight[0]==1) {
			for (x=0; x<(int)out->width; x++) {
				u = xvals[x];
				if (xWeights[2*x+1]==0) {
					p++[0] = pixels[u];
				}
				else {
					p++[0] = 
						xWeights[2*x] *   pixels[u] +
						xWeights[2*x+1] * pixels[u+1];
				}
			}
		}
		else {
			for (x=0; x<(int)out->width; x++) {
				u = xvals[x];
				if (xWeights[2*x+1]==0) {
					p++[0] = 
						xWeights[2*x] *   (yWeight[0]*pixels[u] +
										   yWeight[1]*pixels2[u]);
				}
				else {
					p++[0] = 
						xWeights[2*x] *   (yWeight[0]*pixels[u] +
										   yWeight[1]*pixels2[u]) +
						xWeights[2*x+1] * (yWeight[0]*pixels[u+1] +
										   yWeight[1]*pixels2[u+1]);
				}
				/*
				PixelFormat sum = in->pixels[u+in->width*v];
				if (rx-u>0.005f) {
					sum*= (u+1)-rx;
					sum+= in->pixels[u+1+in->width*v] * (rx-u);
					if (ry-v > 0.0005f) {
						sum*= (v+1)-ry;
						sum+= (ry-v) * (in->pixels[u  +in->width*(v+1)] * ((u+1)-rx) +
										in->pixels[u+1+in->width*(v+1)] * (rx-u));
					}
				}
				else if (ry-v > 0.0005f) {
					sum*= (v+1)-ry;
					sum+= in->pixels[u+in->width*(v+1)] * (ry-v);
				}//*/
				//p++[0] = sum;
				/*
				rx = x*scalex-xoffset;
				if (rx<0) rx = 0;
				if (rx>in->height-1) rx = (float) (in->height-1);
				u = (int) rx;

				PixelFormat sum = yWeight[0]*in->pixels[u+v1] +
					yWeight[1]*in->pixels[u+v2];
				if (rx-u>0.005f) {
					sum*= (u+1)-rx;
					sum+= (rx-u)*(yWeight[0]*in->pixels[u+1+v1] +
								  yWeight[1]*in->pixels[u+1+v2]);
				}*/
					

			}
		}
	}
	//free(xWeights);
}


inline void __fastcall Decode(int len, int &offset, int &pos, unsigned int &ndata, unsigned char *data, float &start, float *out,
					HuffmanTable &htAC, HuffmanTable &htDC) {

	int i;
	unsigned short s = ndata>>16;
	unsigned int p=0;

	for (i=0; i<16; i++) {
		if (htDC.maxCode[i] > s && htDC.minCode[i] <= s) {
			p+=((s - htDC.minCode[i])>>(15-i));
			break;
		}
		p += htDC.numCodes[i];
	}

	int len2 = htDC.values[p];

	MoveJunk(len, offset, pos, ndata, data, i+1);

	int negMask[16] = {0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

	int val;
	if (len2!=0) {
		val = ndata>>(32-len2);

		if (0 == (val & (1<<(len2-1)))) {
			val = -(val^negMask[len2]);
		}
		out[0] = start+(float)val;
		start = out[0];

		MoveJunk(len, offset, pos, ndata, data, len2);
	}
	else {
		out[0] = start;
	}

	memset(out+1, 0, sizeof(float)*63);
	int vpos = 1;
	while (vpos < 64) {
		p=0;
		s = ndata>>16;
		for (i=0; i<16; i++) {
			if (htAC.maxCode[i] > s && htAC.minCode[i] <= s) {
				p+=((s - htAC.minCode[i])>>(15-i));
				break;
			}
			p += htAC.numCodes[i];
		}
		p = htAC.values[p];

		MoveJunk(len, offset, pos, ndata, data, i+1);

		int jump = p>>4;
		len2 = p & 0xF;
		vpos+=jump;

		if (len2==0) {
			if (jump==0) break;
			vpos++;
			continue;
		}
		else {
			val = ndata>>(32-len2);
		}

		if (0 == (val & (1<<(len2-1)))) {
			val = -(val^negMask[len2]);
		}
		out[vpos++] = (float)val;

		MoveJunk(len, offset, pos, ndata, data, len2);
	}
}

inline void ReorderAndIDCT(float *out, float *qt) {
	int *order = reorder;

	float yvals[8][8];
	float tmp10, tmp11, tmp13, z10, z11, z12, z13, z5;
	float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	float *working = yvals[0];
	while (working<yvals[1]) {
		if (!out[order[8 ]] && !out[order[16]] &&
			!out[order[24]] && !out[order[32]] &&
			!out[order[40]] && !out[order[48]] &&
			!out[order[56]]) {

			working[ 0] = out[order++[0]] * qt++[0];
			working[ 8] = working[0];
			working[16] = working[0];
			working[24] = working[0];
			working[32] = working[0];
			working[40] = working[0];
			working[48] = working[0];
			working[56] = working[0];
		}
		else {
			
			tmp0 = out[order[ 0]] * qt[0];
			tmp1 = out[order[16]] * qt[16];
			tmp2 = out[order[32]] * qt[32];
			tmp3 = out[order[48]] * qt[48];

			tmp10 = tmp0 + tmp2;// phase 3
			tmp11 = tmp0 - tmp2;

			tmp13 = tmp1 + tmp3;// phases 5-3
			z5 = (tmp1 - tmp3) * 1.414213562f - tmp13; // 2*c4

			tmp0 = tmp10 + tmp13;
			tmp3 = tmp10 - tmp13;
			tmp1 = tmp11 + z5;
			tmp2 = tmp11 - z5;

			tmp4 = out[order[8 ]] * qt[8 ];
			tmp5 = out[order[24]] * qt[24];
			tmp6 = out[order[40]] * qt[40];
			tmp7 = out[order++[56]] * qt++[56];

			z13 = tmp6 + tmp5;
			z10 = tmp6 - tmp5;
			z11 = tmp4 + tmp7;
			z12 = tmp4 - tmp7;

			tmp7 = z11 + z13;
			tmp11= (z11 - z13) * 1.414213562f;

			z5 = (z10 + z12) * 1.847759065f;
			tmp10 = 1.082392200f * z12 - z5;

			tmp6 = -2.613125930f * z10 + z5 - tmp7;
			tmp5 = tmp11 - tmp6;
			tmp4 = tmp10 + tmp5;

			working[ 0] = tmp0 + tmp7;
			working[ 8] = tmp1 + tmp6;
			working[16] = tmp2 + tmp5;
			working[24] = tmp3 - tmp4;
			working[32] = tmp3 + tmp4;
			working[40] = tmp2 - tmp5;
			working[48] = tmp1 - tmp6;
			working[56] = tmp0 - tmp7;

		}
		working++;
	}
	working = yvals[0];
	float *data = out;
	while (working<yvals[8]) { 
		tmp10 = working[0] + working[4];
		tmp11 = working[0] - working[4];
		tmp13 = working[2] + working[6];
		z5 = (working[2] - working[6]) * 1.414213562f-tmp13;

		tmp0 = tmp10+tmp13;
		tmp3 = tmp10-tmp13;
		tmp1 = tmp11+z5;
		tmp2 = tmp11-z5;

		z13 = working[5] + working[3];
		z10 = working[5] - working[3];
		z11 = working[1] + working[7];
		z12 = working[1] - working[7];
		tmp7 = z11+z13;
		tmp11 = (z11-z13) * 1.414213562f;
		z5 = (z10 + z12) * 1.847759065f;
		tmp10 = 1.082392200f * z12 - z5;

		tmp6 = -2.613125930f * z10 + z5 - tmp7;
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		data[0] = tmp0 + tmp7;
		data[1] = tmp1 + tmp6;
		data[2] = tmp2 + tmp5;
		data[3] = tmp3 - tmp4;
		data[4] = tmp3 + tmp4;
		data[5] = tmp2 - tmp5;
		data[6] = tmp1 - tmp6;
		data[7] = tmp0 - tmp7;

		working+=8;
		data+=8;
	}
}


int LoadJpeg(ScriptValue &sv, unsigned char *data, int len) {
	int u, v;

	int ready=0;
	HuffmanTable htDC[4];
	HuffmanTable htAC[4];
	if (len<18) return 0;
	if (data[0]!=0xFF || data[1]!=SOI) return 0;
	float qt[4][64];
	memset(qt, 0, sizeof(qt));
	data += 2;
	len-=2;
	int len2, pos=0;

	int RestartInterval=0;
	int i, j, k;
	int height=0, width=0;
	Component components[10];
	ComponentScan componentScans[10];
	int pos2;
	while (pos < len-1) {
		unsigned char c = data[pos++];
		if (c!=0xFF) continue;
		c = data[pos++];

		if (c==0xFF || c==0x01 ||
			c==0xd0 || c==0xd1 ||
			c==0xd2 || c==0xd3 ||
			c==0xd4 || c==0xd5 ||
			c==0xd6 || c==0xd7) {
			continue;
		}
		if (c==EOI) break;
		if (c==DAC) return 0;
		if (pos>=len-1) continue;

		len2 = Flip(&data[pos]);
		if (pos+len2 > len) {
			pos += len2;
			continue;
		}

		if (c == DQT) {
			pos2 = pos+2;
			while (pos2<pos+len2) {
				int num = data[pos2]&0xF;
				int bits = data[pos2]>>4;
				if (num>=4) return 0;
				pos2++;
				long qtTemp[64];
				memset(qtTemp,0,sizeof(qtTemp));
				for (j=0; j<64; j++) {
					for (k=0; k<=bits; k++) {
						if (pos2>len) continue;
						qtTemp[j] += (data[pos2++]<<((bits-k-1)<<8));
					}
				}
				for (j=0; j<64; j++) {
					const float scalefactor[8]={1.0f/2.828427125f, 1.387039845f/2.828427125f, 1.306562965f/2.828427125f, 1.175875602f/2.828427125f,
									   1.0f/2.828427125f, 0.785694958f/2.828427125f, 0.541196100f/2.828427125f, 0.275899379f/2.828427125f};

					qt[num][j] = qtTemp[reorder[j]] * scalefactor[j/8] * scalefactor[j%8];
				}
			}
			ready|=8;
		}
		else if (c == DHT) {
			pos2 = pos+2;
			while (pos2<pos+len2) {
				int num = data[pos2]&0xF;
				int type = (data[pos2]>>4)&1;
				if (data[pos2]>>5) return 0;
				pos2++;
				HuffmanTable *h;
				if (type) h = &htAC[num];
				else h = &htDC[num];
				memset(h->values, 0, sizeof(h->values));
				unsigned short min=0;
				unsigned short inc = 0x8000;
				for (j=0; j<16; j++) {
					if (pos2>len) return 0;
					h->minCode[j] = min;
					unsigned short codes = data[pos2++];
					h->maxCode[j] = min+(codes)*inc;
					min = h->maxCode[j];
					h->numCodes[j] = codes;
					inc>>=1;
				}
				//h->values[0]=0;
				min = 0;
				inc = 0x8000;
				i=0;
				for (j=0; j<16; j++) {
					while (h->maxCode[j]>min) {
						if (pos2>len) return 0;
						h->values[i++] = ((unsigned char*) data)[pos2++];
						min += inc;
					}
					inc>>=1;
				}
			}
			ready|=4;
		}
		else if (c == DRI) {
			if (len2!=4) continue;
			RestartInterval = Flip(&data[pos+2]);
		}
		else if (c == SOF0) {
			if (len2<11) return 0;
			pos2 = pos+2;
			int bytes = data[pos2++];
			//if (bytes!=8) return 0;
			height = Flip(&data[pos2]);
			width = Flip(&data[pos2+2]);
			pos2+=4;
			//Note that these are technically signed, and I'm too lazy to mess with it
			if (width>20000 || height > 20000) return 0;
			int numComponents = data[pos2++];
			if (len2<numComponents*3+8 || numComponents>4) return 0;
			for (i=0; i<numComponents; i++) {
				components[i].id = data[pos2++];
				if (components[i].id>=4 || components[i].id<1) return 0;
				components[i].vertFactor = data[pos2]&0xF;
				components[i].horizFactor = data[pos2++]>>4;
				if (components[i].horizFactor==0) return 0;
				if (components[i].vertFactor==0) return 0;
				components[i].qt = data[pos2++];
			}
			components[i].id=0;
			ready|=2;
		}
		else if (c == SOS) {
			if (len2<6) return 0;
			pos2 = pos+2;
			int numComponents = data[pos2++];
			if (len2<6+numComponents) return 0;
			for (i=0; i<numComponents; i++) {
				componentScans[i].id = data[pos2++];
				if (componentScans[i].id>=4 || componentScans[i].id<1) return 0;
				componentScans[i].AC = data[pos2]&0xF;
				componentScans[i].DC = data[pos2++]>>4;
			}
			componentScans[i].id=0;
			pos += len2;
			ready|=1;
			break;
		}
		else if (c == EOI) {
			return 0;
		}
		else if ((c&0xF0) == 0xC0){
			return 0;
		}
		//else {
			//pos=pos;
		//}
		pos += len2;
	}
	if (ready!=0xF)
		return 0;
	i=0;
	int maxV=1;
	int maxH=1;
	while (componentScans[i].id || components[i].id) {
		if (componentScans[i].id != components[i].id) return 0;
		if (componentScans[i].id!=1 &&
			componentScans[i].id!=2 &&
			componentScans[i].id!=3) return 0;
		if (components[i].horizFactor > maxH)
			maxH = components[i].horizFactor;
		if (components[i].vertFactor > maxV)
			maxV = components[i].vertFactor;
		i++;
	}
	int numComponents = i;

	float start[5] = {0};
	int x, y;
	if (!MakeGenericImage<unsigned char>(sv, width, height, 4)) return 0;
	GenericImage<unsigned char> *image = (GenericImage<unsigned char> *) sv.stringVal->value;
	//Section * out = (Section*) malloc(sizeof(Section)*width*height);
	float * out = (float*) malloc(64*3*sizeof(float)*maxH*maxV);

	GenericImage<float> out2;
	out2.pixels = (float*) malloc(64*3*sizeof(float)*maxH*maxV);
	GenericImage<float> in2;
	in2.width = in2.height = 8;
	in2.pixels = (float*) malloc(64*3*sizeof(float)*maxH*maxV);
	if (!out || !out2.pixels || !in2.pixels) {
		sv.stringVal->Release();
		free(out);
		free(out2.pixels);
		free(in2.pixels);
		return 0;
	}
	//in2.pixels = ins->v;

	int offset = 0;
	unsigned int ndata;
	ndata = (data[pos]<<24) + (data[pos+1]<<16) + (data[pos+2]<<8) + data[pos+3];
	pos+=4;
	//memset(out, 0, sizeof(Section)*width*height);
	//memset(image->pixels, 255, width*height*sizeof(Color));
	int pieces = 0;
	maxH*=8;
	maxV*=8;
	for (y=0; y<height; y += maxV) {
		for (x=0; x<width; x += maxH) {
			for (u=maxH*maxV*3-1; u>=0; u--) {
				out[u]=0.5f;
			}

			for (i=0; i<numComponents; i++) {
				//Section *s = &ins[0];

				//int upscalex = maxH/components[i].horizFactor;
				//int upscaley = maxV/components[i].vertFactor;
				//float downscalex = (components[i].horizFactor*8-1)/(float)(maxH*8-1);
				//float downscaley = (components[i].vertFactor*8-1)/(float)(maxV*8-1);
				out2.width = maxH/components[i].horizFactor;
				out2.height = maxV/components[i].vertFactor;
				for (int ly = 0; ly<maxV; ly+=out2.height) {
					for (int lx = 0; lx<maxH; lx+=out2.width) {
						Decode(len, offset, pos, ndata, data, start[i], in2.pixels,
							   htAC[componentScans[i].AC], htDC[componentScans[i].DC]);

						ReorderAndIDCT(in2.pixels, qt[components[i].qt]);

						for (int j=0; j<64; j++) {
							if (in2.pixels[j]<-128)
								in2.pixels[j] = -128;
							else if (in2.pixels[j]>127)
								in2.pixels[j] = 127;
						}

						if (out2.width != 8 || out2.height != 8) {
							//ResampleBilinear(&in2, &out2);
							ResampleBilinear(&in2, &out2);
						}
						else {
							float *temp = out2.pixels;
							out2.pixels = in2.pixels;
							in2.pixels = temp;
							//out2.pixels = ins;
						}

						int endy = ly+out2.height;
						int endx = lx+out2.width;
						//int temp = 

						float *f = out2.pixels;
						float *dest = &out[3*(lx+maxH*ly)];
						if (components[i].id == 1) {
							for (int y2 = ly; y2<endy; y2++) {
								//int temp = out2.width*(y2-ly) - lx;
								//float *dest = &out[3*(lx+maxH*y2)];
								for (int x2 = lx; x2<endx; x2++) {
									//float sum = out2.pixels[x2 + temp]+128;
									float sum = f[0]+128;
									dest[0] += sum;
									dest[1] += sum;
									dest[2] += sum;
									dest+=3;
									f++;
								}
								dest += 3*(maxH-out2.width);
							}
						}
						else if (components[i].id == 2) {
							for (int y2 = ly; y2<endy; y2++) {
								//int temp = out2.width*(y2-ly) - lx;
								//float *dest = &out[3*(lx+maxH*y2)];
								for (int x2 = lx; x2<endx; x2++) {
									//float sum = f[0]+128;
									//float sum = out2.pixels[x2 + temp];
									dest[1] -= 0.34414f * f[0];
									dest[2] += 1.772f   * f[0];
									dest+=3;
									f++;
								}
								dest += 3*(maxH-out2.width);
							}
						}
						else {
							for (int y2 = ly; y2<endy; y2++) {
								//int temp = out2.width*(y2-ly) - lx;
								//float *dest = &out[3*(lx+maxH*y2)];
								for (int x2 = lx; x2<endx; x2++) {
									//float sum = out2.pixels[x2 + temp];
									dest[0] += 1.402f   * f[0];
									dest[1] -= 0.71414f * f[0];
									dest+=3;
									f++;
								}
								dest += 3*(maxH-out2.width);
							}
						}
					}
				}

			}
			if (++pieces == RestartInterval) {
				if (offset) {
					ndata <<= (8-offset);
					offset = 8;
				}
				memset(start, 0, sizeof(start));
				pieces=0;
			}

			float *temp = out;
			Color4 *c = (Color4*)(&image->pixels[4*x+y*image->memWidth]);
			int mu = maxV;
			if (y+mu>=height) mu = height-y;
			int mv = maxH;
			if (x+mv>=width) mv = width-x;
			for (u=0; u<mu; u++) {
				for (v=0; v<mv; v++) {
					if (temp[0]<0) c->r = 0;
					else if (temp[0]>255) c->r = 255;
					else c->r = (unsigned char) temp[0];

					if (temp[1]<0) c->g = 0;
					else if (temp[1]>255) c->g = 255;
					else c->g = (unsigned char) temp[1];

					if (temp[2]<0) c->b = 0;
					else if (temp[2]>255) c->b = 255;
					else c->b = (unsigned char) temp[2];

					c->a = 255;

					temp+=3;
					c++;
				}
				c+=image->memWidth/4-mv;
				temp+=(maxH-mv)*3;
			}

		}
	}
	free(in2.pixels);
	free(out);
	free(out2.pixels);

	return 1;
}