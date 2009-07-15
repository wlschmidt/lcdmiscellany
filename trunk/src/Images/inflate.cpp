#include "inflate.h"
#include "../malloc.h"

struct ExtendedCode {
	unsigned int lengths;
	unsigned int bits;
};



inline unsigned int ConsumeBytes(const unsigned int num, unsigned char* &pos, unsigned int &offset, const unsigned char *endPos) {
	unsigned int out;
	if (pos<endPos) {
		out = ((((unsigned int*)&pos[-4])[0])>>offset)&((1<<num)-1);
		offset+=num;
		pos +=offset>>3;
		offset &= 7;
		return out;
	}

	if (pos-4<endPos) {
		const unsigned char* temp = endPos-1;
		int ndata2 = 0;
		while (temp>=pos-4) {
			ndata2 = (ndata2<<8) + temp[0];
			temp--;
		}
		ndata2>>=offset;
		ndata2 &= ((1<<num)-1);
		offset += num;
		pos +=offset>>3;
		offset &= 7;

		return ndata2;
	}
	return 0;
}

struct HuffmanCode {
	// Max codes is about 300, so at most 299 splits and 300 codes...codes take 1 number, splits two...
	// Actually, now just splits take 2, codes are put in those.  Better safe than sorry, though.
	unsigned int code[1200];

	inline unsigned int GetCode(unsigned char *&pos, unsigned int &offset, const unsigned char*endPos) {
		unsigned int p;//, i;
		unsigned int temp;
		if (pos < endPos) {
			temp = ((unsigned int*)&pos[-4])[0]>>offset;
		}
		else {
			if (pos>=endPos+4) {
				return 256;
			}
			else {
				temp = ((unsigned int*)&endPos[-4])[0]>>(offset+8*(pos+4-endPos));
			}
		}

		p = code[(temp&0xFF)];
		temp>>=8;
		while (p<0x8000) {
			p=code[p+(temp&3)];
			temp >>= 2;
		}

		offset+=p>>16;
		pos += offset>>3;
		offset&= 7;

		return p&0x7FFF;
	}


	void Init(const unsigned int* lengths, const unsigned int num) {
		unsigned int numCodes[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		unsigned int codes[16][300];
		unsigned int i, j, code;

		for (i=0; i<num; i++) {
			codes[lengths[i]][numCodes[lengths[i]]++] = i;
		}
		code=0;
		unsigned int bit = 1<<15;
		for (i=0; i<256; i++) {
			this->code[i] = 0x8000;
		}
		unsigned int next = 256;

		unsigned int k;
		for (i=1; i<16; i++) {
			unsigned int numQuads = ((i+1)&~1)-1;
			for (j=0; j<numCodes[i]; j++) {
				unsigned int pos = 0;
				unsigned int d = ((code>>15)&1) + ((code>>13)&2) + ((code>>11)&4) + ((code>>9)&8) +
								 ((code>>7)&16) + ((code>>5)&32) + ((code>>3)&64) + ((code>>1)&128);
				if (i<=8) {
					int inc = 1<<i;
					int code2 =  codes[i][j] + 0x8000 + (i<<16);
					for (k = d; k < 256; k+=inc) {
						this->code[k] = code2;
					}
					code+=bit;
					continue;
				}
				else {
					if (this->code[d] & 0x8000) {
						this->code[d] = next;
						this->code[next] = 0x8000;
						this->code[next+1] = 0x8000;
						this->code[next+2] = 0x8000;
						this->code[next+3] = 0x8000;
						pos = next;
						next += 4;
					}
					else {
						pos = this->code[d];
					}
				}
				for (k=9; k<numQuads; k+=2) {
					d = ((code>>(16-k))&1) + ((code>>(14-k))&2);
					if (this->code[d+pos] & 0x8000) {
						this->code[d+pos] = next;
						this->code[next] = 0x8000;
						this->code[next+1] = 0x8000;
						this->code[next+2] = 0x8000;
						this->code[next+3] = 0x8000;
						pos = next;
						next += 4;
					}
					else {
						pos = this->code[d+pos];
					}
				}

				d = ((code>>(16-k))&1);
				if (i&1) {
					this->code[d+pos] = codes[i][j] + 0x8000 + (i<<16);
					this->code[d+pos+2] = codes[i][j] + 0x8000 + (i<<16);
				}
				else {
					d += ((code>>(14-k))&2);
					this->code[d+pos] = codes[i][j] + 0x8000 + (i<<16);
				}

				code+=bit;
			}
			bit>>=1;
		}
	}
};


unsigned char * Inflate(unsigned char* compressed, unsigned int compressedBytes, unsigned int size, unsigned int &uncompressedBytes, int gzip) {
	//unsigned int ndata;

	int windowSize = 1<<15;
	unsigned char *pos3 = compressed+4;
	if (!gzip) {
		unsigned char method = compressed[0]&0xF;
		unsigned char flags = compressed[0] >> 4;
		if (method !=8 ||  flags>7 ||
			((compressed[0] * 256 + compressed[1]) %31) ||
			compressed[1] & 0x20) {
			return 0;
		}
		windowSize = 1<<(flags + 8);
		pos3 += 2;
	}
	unsigned char *uncompressed = (unsigned char*) malloc(sizeof(unsigned char)*(size+15));
	if (!uncompressed) return 0;
	//ndata = (compressed[2]) + (compressed[3]<<8) + (compressed[4]<<16) + (compressed[5]<<24);
	unsigned int offset = 0;
	unsigned int bfinal;
	unsigned int type;
	unsigned int i;
	unsigned int pos = 0, dist;
	unsigned int hlit, hdist, hclen;
	unsigned int codeLengths[330];
	HuffmanCode lengths, characters, distances;

	ExtendedCode lengthTable[29] = {{  3,0}, {  4,0}, {  5,0}, {  6,0}, {  7,0}, {  8,0}, 
									{  9,0}, { 10,0}, { 11,1}, { 13,1}, { 15,1}, { 17,1}, 
									{ 19,2}, { 23,2}, { 27,2}, { 31,2}, { 35,3}, { 43,3},
									{ 51,3}, { 59,3}, { 67,4}, { 83,4}, { 99,4}, {115,4},
									{131,5}, {163,5}, {195,5}, {227,5}, {258,0}};

	ExtendedCode distTable[30] =   {{   1,0}, {   2,0}, {   3,0}, {   4,0}, {   5,1}, {   7,1}, 
									{   9,2}, {  13,2}, {  17,3}, {  25,3}, {  33,4}, {  49,4}, 
									{  65,5}, {  97,5}, { 129,6}, { 193,6}, { 257,7}, { 385,7},
									{ 513,8}, { 769,8}, {1025,9}, {1537,9}, {2049,10}, {3073,10},
									{4097,11}, {6145,11}, {8193,12}, {12289,12}, {16385,13}, {24577,13}};

	unsigned int len;
	unsigned int lengthOrder[19] = {16, 17, 18,
		0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	ExtendedCode codeLengthTable[2] = {{3,3}, {11,7}};

	unsigned char* endPos = &compressed[compressedBytes-4];
	while (1) {
		//*
		bfinal = ConsumeBytes(1, pos3, offset, endPos);
		type = ConsumeBytes(2, pos3, offset, endPos);
		//offset+=3;
		//ndata>>=3;
		//*/
		//bfinal = ConsumeBytes(1, pos2, offset, compressed, compressedBytes);
		if (type == 0) {
			if (offset) ConsumeBytes(8-offset, pos3, offset, endPos);
			if (pos3 < endPos) {
				len = pos3[-4] + (pos3[-3]<<8);
				unsigned short nlen = pos3[-2] + (pos3[-1]<<8);
				pos3 += 4;
				if (pos3-4 + len > endPos || len != (unsigned short)~nlen) {
					free(uncompressed);
					return 0;
				}
				if (pos+(unsigned long)len >= size) {
					if (size > (size+size+len) || !srealloc(uncompressed, sizeof(unsigned char)*(size+size+len))) {
						free(uncompressed);
						return 0;
					}
					size += size + len;
				}
				/*if (pos2 + (unsigned long)len >= compressedBytes) {
					break;
				}//*/
				memcpy(&uncompressed[pos], pos3-4, len);
				//pos2+=len;
				pos3+=len;
				pos += len;
				//ConsumeBytes(32, pos3, offset, endPos);
			}
			else {
				free(uncompressed);
				return 0;
			}
		}
		else if (type==3) {
			free(uncompressed);
			return 0;
			//if (type == 1) {
			//i=0;
		}
		else {

			if (type == 2) {
				hlit = ConsumeBytes(5, pos3, offset, endPos) + 257;
				hdist = ConsumeBytes(5, pos3, offset, endPos) + 1;
				hclen = ConsumeBytes(4, pos3, offset, endPos) + 4;

				for (i=0; i<hclen; i++) {
					codeLengths[lengthOrder[i]] = ConsumeBytes(3, pos3, offset, endPos);
				}
				for (;i<19;i++) {
					codeLengths[lengthOrder[i]] = 0;
				}

				lengths.Init(codeLengths, 19);

				for (i=0; i<hlit+hdist; i++) {
					codeLengths[i] = lengths.GetCode(pos3, offset, endPos);
					if (codeLengths[i]>15) {
						if (codeLengths[i]==16) {
							//3 + bytes codes, but the first one is the current one
							len = 3 - 1 + ConsumeBytes(2, pos3, offset, endPos);
							codeLengths[i] = codeLengths[i-1];
							while (len-- && i<330) {
								i++;
								codeLengths[i] = codeLengths[i-1];
							}
						}
						else {
							len = -1 + codeLengthTable[codeLengths[i]-17].lengths +
								ConsumeBytes(codeLengthTable[codeLengths[i]-17].bits, pos3, offset, endPos);
							codeLengths[i]=0;
							while (len-- && i<330) {
								i++;
								codeLengths[i] = 0;
							}
						}
					}
				}

				characters.Init(codeLengths, hlit);
				distances.Init(&codeLengths[hlit], hdist);

				/*for (i=0; i<hdist; i++) {
					codeLengths[i] = lengths.GetCode(pos2, offset, compressed, compressedBytes);
					if (codeLengths[i]>15) {
						if (codeLengths[i]==16) {
							//3 + bytes codes, but the first one is the current one
							len = 3 - 1 + ConsumeBytes(2, pos2, offset, compressed, compressedBytes);
							codeLengths[i] = codeLengths[i-1];
							while (len-- && i<299) {
								i++;
								codeLengths[i] = codeLengths[i-1];
							}
						}
						else {
							len = -1 + codeLengthTable[codeLengths[i]-17].lengths +
								ConsumeBytes(codeLengthTable[codeLengths[i]-17].bits, pos2, offset, compressed, compressedBytes);
							codeLengths[i]=0;
							while (len-- && i<299) {
								i++;
								codeLengths[i] = 0;
							}
						}
					}
				}

				distances.Init(codeLengths, hdist);*/
			}
			else {
				for (i=0; i<144; i++)
					codeLengths[i] = 8;
				for (   ; i<256; i++)
					codeLengths[i] = 9;
				for (   ; i<280; i++)
					codeLengths[i] = 7;
				for (   ; i<288; i++)
					codeLengths[i] = 8;
				characters.Init(codeLengths, i);

				for (i=0; i<30; i++)
					codeLengths[i] = 5;
				distances.Init(codeLengths, i);
			}

			while (1) {
				if (285+pos>size) {
					if (size > (size+size + 285) || !srealloc(uncompressed, sizeof(unsigned char)*(size+size + 285))) {
						free(uncompressed);
						return 0;
					}
					size += size + 285;
				}
				register unsigned int i=characters.GetCode(pos3, offset, endPos);


				if (i<256) {
					uncompressed[pos++] = (unsigned char) i;
				}
				else if (i>256) {
					len = lengthTable[i-257].lengths +
						ConsumeBytes(lengthTable[i-257].bits, pos3, offset, endPos);

					dist = distances.GetCode(pos3, offset, endPos);
					dist = distTable[dist].lengths +
						ConsumeBytes(distTable[dist].bits, pos3, offset, endPos);

					if (pos>=dist) {
						while (len+pos>size) {
							if (size > (size+=size) || !srealloc(uncompressed, sizeof(unsigned char)*size)) {
								free(uncompressed);
								return 0;
							}
						}

						/*// memcpy is not safe to use here.
						if (dist>=len) {
							memcpy(&uncompressed[pos], &uncompressed[pos-dist], len);
							pos+=len;
						}
						else {*/
						for (i=0; i<len; i++) {
							uncompressed[pos] = uncompressed[pos-dist];
							pos++;
						}
						//}
					}
				}
				else {
					break;
				}
			}
		}
		/*else if (type == 3) {
			i=0;
		}*/
		if (bfinal) {
			break;
		}
	}
	if (offset) ConsumeBytes(8-offset, pos3, offset, endPos);
	uncompressedBytes = pos;
	//unsigned int test = Adler32(uncompressed, pos);
	//unsigned int test2 = (compressed[pos2-4]<<24)+(compressed[pos2-3]<<16)+(compressed[pos2-2]<<8)+compressed[pos2-1];
	// | 1 makes sure I don't get a 0-length result, if all goes well.
	srealloc(uncompressed, (uncompressedBytes | 1) * sizeof(char));
	return uncompressed;
}


#define FTEXT		1
#define FHCRC		2
#define FEXTRA		4
#define FNAME		8
#define FCOMMENT	16


char *InflateGzip(char *compressed, unsigned int compressedBytes, unsigned int &uncompressedBytes) {
	if (compressedBytes<10) return 0;
	unsigned int p = 0;
	unsigned char id1 = compressed[p++],
		id2 = compressed[p++],
		cm = compressed[p++],
		flg = compressed[p++];
	int mtime = ((int*)&compressed[p])[0];
	p+=4;
	unsigned char xfl = compressed[p++],
		os = compressed[p++];
	if (id1 != 0x1f || // gzip
		id2 != 0x8b || // gzip
		cm != 8) // deflate
			return 0;
	if (flg & FEXTRA) {
		unsigned short len = ((unsigned short*)&compressed[p])[0];
		p+=2;
		p += len;
	}
	if (flg & FNAME) {
		while (p<compressedBytes && compressed[p]) p++;
		p++;
	}
	if (flg & FCOMMENT) {
		while (p<compressedBytes && compressed[p]) p++;
		p++;
	}
	if (flg & FHCRC) p+=2;
	if (p>=compressedBytes) return 0;
	return (char*) Inflate((unsigned char*)&compressed[p], compressedBytes-p, compressedBytes * 3, uncompressedBytes, 1);
}
