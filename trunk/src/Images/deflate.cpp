#include "deflate.h"
#include "string.h"

inline long FlipLong(unsigned char *c) {
	unsigned long s;
	((unsigned char*)&s)[0] = c[3];
	((unsigned char*)&s)[1] = c[2];
	((unsigned char*)&s)[2] = c[1];
	((unsigned char*)&s)[3] = c[0];
	return s;
}

#define BASE 65521

inline unsigned long UpdateAdler32(unsigned long adler, unsigned char *buf, int len) {
	unsigned long s1 = adler & 0xffff;
	unsigned long s2 = (adler >> 16) & 0xffff;
	int n;

	for (n = 0; n < len; n++) {
		s1 = (s1 + buf[n]) % BASE;
		s2 = (s2 + s1)     % BASE;
	}
	return (s2 << 16) + s1;
}

/* Return the adler32 of the bytes buf[0..len-1] */

inline unsigned long Adler32(unsigned char *buf, int len) {
	return UpdateAdler32(1L, buf, len);
}


struct WriteBuffer {
	unsigned int nextData, offset, size;
	unsigned char* pos;
	unsigned char* data;
};

inline void WriteBytesForce(const unsigned int num, const unsigned int bytes, WriteBuffer *buffer) {
	buffer->nextData += bytes<<buffer->offset;
	buffer->offset += num;
	while (buffer->offset>7) {
		/*if (buffer->pos>=buffer->size) {
			buffer->size+=buffer->size;
			buffer->data = (unsigned char*) realloc(buffer->data, buffer->size);
		}*/
		buffer->pos[0] = (unsigned char) buffer->nextData;
		buffer->pos ++;
		buffer->offset -= 8;
		buffer->nextData >>= 8;
	}
}

/*
struct WriteBuffer {
	unsigned int nextData, pos, offset, size;
	unsigned char* data;
};
*/

inline void WriteBytes(const unsigned int num, const unsigned int bytes, WriteBuffer *buffer) {
	__asm {
		mov edi, buffer
		mov esi, bytes
		mov ecx, [edi+4] //offset
		mov edx, num
		mov eax, [edi]   //nextData
		shl esi, cl
		add edx, ecx
		add eax, esi
		cmp edx, 32
		jb skip
		mov esi, [edi+12] //pos
		neg ecx
		mov [esi], eax
		add esi, 4
		add ecx, 32
		sub edx, 32
		mov eax, bytes
		mov [edi+12], esi
		shr eax, cl
skip:
		mov [edi+4], edx
		mov [edi], eax
	}
}

/*inline void WriteBytes(const unsigned int num, const unsigned int bytes, WriteBuffer *buffer) {
	__asm {
		mov edi, buffer
		mov esi, bytes
		mov ecx, [edi+8] //offset
		mov edx, num
		mov eax, [edi]   //nextData
		shl esi, cl
		add edx, ecx
		add eax, esi
		cmp edx, 32
		jb skip
		mov esi, [edi+4] //pos
		mov ecx, [edi+16] //data
		sub edx, 32
		mov [ecx+esi], eax
		mov ecx, [edi+8] // original offset
		add esi, 4
		neg ecx
		mov eax, bytes
		add ecx, 32
		mov [edi+4], esi
		shr eax, cl
skip:
		mov [edi+8], edx
		mov [edi], eax
	}
}*/


/*
inline void WriteBytesForce(const unsigned int num, const unsigned int bytes, unsigned int &ndata, unsigned int &pos, unsigned int &offset, unsigned char* &data, unsigned int &size) {
	ndata += bytes<<offset;
	offset += num;
	while (offset>7) {
		if (pos>=size) {
			size+=size;
			data = (unsigned char*) realloc(data, size);
		}
		data[pos] = (unsigned char) ndata;
		pos ++;
		offset -= 8;
		ndata >>= 8;
	}
}

inline void WriteBytes(const unsigned int num, const unsigned int bytes, unsigned int &ndata, unsigned int &pos, unsigned int &offset, unsigned char* &data, unsigned int &size) {
	if (offset+num>=32) {
		if (pos+5>=size) {
			size+=size+5;
			data = (unsigned char*) realloc(data, size);
		}
		((unsigned int*)(&data[pos]))[0] = ndata;
		pos += (offset>>3);
		ndata >>= (offset&~7);
		offset &= 7;*/
		/*((unsigned int*)(&data[pos]))[0] = ndata;
		unsigned int pos2 = pos+(offset>>3);
		unsigned int ndata2 = ndata >> (offset&~7);
		unsigned int offset2 = offset & 7;
		do {
			if (data[pos]!=(unsigned char) ndata)
				pos=pos;
			data[pos] = (unsigned char) ndata;
			pos ++;
			offset -= 8;
			ndata >>= 8;
		}
		while (offset>7);
		if (offset!=offset2 ||
			ndata!=ndata2 ||
			pos!=pos2)
		pos2=pos2;*/
/*	}
	ndata += bytes<<offset;
	offset += num;
}*/

struct ExtendedCode {
	unsigned int lengths;
	unsigned int bits;
};

struct Code {
	unsigned int length, code;
};


struct Jump {
	unsigned char* pos;
	unsigned int distance, len, distCode, lenCode;
};

inline unsigned int LookupCode2(ExtendedCode *t, unsigned int val) {
	int i;
	for (i=0; 1;i++) {
		if (val < t[i+1].lengths) return i;
	}
}


inline unsigned int LookupCode(ExtendedCode *t, unsigned int val) {
	int i;
	for (i=0; 1;i++) {
		if (t[i].lengths + (1<<t[i].bits) > val) return i;
	}
}

struct LengthGenData {
	int character;
	float score;
};

inline void GenLengths(unsigned int *counts, unsigned int *codeLengths, unsigned int &numChars, const unsigned int maxDepth) {
	LengthGenData lengths[16][300];
	int numCodes[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int firstCode[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int lastCode[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//float scores[300];
	//int depths[300];
	unsigned int i;
	unsigned long freeSpace = (1<<16);
	int num=0;
	unsigned int maxChar = 0;
	int count=0;
	for (i=0; i<numChars; i++) {
		count+= counts[i];
	}
	for (i=0; i<numChars; i++) {
		if (counts[i]) {
			num++;
			int temp = count/counts[i];

			// Could be 0, but I worry about exceeding maxDepth or having <0 freeSpace.
			//codeLengths[i] = 1;
			//int div = 1;
			unsigned int codeLength = 1;
			while (temp) {
				codeLength ++;
				temp>>=1;
				//div<<=1;
			}
			float tempScore;
			if (codeLength <maxDepth) {
				tempScore = ((float) counts[i])/(1<<(16-codeLength));
			}
			else {
				tempScore = (float) counts[i];
				codeLength = maxDepth;
			}
			int pos;
			for (pos = numCodes[codeLength]; pos > 0; pos--) {
				if (tempScore <= lengths[codeLength][pos-1].score) break;
				lengths[codeLength][pos] = lengths[codeLength][pos-1];
			}
			lengths[codeLength][pos].score = tempScore;
			lengths[codeLength][pos].character = i;
			lastCode[codeLength]=++numCodes[codeLength];
			freeSpace -= 1<<(16-codeLength);
			maxChar = i+1;
		}
		else {
			codeLengths[i] = 0;
		}
	}
	if (num<2) {
		if (num) {
			for (i=0; i<maxChar; i++) {
				if (codeLengths[i]) {
					codeLengths[i]=1;
					break;
				}
			}
			if (i) codeLengths[i-1]=1;
			else codeLengths[1]=1;
		}
		else {
			if (i) codeLengths[i-1]=1;
			else codeLengths[1]=1;
		}
		return;
	}
	numChars = maxChar;
	while (freeSpace) {
		float maxScore = -1;
		int maxPos;
		for (i=2; i<16; i++) {
			if (freeSpace>>(16-i) && numCodes[i]) {
				if (lengths[i][firstCode[i]].score>maxScore) {
					maxScore = lengths[i][firstCode[i]].score;
					maxPos = i;
				}
			}
		}


		freeSpace -= (1<<(16-maxPos));
		lengths[maxPos-1][lastCode[maxPos-1]].character = lengths[maxPos][firstCode[maxPos]].character;
		lengths[maxPos-1][lastCode[maxPos-1]++].score = lengths[maxPos][firstCode[maxPos]++].score/2;
		numCodes[maxPos-1]++;
		numCodes[maxPos]--;

	
	}
	for (i=1; i<16; i++) {
		for (int j=firstCode[i]; j<lastCode[i]; j++) {
			codeLengths[lengths[i][j].character] = i;
		}
	}
}



struct HuffmanCode2 {
	Code codes[300];

	/*inline unsigned int GetCode(unsigned int &ndata, unsigned int &pos, unsigned int &offset, const unsigned char* data, const unsigned int size) {
		unsigned int p = 0, i;
		for (i=0; i<15; i++) {
			p+= (ndata>>i)&1;
			if (code[p]&0x8000) {
				break;
			}
			p = code[p];
		}
		ConsumeBytes(i+1, ndata, pos, offset, data, size);
		return code[p] & 0x7FFF;
	}*/

	inline void Init(const unsigned int* lengths, const unsigned int num) {
		unsigned int numCodes[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		unsigned int codes[16][300];
		unsigned int i, j, code;
		//for (i=1; i<16; i++) {
		//	numCodes[i]=0;
		//}
		for (i=0; i<num; i++) {
			codes[lengths[i]][numCodes[lengths[i]]++] = i;
		}
		code=0;
		unsigned int bit = 1<<15;
		for (i=1; i<16; i++) {
			for (j=0; j<numCodes[i]; j++) {
				unsigned int pos = 0;
				unsigned int flippedCode = 0;
				for (unsigned int k=1; k<=i; k++) {
					flippedCode += ((code>>(16-k))&1)<<(k-1);
				}
				this->codes[codes[i][j]].code = flippedCode;
				this->codes[codes[i][j]].length = i;
				code+=bit;
			}
			bit>>=1;
		}
		for (j=0; j<numCodes[0]; j++) {
			this->codes[codes[0][j]].length=0;
		}
	}
};


unsigned char * DeflateSSE2(unsigned char* uncompressed, unsigned int uncompressedBytes, unsigned int &compressedBytes, unsigned int lineLength) {
	__declspec(align(16)) int table[16384][2];
	__declspec(align(16)) unsigned int counts[336];
	__declspec(align(16)) unsigned int distCheck[4] = {35000, 35000, 35000, 35000};
	unsigned int s1=1, s2=0;
	//Tree trees[600];
	unsigned char *pos = (unsigned char*) malloc(sizeof(unsigned char)*(uncompressedBytes*2+400));
	WriteBuffer buffer = {0,0,uncompressedBytes*2+400, pos, pos};
	//memset(&buffer, 0, sizeof(buffer));

	//buffer.size = uncompressedBytes*2+400;
	//buffer.pos = buffer.data = (unsigned char*) malloc(sizeof(unsigned char)*(buffer.size));
	WriteBytes(16, 0x9C78, &buffer);
	//compressed[0] = 0x78;
	//compressed[1] = 0x9C;


	//unsigned int pos2=0;
	pos = uncompressed;
	unsigned int i, j;
	//unsigned int dist;

	unsigned int bfinal = 0;
	unsigned int type;
	unsigned int codeLengths[360];
	HuffmanCode2 lengths, characters, distances;

	ExtendedCode lengthTable[30] = {{  3,0}, {  4,0}, {  5,0}, {  6,0}, {  7,0}, {  8,0}, 
									{  9,0}, { 10,0}, { 11,1}, { 13,1}, { 15,1}, { 17,1}, 
									{ 19,2}, { 23,2}, { 27,2}, { 31,2}, { 35,3}, { 43,3},
									{ 51,3}, { 59,3}, { 67,4}, { 83,4}, { 99,4}, {115,4},
									{131,5}, {163,5}, {195,5}, {227,5}, {258,0},
									{33333,0}};

	ExtendedCode distTable[31] =   {{   1,0}, {   2,0}, {   3,0}, {   4,0}, {   5,1}, {   7,1}, 
									{   9,2}, {  13,2}, {  17,3}, {  25,3}, {  33,4}, {  49,4}, 
									{  65,5}, {  97,5}, { 129,6}, { 193,6}, { 257,7}, { 385,7},
									{ 513,8}, { 769,8}, {1025,9}, {1537,9}, {2049,10}, {3073,10},
									{4097,11}, {6145,11}, {8193,12}, {12289,12}, {16385,13}, {24577,13},
									{332769, 0}};

	//unsigned int len;
	unsigned int lengthOrder[19] = {16, 17, 18,
		0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	ExtendedCode codeLengthTable[2] = {{3,3}, {11,7}};
	unsigned int blockLen=0;

	Jump *jumps = (Jump*) malloc((uncompressedBytes*sizeof(Jump))/8);

	unsigned int code;

/*	__asm {
		mov s1, BASE
		mov ecx, uncompressed
		mov esi, uncompressedBytes
		push ebx
		//dec ecx
		mov eax, 1
		xor edx, edx
		add ecx, esi
		xor edi, edi
		mov temp, ecx
		neg esi

		mov ebx, [esi+ecx]
//adlerLoopStart2:
adlerLoopStart:
		movzx ecx, bl
		movzx edx, bh

		//mov cl, dl
		//shr edx, 8
		add eax, ecx
		//mov ecx, 0xFF
		lea edi, [edi+4*eax]
		//mov cl, dh
		shr ebx, 16
		add esi, 4
		mov cl, bl
		shr ebx, 8
		// d = 2nd (3), c = 3rd (2), b = 4th (1)
		add eax, ebx
		add ecx, edx
		add ebx, edx
		mov edx, temp
		lea edi, [edi + 2*ecx]
		add eax, ecx
		lea ecx, [4 + esi]
		add edi, ebx

		mov ebx, [esi+edx]
		//and ecx, esi
		//add eax, ebx
		//mov ecx, 0xFF
		//add edi, eax
		//mov cl, dl
		//shr edx, 8
		//add eax, ecx
		//add edi, eax
		//add eax, edx

		//add edi, eax

		////mov temp2, edx
		////mov edx, ecx
		////mov ecx, temp2

		test ecx, 0x7FC
		jnz adlerLoopStart
		xor edx, edx
		div dword ptr[s1]
		mov eax, edi
		mov edi, edx
		xor edx, edx
		div dword ptr[s1]
		mov eax, edi
		mov edi, edx
		//xor edx, edx
		cmp esi, -4
		jl adlerLoopStart

		test esi, esi
		jz adlerEnd
		mov ecx, temp

adlerLoop2Start:
		xor edx, edx
		mov dl, [esi+ecx]
		add eax, edx
		inc esi
		add edi, eax
		test esi, esi
		jnz adlerLoop2Start
adlerEnd:
		pop ebx
		xor edx, edx
		div dword ptr[s1]
		mov eax, edi
		mov edi, edx
		xor edx, edx
		div dword ptr[s1]
		//mov eax, edi
		//mov edi, edx
		//mov s1, eax
		//mov s2, edi
		mov s1, edi
		mov s2, edx
	}*/
	__asm {
		mov s1, BASE
		mov ecx, uncompressed
		mov esi, uncompressedBytes
		push ebx
		//dec ecx
		mov eax, 1
		xor edx, edx
		//add ecx, esi
		lea ebx, [ecx+esi]
		xor edi, edi
		//mov temp, ecx
		neg esi

		//mov ebx, ecx
		mov ecx, [esi+ebx]
//adlerLoopStart2:
		add esi, 4
adlerLoopStart:
		//xor ecx, ecx

		movzx edx, cl
		//shr edx, 8
		add eax, edx
		//mov ecx, 0xFF
		add edi, eax
		movzx edx, ch
		add eax, edx
		add esi, 4
		shr ecx, 16

		add edi, eax
		//mov ecx, 0xFF
		movzx edx, ch
		movzx ecx, cl
		add eax, ecx
		mov ecx, [esi+ebx-4]
		//shr ecx, 8
		//mov ecx, temp
		add edi, eax
		add eax, edx

		//lea edx, [esi+4]
		//mov edx, 0x7FC
		//and edx, esi
		add edi, eax

		/*mov temp2, edx
		mov edx, ecx
		mov ecx, temp2*/

		test esi, 0x7FC
		jnz adlerLoopStart
		xor edx, edx
		div dword ptr[s1]
		mov eax, edi
		mov edi, edx
		xor edx, edx
		div dword ptr[s1]
		mov eax, edi
		mov edi, edx
		//xor edx, edx
		cmp esi, 0
		jl adlerLoopStart
		sub esi, 4

		test esi, esi
		jz adlerEnd

adlerLoop2Start:
		xor edx, edx
		mov dl, [esi+ebx]
		add eax, edx
		inc esi
		add edi, eax
		test esi, esi
		jnz adlerLoop2Start
adlerEnd:
		pop ebx
		xor edx, edx
		div dword ptr[s1]
		mov eax, edi
		mov edi, edx
		xor edx, edx
		div dword ptr[s1]
		//mov eax, edi
		//mov edi, edx
		//mov s1, eax
		//mov s2, edi
		mov s1, edi
		mov s2, edx
	}

	type = 0;
	//unsigned int i;
	//unsigned int distCheck[11] = {2, 1, 3, lineLength-9, lineLength-6, lineLength-3, lineLength, lineLength+3, lineLength+6, lineLength+9, uncompressedBytes+100};
	//memset(table, -1, sizeof(table));
	int remainingLength = uncompressedBytes;
	//int startPos = 0;

	//unsigned int nextHash = QuickHash(((int*)pos)[0]);


	{
		//__declspec(align(16)) unsigned int temp[4] = {-35000, -35000, -35000, -35000};
		__declspec(align(16)) unsigned char* temp[4] = {pos-35000, pos-35000, pos-35000, pos-35000};
		__asm{
			movdqa xmm0, temp
			mov eax, 0x20000
			//lea edi, [table]
cleantemp2:
			sub eax, 0x40
			movdqa [table+eax+0x30], xmm0
			movdqa [table+eax+0x20], xmm0
			movdqa [table+eax+0x10], xmm0
			movdqa [table+eax], xmm0
			jne cleantemp2
		}
	}
	while (!bfinal) {
		//if (uncompressedBytes-pos2 <= 32768)
		blockLen = remainingLength;
		//pos = &uncompressed[uncompressedBytes-remainingLength];
		if (blockLen>lineLength*120) blockLen = lineLength*80;
		else bfinal = 1;

		unsigned char* blockEnd = pos+blockLen;
		unsigned char* blockEnd2 = pos+blockLen-4;
		if (0&&uncompressedBytes<250) {
			//type = 2;
			type = 1;
			//blockLen = 0;
		}
		else {
			//bfinal = 0;
			//type = 2;
			//type = 1;
			//bfinal = 1;
			//type = 2;
			type = 2;
		}
		WriteBytes(3, (type<<1) + bfinal, &buffer);
		unsigned int safetyEnd = remainingLength-16;
		unsigned char *safetyPos = pos + safetyEnd;
		if (safetyPos>blockEnd) safetyPos = blockEnd;
		//safetyPos--;
		//safetyPos-=16;

		Jump *jump = jumps;


		unsigned int numChars;
		//int nextDist;

		if (type==2) {


			__asm{
				pxor xmm0, xmm0
				mov eax, 0x540
				//lea edi, [counts]
		cleancounts:
				sub eax, 0x40
				movdqa [counts+eax+0x30], xmm0
				movdqa [counts+eax+0x20], xmm0
				movdqa [counts+eax+0x10], xmm0
				movdqa [counts+eax], xmm0
				jne cleancounts
			}

			unsigned char *pos2;// = pos+i;
			//unsigned int matched;
			//unsigned int matchedDist;
			unsigned int last = 0;
			__asm {
				mov edi, pos
				//mov pos2, edi
				cmp edi, safetyPos
				jge deflateLoopEnd
				push ebx
				xor ebx, ebx
deflateLoop:
					mov eax, [edi+1]
					mov esi, [edi+5]
					mov ecx, [edi+9]
					mov edx, [edi+13]
					//mov ebx, last
					prefetcht0 [edi+0x10]
					////mov matched, 12
							ror ecx, 21
							ror edx, 5

					xor ecx, eax
					add edx, eax

					rol eax, 3
					sub eax, esi
					//mov ecx, esi
					sar esi, 15

					add eax, esi
					//mov esi, last
					mov esi, dword ptr table[ebx*4]
					//xor esi, esi

					sar ecx, 13
					sar edx, 25

					mov dword ptr table[ebx*4], edi
					mov ebx, [edi]

					xor ecx, edx
					add ecx, eax
					sub esi, edi
					////mov eax, dword ptr table[ebx*8]
					//xor eax, edx
					//sub ecx, esi
					//mov edi, i

					//xor eax, ecx
					////mov edx, edi
					////sub edx, dword ptr table[ebx*8+4]
					//add edi, startPos
					//mov key, eax




					and ecx, 0x3FFF
					mov last, ecx

					////sub esi, eax
					//sub edi, dword ptr table[eax*8]
					cmp esi, -(0x8000)

					////mov dword ptr table[ebx*8+4], eax
					//mov [distCheck], edi
					////mov [distCheck+4], edx

					jle skipSearchFast
					//mov edx, [esi+edi]
					////mov j, 0
					//mov esi, eax
//startSearch:
					//mov edx, -4
					//sub edx, edi

					//mov ecx, 12
					//sub edx, 4


					cmp ebx, [esi+edi]
					//cmp edx, [edi+esi]
					jne skipSearchFast

					mov edx, blockEnd2
					mov ecx, 254
					sub edx, edi
					//add edi, esi
					//neg esi
					xor eax, eax
					cmp edx, ecx
					cmovl ecx, edx
startQuadMatch:

					cmp eax, ecx
					jg endQuadMatch

					//add edi, 4
					add eax, 4
					mov edx, [edi+4]
					cmp edx, [edi+esi+4]
					lea edi, [edi+4]
					je startQuadMatch

endQuadMatch:
					////cmp eax, matched
					cmp eax, 12
					jle tryNext
					add ecx, 3
					sub eax, 3
					sub edi, 3

					//mov edx, 257
					//cmp ecx, 257
					//cmovge ecx, edx

startSingleMatch:
					inc eax
					inc edi
					cmp eax, ecx
					jg endSingleMatch

					mov dl, [edi]
					cmp dl, [edi+esi]
					je startSingleMatch
//					jmp startSingleMatch

//					align 16
endSingleMatch:
					////cmp eax, matched
					neg esi
					cmp eax, 12
					jg jumpBranch
					//mov matched, eax
					//mov matchedDist, esi

					//Basically duplicated code.  Avoid a delay on access matched twice in a row
					//add edi, esi
					//jmp jumpBranch
					/*mov ecx, j
					add edi, esi
					mov esi, [distCheck+4*ecx+4]
					sub edi, eax
					inc ecx
					cmp esi, 0x8000
					//mov edi, pos2
					mov j, ecx
					jl startSearch
					jmp jumpBranch
					//*/
tryNext:
					sub edi, eax
					////mov ecx, j
					//sub edi, eax
tryNext2:
					////mov esi, [distCheck+4*ecx+4]
					//add edi, esi
					////inc ecx
					////cmp esi, 0x8000
					//mov edi, pos2
					////mov j, ecx
					////jl startSearch
skipSearch:
					////mov eax, matched
					////cmp eax, 13
					////jge jumpBranch
					// Better a misalignment than 8-bit memory access
					//mov ecx, 0xFF
					//and ecx, [edi]
					//inc counts[4*ecx]
					//and ebx, 0xFF
					and ebx, 0xFF
					inc counts[4*ebx]
					inc edi
					mov ebx, last
					jmp nextPos
skipSearchFast:
					////mov eax, matched
					////cmp eax, 13
					////jge jumpBranch
					// Better a misalignment than 8-bit memory access
					//mov ecx, 0xFF
					//and ecx, [edi]
					//inc counts[4*ecx]
					//and ebx, 0xFF
					and ebx, 0xFF
					inc counts[4*ebx]
					inc edi
					mov ebx, ecx
					jmp nextPos
jumpBranch:

					sub edi, eax
					////mov esi, [matchedDist]
					mov ebx, [jump]
					//mov pos2, edi
					mov [ebx], edi
					mov [ebx+4], esi
					mov [ebx+8], eax

					mov edx, -1
					mov ecx, 8
findDist:
					inc edx
					cmp esi, dword ptr[distTable + 8*edx+8]
					jge findDist
					mov [ebx + 12], edx
					//add edx, 287
					inc [counts+4*edx+4*287]

findLength:
					inc ecx
					cmp eax, dword ptr[lengthTable + 8*ecx+8]
					jge findLength
					mov [ebx + 16], ecx
					//add ecx, 257
					inc [counts+4*ecx+4*257]

					add ebx, 20
					mov [jump], ebx
					//pop ebx
				//}
					
				/*if (matched == 258)
					jump->lenCode = 28;
				else
					jump->lenCode = LookupCode2(lengthTable, jump->len);

				/*if (matched<9) {
					counts[pos2[0]]++;
				}
				else {*/
				//jump->distance = matchedDist;
				//jump->len = matched;
/*struct Jump {
	unsigned char* pos;
	unsigned int distance, len, distCode, lenCode;
};*/
					/*__asm {
						mov eax, numJumps
						mov esi, numJumps
						lea eax, [eax+4*eax]
						lea esi, [esi+4*eax]
						mov edx, matched
						cmp edx, 258
						jne LookupCodeNot258
						mov edi, 28
						jmp EndLookup
LookupCodeNot258:
						mov eax, 0
						
EndLookup:
						mov [esi+8], edi
						}
						*/
/*inline unsigned int LookupCode(ExtendedCode *t, unsigned int val) {
	int i;
	for (i=0; 1;i++) {
		if (t[i].lengths + (1<<t[i].bits) > val) return i;
	}
}
					}*/
				/*if (30000<matchedDist) {
					pos=pos;
				}*/
				/*if (matched == 258)
					jump->lenCode = 28;
				else
					jump->lenCode = LookupCode2(lengthTable, jump->len);
				//*/
				//jump->distCode= LookupCode2(distTable, matchedDist);
				//counts[jump->lenCode+257]++;
				//counts[jump->distCode+287]++;
				//jump->pos = pos2;
				//jump++;
				//__asm {
					//push ebx

					//mov eax, matched
					//mov edi, pos2
					dec eax
					mov ebx, edi
					//add eax, edi
					//mov pos2, eax
					add ebx, eax
					//cmp eax, safetyEnd
					//jle nextPos
				//}
				//j=matched-1;
				//i++;
				//if (j<safetyEnd) {
					//while(i<j) {
					/*unsigned int test[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF};
					__asm {
						mov edi, pos2
						inc edi
						movdqu xmm0, [edi]
						movdqu xmm4, [edi+4]
						//movdqu xmm1, [edi]
						//movdqu xmm2, [edi+4]
						movdqu xmm3, [test]

						movdqa xmm5, xmm0
						movdqa xmm6, xmm0
						movdqa xmm7, xmm4

						pslld xmm0, 3
						psrad xmm5, 13
						psrad xmm6, 25

						psubd xmm0, xmm5

						psrad xmm4, 15
						pxor xmm0, xmm6
						psubd xmm7, xmm4
						pxor xmm0, xmm7
						pand xmm0, xmm3



						movdqu xmm1, [edi+1]
						movdqu xmm4, [edi+5]

						movdqa xmm5, xmm1
						movdqa xmm6, xmm1
						movdqa xmm7, xmm4

						pslld xmm1, 3
						psrad xmm5, 13
						psrad xmm6, 25

						psubd xmm1, xmm5

						psrad xmm4, 15
						pxor xmm1, xmm6
						psubd xmm7, xmm4
						pxor xmm1, xmm7
						pand xmm1, xmm3



						movdqu xmm2, [edi+2]
						movdqu xmm4, [edi+6]

						movdqa xmm5, xmm2
						movdqa xmm6, xmm2
						movdqa xmm7, xmm4

						pslld xmm2, 3
						psrad xmm5, 13
						psrad xmm6, 25

						psubd xmm2, xmm5

						psrad xmm4, 15
						pxor xmm2, xmm6
						psubd xmm7, xmm4
						pxor xmm2, xmm7
						pand xmm2, xmm3



						movdqu xmm7, [edi+3]
						movdqu xmm4, [edi+7]

						movdqa xmm5, xmm7
						movdqa xmm6, xmm7

						pslld xmm7, 3
						psrad xmm5, 13
						psrad xmm6, 25

						psubd xmm7, xmm5
						movdqa xmm5, xmm4

						psrad xmm4, 15
						pxor xmm7, xmm6
						psubd xmm5, xmm4
						pxor xmm7, xmm5
						pand xmm3, xmm7

						movdqa xmm4, xmm0
						punpckldq xmm0, xmm2
						punpckhdq xmm4, xmm2

						movdqa xmm5, xmm1
						punpckldq xmm1, xmm3
						punpckhdq xmm5, xmm3

						movdqa xmm2, xmm0
						punpckldq xmm0, xmm1
						punpckhdq xmm2, xmm1
						movdqa xmm1, xmm2

						movdqa xmm2, xmm4
						punpckldq xmm2, xmm5
						punpckhdq xmm4, xmm5
						movdqa xmm3, xmm4
					}*/
					//unsigned int temp;
					//	__asm {
					//		mov edi, pos2
					//		//mov edx, i
					//		mov eax, j
							//add edx, eax
					//		add eax, edi
							//inc edi
					//		mov pos2, eax
								//mov edi, eax
								//jmp nextPos
							/*inc edi
							mov last, ecx
							mov dword ptr table[ecx*4], edi
							//*/
							mov ecx, last
							inc edi
							mov eax, [edi+1]
							mov esi, [edi+5]
							mov dword ptr table[ecx*4], edi
					mov ecx, [edi+9]
					mov edx, [edi+13]
hashLoop:
							inc edi

							ror ecx, 21
							ror edx, 5

							xor ecx, eax
							add edx, eax

							rol eax, 3
							sub eax, esi
							//mov ecx, esi
							sar esi, 15
							sar ecx, 13
							sar edx, 25


							add eax, esi
							xor ecx, edx
							add ecx, eax

							mov eax, [edi+1]
							mov esi, [edi+5]

							and ecx, 0x3FFF
							////mov edx, dword ptr table[ecx*8]

							cmp edi, ebx
							mov dword ptr table[ecx*4], edi
							////mov dword ptr table[ecx*8+4], edx
					mov ecx, [edi+9]
					mov edx, [edi+13]
							jl hashLoop

							ror ecx, 21
							ror edx, 5

							xor ecx, eax
							add edx, eax

							rol eax, 3
							sub eax, esi
							//mov ecx, esi
							sar esi, 15
							sar ecx, 13
							sar edx, 25

							inc edi
							/*sub eax, ecx


							mov ecx, esi
							sar esi, 15
							xor eax, edx
							mov edx, edi
							inc edi
							sub ecx, esi



							xor eax, ecx*/
							add eax, esi
							xor ecx, edx
							add ecx, eax

							and ecx, 0x3FFF
							mov ebx, ecx
				//		}
				//}
				//else {
				//	pos2+=j;
				//}
			//}
			//__asm {
//nextPos2:
			//	mov edi, pos2
nextPos:
				cmp edi, safetyPos
				jl deflateLoop

deflateLoopEnd:
				pop ebx
				mov pos2, edi
			}
			for (;pos2<blockEnd;pos2++) {
				counts[pos2[0]]++;
			}
			jump->pos = pos2+10;
			counts[256] = 1;
			numChars = 286;
			GenLengths(counts, codeLengths, numChars, 15);
			if (numChars==-1) {
				numChars = 286;
				for (i=0; i<144; i++)
					codeLengths[i] = 8;
				for (   ; i<256; i++)
					codeLengths[i] = 9;
				for (   ; i<282; i++)
					codeLengths[i] = 7;
				for (   ; i<286; i++)
					codeLengths[i] = 8;
				for (   ; i<300; i++)
					codeLengths[i] = 8;
			}
			characters.Init(codeLengths, numChars);
			if (numChars<257) {
				// should never get here, but you never know
				numChars = 257;
			}

			unsigned int numDistances = 30;
			GenLengths(&counts[287], &codeLengths[numChars], numDistances, 15);
			if (numDistances==-1) {
				numDistances = 30;
				for (i=0; i<30; i++)
					codeLengths[numChars+i] = 5;
				codeLengths[numChars+2] = 4;
				codeLengths[numChars+3] = 4;
			}
			/*else if (numDistances<1) {
				numDistances = 1;
				codeLengths[numChars]=1;
				//codeLengths[numChars+1]=1;
				//codeLengths[1]=1;
			}*/
			distances.Init(&codeLengths[numChars], numDistances);

			for (i=0; i<19; i++)
				counts[i] = 0;
			unsigned int temp = numDistances+numChars;
			for (i=0; i<temp; i++) {
				if (codeLengths[i]==0) {
					int matchLen = 1;
					for (j=i+1; j<temp; j++) {
						if (codeLengths[j]==0)
							matchLen++;
						else
							break;
					}
					if (matchLen >= 3) {
						if (matchLen>=11) {
							if(matchLen>138) matchLen = 138; 
							counts[18]++;
						}
						else {
							counts[17]++;
						}
						i+=matchLen-1;
					}
					else {
						counts[0]++;
					}
				}
				else if (i && i<temp-2 &&
						 codeLengths[i] == codeLengths[i-1] &&
						 codeLengths[i] == codeLengths[i+1] &&
						 codeLengths[i] == codeLengths[i+2]) {
					int matchLen = 3;
					for (j=i+3; j<temp && matchLen<6; j++) {
						if (codeLengths[j]==codeLengths[i])
							matchLen++;
						else
							break;
					}
					counts[16]++;
					i+=matchLen-1;
				}
				else counts[codeLengths[i]]++;
			}


			unsigned int numLengths = 19;
			GenLengths(counts, &codeLengths[330], numLengths, 7);
			if (numLengths==-1) {
				for (i=0;i<19;i++) {
					codeLengths[330+lengthOrder[i]] = 4;
				}
				codeLengths[330+18] = 5;
				codeLengths[330+17] = 5;
				codeLengths[330+16] = 5;
				codeLengths[330+15] = 5;
				codeLengths[330+14] = 5;
				codeLengths[330+13] = 5;
			}
			lengths.Init(&codeLengths[330], 19);
			numLengths = 4;
			for (i=4; i<19; i++) {
				if (lengths.codes[lengthOrder[i]].length) numLengths = i+1;
			}
			//numLengths = 19;

			WriteBytes(5, numChars-257, &buffer);
			WriteBytes(5, numDistances-1, &buffer);
			WriteBytes(4, numLengths-4, &buffer);

			for (i=0; i<numLengths; i++) {
				WriteBytes(3, lengths.codes[lengthOrder[i]].length, &buffer);
			}

			for (i=0; i<temp; i++) {
				if (codeLengths[i]==0) {
					int matchLen = 1;
					for (j=i+1; j<temp; j++) {
						if (codeLengths[j]==0)
							matchLen++;
						else
							break;
					}
					if (matchLen >= 3) {
						if (matchLen>=11) {
							if (matchLen>138) matchLen = 138;
							WriteBytes(lengths.codes[18].length, lengths.codes[18].code, &buffer);
							WriteBytes(7, matchLen-11, &buffer);
						}
						else {
							WriteBytes(lengths.codes[17].length, lengths.codes[17].code, &buffer);
							WriteBytes(3, matchLen-3, &buffer);
						}
						i+=matchLen-1;
					}
					else {
						WriteBytes(lengths.codes[0].length, lengths.codes[0].code, &buffer);
						//counts[0]++;
					}
				}
				else if (i && i<temp-2 &&
						 codeLengths[i] == codeLengths[i-1] &&
						 codeLengths[i] == codeLengths[i+1] &&
						 codeLengths[i] == codeLengths[i+2]) {
					//WriteBytes(lengths.codes[codeLengths[i]].length, lengths.codes[codeLengths[i]].code, ndata, pos, offset, compressed, size);
					int matchLen = 3;
					for (j=i+3; j<temp && matchLen<6; j++) {
						if (codeLengths[j]==codeLengths[i])
							matchLen++;
						else
							break;
					}
					WriteBytes(lengths.codes[16].length, lengths.codes[16].code, &buffer);
					WriteBytes(2, matchLen-3, &buffer);

					i+=matchLen-1;
				}
				else {
					WriteBytes(lengths.codes[codeLengths[i]].length, lengths.codes[codeLengths[i]].code, &buffer);
				}
			}

		}
		else {
			for (i=0; i<blockLen; i++) {
				s1 = (s1 + pos[i]) % BASE;
				s2 = (s2 + s1) % BASE;
			}
			for (i=0; i<144; i++)
				codeLengths[i] = 8;
			for (   ; i<256; i++)
				codeLengths[i] = 9;
			for (   ; i<280; i++)
				codeLengths[i] = 7;
			for (   ; i<288; i++)
				codeLengths[i] = 8;
			numChars = 288;
			characters.Init(codeLengths, numChars);

			for (i=0; i<30; i++)
				codeLengths[i] = 5;
			distances.Init(codeLengths, i);
		}


		jump = jumps;
		unsigned char * finalPos = pos+blockLen;
		while (pos<finalPos) {
			/*if (jump->pos == pos && remainingLength!=compressedBytes) {
				jump++;
			}*/
			if (jump->pos == pos) {
				/*if (jumps[currentJump].len==258)
					code = 28;
				else
					code = LookupCode(lengthTable, jumps[currentJump].len);*/
				code = jump->lenCode;
				//WriteBytes(characters.codes[code+257].length, characters.codes[code+257].code, &buffer);
				//WriteBytes(lengthTable[code].bits, jump->len - lengthTable[code].lengths, &buffer);
				WriteBytes(characters.codes[code+257].length + lengthTable[code].bits,
					characters.codes[code+257].code + ((jump->len - lengthTable[code].lengths)<<characters.codes[code+257].length), &buffer);

				//code = LookupCode(distTable, jumps[currentJump].distance);
				code = jump->distCode;
				//WriteBytes(distances.codes[code].length, distances.codes[code].code, &buffer);
				//WriteBytes(distTable[code].bits, jump->distance - distTable[code].lengths, &buffer);
				WriteBytes(distances.codes[code].length + distTable[code].bits,
					distances.codes[code].code + ((jump->distance - distTable[code].lengths)<<distances.codes[code].length), &buffer);
				//*/
				/*WriteBytes(8, 0xA3, ndata, pos, offset, compressed, size);
				WriteBytes(5, 0x0, ndata, pos, offset, compressed, size);*/

				//pos2+=jump->len;
				pos+=jump->len;
				jump++;
			}
			else {
				// In retrospect, this doesn't seem to help much
				unsigned char* finalPosSub = finalPos;
				if (finalPosSub>jump->pos) finalPosSub = jump->pos;
				//int test = pos2;
				//pos=pos;
				for (;pos<finalPosSub;) {
					unsigned int length3, code;
					//unsigned int finalValue = end;
					//unsigned char* pos2;
					__asm {
						push ebx
						mov edi, pos

						//mov ecx, 0
						//mov edx, 0
						xor ecx, ecx
						movzx eax, [edi] //uncompressed[pos2]
						mov ebx, 0

						mov esi, [characters+8*eax] // length
						mov eax, [characters+8*eax+4] // code

						mov edx, [edi+1] //uncompressed[pos2]
next:
						and edx, 0xFF
						inc edi
						//sub esi, ecx
						shl eax, cl
						//add ecx, esi
						add ebx, eax


						mov ecx, esi


						//mov eax, [characters+8*esi+4] // code
						cmp edi, finalPosSub
						//mov esi, [characters+8*eax] // length
						//mov esi, ecx // length
						je stop


						add esi, [characters+8*edx]
						mov eax, [characters+8*edx+4] // code
						// 32 doesn't work because shift only uses the first 31 bits
						mov edx, [edi+1] //uncompressed[pos2]
						cmp esi, 32
						jl next
stop:
						mov code, ebx
						mov length3, ecx
						//mov code, edx
						mov pos, edi
						pop ebx
					}
					WriteBytes(length3, code, &buffer);
				}
			}
		}
		remainingLength-= blockLen;
		//startPos += blockLen;
		WriteBytes(characters.codes[256].length, characters.codes[256].code, &buffer);
	}
	WriteBytesForce(0, 0, &buffer);
	if (buffer.offset) {
		WriteBytesForce(8-buffer.offset, 0, &buffer);
	}
	unsigned int adler32 = (s2 << 16) + s1;
	bfinal = FlipLong((unsigned char*)&adler32);
	WriteBytesForce(32, bfinal, &buffer);
	free(jumps);


	compressedBytes = (int)(buffer.pos-buffer.data);
	return buffer.data;
}




unsigned char * DeflateSSE2Old(unsigned char* uncompressed, unsigned int uncompressedBytes, unsigned int &compressedBytes, unsigned int lineLength) {
	__declspec(align(16)) int table[16384][2];
	__declspec(align(16)) unsigned int counts[336];
	__declspec(align(16)) unsigned int distCheck[4] = {35000, 35000, 35000, 35000};
	//unsigned int s1=1, s2=0;
	//Tree trees[600];
	WriteBuffer buffer;
	memset(&buffer, 0, sizeof(buffer));

	buffer.size = uncompressedBytes*2+400;
	buffer.pos = buffer.data = (unsigned char*) malloc(sizeof(unsigned char)*(buffer.size));
	WriteBytes(16, 0x9C78, &buffer);
	//compressed[0] = 0x78;
	//compressed[1] = 0x9C;


	//unsigned int pos2=0;
	unsigned char *pos = uncompressed;
	unsigned int i, j;
	//unsigned int dist;

	unsigned int bfinal = 0;
	unsigned int type;
	unsigned int codeLengths[360];
	HuffmanCode2 lengths, characters, distances;

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

	//unsigned int len;
	unsigned int lengthOrder[19] = {16, 17, 18,
		0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	ExtendedCode codeLengthTable[2] = {{3,3}, {11,7}};
	unsigned int blockLen;

	Jump *jumps = (Jump*) malloc(sizeof(Jump)*65);
	unsigned int numJumps;
	unsigned int maxNumJumps = 64;
	unsigned int code;

	unsigned int t1, t2;
	__asm {
		mov t1, BASE
		mov ecx, uncompressed
		mov esi, uncompressedBytes
		dec ecx
		mov eax, 1
		xor edx, edx
		xor edi, edi
		add ecx, esi
		neg esi

alderLoop:
		inc esi
		mov dl, [esi+ecx]
		add eax, edx
		add edi, eax
		test esi, 255
		jnz alderLoop
		xor edx, edx
		div dword ptr[t1]
		mov eax, edi
		mov edi, edx
		xor edx, edx
		div dword ptr[t1]
		mov eax, edi
		mov edi, edx
		xor edx, edx
		test esi, esi
		jnz alderLoop
		mov t1, eax
		mov t2, edi
	}

	type = 0;
	//unsigned int i;
	//unsigned int distCheck[11] = {2, 1, 3, lineLength-9, lineLength-6, lineLength-3, lineLength, lineLength+3, lineLength+6, lineLength+9, uncompressedBytes+100};
	{
		__declspec(align(16)) unsigned int temp[4] = {-35000, -35000, -35000, -35000};
		__asm{
			movdqa xmm0, temp
			mov eax, 0x20000
			//lea edi, [table]
cleantemp:
			sub eax, 0x40
			movdqa [table+eax+0x30], xmm0
			movdqa [table+eax+0x20], xmm0
			movdqa [table+eax+0x10], xmm0
			movdqa [table+eax], xmm0
			jne cleantemp
		}
	}
	//memset(table, -1, sizeof(table));
	int remainingLength = uncompressedBytes;
	int startPos = 0;

	
	//unsigned int nextHash = QuickHash(((int*)pos)[0]);


	while (!bfinal) {
		//if (uncompressedBytes-pos2 <= 32768)
		blockLen = remainingLength;
		if (0 && blockLen>lineLength*40) blockLen = lineLength*40;
		else bfinal = 1;
		//	for (i=0; i<blockLen; i++) {
		//		unsigned char *pos2 = pos+i;
		//		s1 = (s1 + pos2[0]) % BASE;
		//		s2 = (s2 + s1)     % BASE;
		//	}
		unsigned char* blockend = pos+blockLen;
		if (0&&uncompressedBytes<250) {
			//type = 2;
			type = 1;
			//blockLen = 0;
		}
		else {
			//bfinal = 0;
			//type = 2;
			//type = 1;
			//bfinal = 1;
			//type = 2;
			type = 2;
		}
		WriteBytes(3, (type<<1) + bfinal, &buffer);
		unsigned int safetyEnd = remainingLength-8;
		/*WriteBytes(5, 0, ndata, pos, offset, compressed, size);
		unsigned int test = Flip((unsigned char*)&uncompressedBytes);
		WriteBytes(16, test, ndata, pos, offset, compressed, size);
		WriteBytes(16, test ^ 0xFFFFFFFF, ndata, pos, offset, compressed, size);
		memcpy(&compressed[7], uncompressed, uncompressedBytes);
		pos+= uncompressedBytes;
		break;*/
		numJumps = 0;


		unsigned int numChars;

		if (type==2) {
			//HashEntry *table[32768];
			//for (i=0; i<330; i++)
			//	counts[i]=0;


			__asm{
				pxor xmm0, xmm0
				mov eax, 0x540
				//lea edi, [counts]
		cleancounts:
				sub eax, 0x40
				movdqa [counts+eax+0x30], xmm0
				movdqa [counts+eax+0x20], xmm0
				movdqa [counts+eax+0x10], xmm0
				movdqa [counts+eax], xmm0
				jne cleancounts
			}


			/*for (i=0; i<blockLen; i++) {
				unsigned int matched = 0;
				unsigned int matchedDist = 0;
				unsigned char *pos2 = pos+i;
				HashEntry *temp;
				unsigned int hashVal = hash(((unsigned int*)pos)[0])&0x7FFF;
				temp = table[hashVal];
				table[hashVal] = (HashEntry*) malloc(sizeof(HashEntry));
				table[hashVal]->next = temp;
				table[hashVal]->value = i;
				while (temp) {
					unsigned int dist = i - temp->value;
					if (dist<i) break;
					//for (j=0;(dist=distCheck[j])<i;j++) {
					//for (dist=lineLength-9; dist<pos2+i && dist<32000; dist+=3) {
					for (len=0; len<=257 && len+pos2+4<blockend; len+=4) {
						if (((int*)&pos2[len])[0] !=
							((int*)&pos2[len-dist])[0]) break;
					}
					if (len) {
						len-=4;
						for (; len<=257 && len+pos2<blockend; len++) {
							if (pos2[len] !=
								pos2[len-dist]) break;
						}
						//len--;
						if (len>matched) {
							matched = len;
							matchedDist = dist;
							if (matched >= 258) {
								matched = 258;
								break;
							}
						}
					}

					temp = temp->next;
				}


					
				if (matched<9) {
					counts[pos2[0]]++;
				}
				else {
					if (maxNumJumps == numJumps) {
						jumps = (Jump*) realloc(jumps, sizeof(Jump)*(1+(maxNumJumps*=2)));
					}
					jumps[numJumps].pos = pos2;
					jumps[numJumps].distance = matchedDist;
					jumps[numJumps].len = matched;
					if (matched == 258)
						jumps[numJumps].lenCode = 28;
					else
						jumps[numJumps].lenCode = LookupCode(lengthTable, jumps[numJumps].len);
					jumps[numJumps].distCode= LookupCode(distTable, matchedDist);
					counts[jumps[numJumps].lenCode+257]++;
					counts[jumps[numJumps].distCode+287]++;
					numJumps++;
					i+=matched-1;
				}
			}
			//continue;*/

			for (i=0; i<blockLen; i++) {
				unsigned int matched = 8;
				unsigned int matchedDist;
				unsigned char *pos2;// = pos+i;
				//int s3 = s1, s4 = s2;
				//s1 = (s1 + pos2[0]) % BASE;
				//s2 = (s2 + s1)     % BASE;
				//pos2[0] = 1;
				//s1=BASE-1;
				//s2= BASE-1;
				__asm {
					mov edi, pos
					mov eax, i
					add edi, eax
					/*
					mov ecx, s1
					movzx eax, [edi+esi]


					mov edx, s2
					add ecx, eax
					add edx, ecx

					mov eax, ecx
					mov esi, edx

					sub eax, BASE
					mov pos2, edi
					cmovge ecx, eax

					sub esi, BASE
					mov s1, ecx
					cmovge edx, esi
					mov s2, edx*/
					//mov eax, i

					cmp eax, safetyEnd

					jge shipHashUpdate
				//}


				//if (i<safetyEnd) {
				//	unsigned int hash;
				//	__asm {
				//		mov edi, pos2
						prefetcht0 [edi+0x10]
						mov eax, [edi]
						mov esi, [edi+4]

						mov ecx, eax
						mov edx, eax

						rol eax, 3
						ror ecx, 13
						ror edx, 25

						sub eax, ecx
						mov ecx, esi
						sar esi, 15
						xor eax, edx
						sub ecx, esi
						mov edi, i

						xor eax, ecx
						add edi, startPos
						//mov key, eax




						and eax, 0x3FFF
						mov esi, edi
						mov ecx, dword ptr table[eax*8]
						mov edx, edi

						sub edi, ecx
						//sub edi, dword ptr table[eax*8]
						sub esi, dword ptr table[eax*8+4]
						cmp edi, 0x8000

						mov dword ptr table[eax*8], edx
						mov dword ptr table[eax*8+4], ecx
						//mov [distCheck], edi
						mov [distCheck+4], esi

						//prefetcht0 [edi]
						//prefetcht0 [esi]
						//xor ecx, ecx
shipHashUpdate:
					//}
					/*hash = (QuickHash(((int*)pos2)[0]) ^ (((int*)pos2)[1]-(((int*)pos2)[1]>>15)))&0x3FFF;
					//if (((int*)table)[hash]>=0 &&i-table[hash]<32000) {
						//FILE * temp=fopen("temp.txt", "ab");
						//fprintf(temp, "%i\t%i\t%i\n", i, table[hash], i-table[hash]);
						//fclose(temp);

					distCheck[0]= startPos+i-table[hash][0];
					distCheck[1]= startPos+i-table[hash][1];
					table[hash][1] = table[hash][0];
					//}
					table[hash][0] =i+startPos;
					*/
				//}

				/*
				unsigned int aux[4];
				__asm {
					mov edi, pos
					lea esi, distCheck
					//mov edx, [esi]
					mov ecx, pos
					add edi, i
					//mov eax, [esi]
					pxor xmm0, xmm0
					//neg eax
					mov edx, edi
					movdqu xmm1, [edi]
quickCheck:
					sub edx, [esi]
					//neg edx
					add esi, 4
					//add edx, edi
					cmp edx, ecx
					jl endCheck
					movdqu xmm2, [edx]
					pcmpeqb xmm2, xmm1
					mov edx, edi
					por xmm0, xmm2
					jmp quickCheck
endCheck:
					//movdqu [aux], xmm0
					movdqu aux, xmm0
				}//*/
				/*
				int nuke = 0;
				for (j=15; ((int*)&j)[0]>=0; j--) {
					if (nuke) {
						counts[pos[++i]]++;
						s1 = (s1 + pos[i]) % BASE;
						s2 = (s2 + s1)     % BASE;
					}
					else if (((char*)aux)[j]==0) {
						nuke=1;
						counts[pos[i]]++;
					}
				}
				// not sure about this
				if (nuke==0) {*/
				//unsigned int tempEnd;
				//unsigned int testLen = 8;
				//unsigned int testDist = 0;
				//__asm {
					// dist
					//mov edi, distCheck
					jge skipSearch
					mov j, 0
					mov esi, pos2
startSearch:

					// len

					mov edx, blockend
					// pos2+len-dist
					mov ecx, 257
					sub edx, 4
					//sub edx, eax

					xor eax, eax
					cmp edx, ecx
					// max len
					cmovl ecx, edx
					sub esi, edi

					mov edx, [esi]
					cmp edx, [esi+edi]
startQuadMatch:
					jne endQuadMatch

					cmp eax, ecx
					jg endQuadMatch

					add esi, 4
					add eax, 4
					mov edx, [esi]
					cmp edx, [esi+edi]
					jmp startQuadMatch

endQuadMatch:
					cmp eax, matched
					jle tryNext
					add ecx, 3
					sub eax, 4
					sub esi, 4

					mov edx, 257
					cmp ecx, 257
					cmovge ecx, edx

startSingleMatch:
					cmp eax, ecx
					jg endSingleMatch

					mov dl, [esi]
					cmp dl, [esi+edi]
					jne endSingleMatch
					inc eax
					inc esi
					jmp startSingleMatch
endSingleMatch:
					cmp eax, matched
					jle tryNext
					mov matched, eax
					mov matchedDist, edi
tryNext:
					mov ecx, j
					mov edi, [distCheck+4*ecx+4]
					inc ecx
					cmp edi, 0x8000
					mov esi, pos2
					mov j, ecx
					jl startSearch
skipSearch:
				}
					/*for (j=0;(dist=distCheck[j])<32767;j++) {
					//for (j=0;(dist=distCheck[j])<=i+startPos && dist<32767;j++) {
					//for (dist=lineLength-9; dist<pos2+i && dist<32000; dist+=3) {
						for (len=0; len<=257 && len+pos2+3<blockend; len+=4) {
							if (((int*)&pos2[len])[0] !=
								((int*)&pos2[len-dist])[0]) break;
						}
						if (len>matched) {
							len-=4;
							for (; len<=257 && len+pos2<blockend; len++) {
								if (pos2[len] !=
									pos2[len-dist]) break;
							}
							//len--;
							if (len>matched) {
								matched = len;
								matchedDist = dist;
								if (matched >= 258) {
									matched = 258;
									break;
								}
							}
						}
					}*/

					
					
					
				if (matched<9) {
					/*while (matched) {
						counts[uncompressed[pos3]]++;
						i++;
						matched--;
					}*/
					counts[pos2[0]]++;
				}
				else {
					if (maxNumJumps == numJumps) {
						jumps = (Jump*) realloc(jumps, sizeof(Jump)*(1+(maxNumJumps*=2)));
					}
					jumps[numJumps].pos = pos2;
					jumps[numJumps].distance = matchedDist;
					jumps[numJumps].len = matched;
					if (matched == 258)
						jumps[numJumps].lenCode = 28;
					else
						jumps[numJumps].lenCode = LookupCode(lengthTable, jumps[numJumps].len);
					jumps[numJumps].distCode= LookupCode(distTable, matchedDist);
					counts[jumps[numJumps].lenCode+257]++;
					counts[jumps[numJumps].distCode+287]++;
					numJumps++;
					j=i+matched-1;
					//i++;
					if (j<safetyEnd) {
						//while(i<j) {
							__asm {
								mov eax, i
hashLoop:
								mov edi, pos
								inc eax
								add edi, eax
								mov i, eax
								/*
								mov ecx, s1
								movzx esi, [edi+eax]
								//inc eax
								//add edi, eax
								add ecx, esi

								mov edx, ecx
								mov esi, -BASE
								add edx, s2
								//add edx, ecx

								//mov eax, ecx

								add esi, ecx
								mov eax, edx
								cmovge ecx, esi

								sub eax, BASE
								mov s1, ecx
								cmovge edx, eax

								mov s2, edx*/

							


								//mov edi, pos2
								mov eax, [edi]
								mov esi, [edi+4]

								mov ecx, eax
								mov edx, eax

								rol eax, 3
								ror ecx, 13
								ror edx, 25

								sub eax, ecx
								//mov key, eax


								mov ecx, esi
								sar esi, 15
								xor edx, eax
								mov eax, i
								sub ecx, esi

								mov esi, startPos
								//mov edi, j


								xor edx, ecx
								add esi, eax
								and edx, 0x3FFF
								cmp eax, j
								mov ecx, dword ptr table[edx*8]

								mov dword ptr table[edx*8], esi
								mov dword ptr table[edx*8+4], ecx
								jl hashLoop
								//xor ecx, ecx
							}
							//s1 = (s1 + pos[i]) % BASE;
							//s2 = (s2 + s1)     % BASE;
							//unsigned int hash = (QuickHash(((int*)(&pos[i]))[0]) ^ (((int*)(&pos[i]))[1]-(((int*)(&pos[i]))[1]>>15)))&0x3FFF;
							//if (((int*)table)[hash]>=0 &&i-table[hash]<32000) {
								//FILE * temp=fopen("temp.txt", "ab");
								//fprintf(temp, "%i\t%i\t%i\n", i, table[hash], i-table[hash]);
								//fclose(temp);

							//	distCheck[0]= i-table[hash];
							//}
							//table[hash] =i;
							//table[hash][1] = table[hash][0];
							//}
							//table[hash][0] =startPos+i;
						//}
					}
					/*else {
						while(i<j) {
							__asm {
								mov eax, i
								mov edi, pos
								inc eax
								mov ecx, s1
								mov i, eax
								add edi, eax
								mov edx, s2

								movzx eax, [edi]

								add ecx, eax
								add edx, ecx

								mov eax, ecx
								mov esi, edx

								sub eax, BASE
								cmovge ecx, eax

								sub esi, BASE
								mov s1, ecx
								cmovge edx, esi

								mov s2, edx
							}
						}
					}*/
					//i+=matched-1;
					//break;
				}
				//}
				/*if (matched<9) {
					counts[pos2[0]]++;
				}
				else {*/
				//}
			}
			jumps[numJumps].pos = pos+blockLen+10;
			counts[256] = 1;
			numChars = 286;
			GenLengths(counts, codeLengths, numChars, 15);
			if (numChars==-1) {
				numChars = 286;
				for (i=0; i<144; i++)
					codeLengths[i] = 8;
				for (   ; i<256; i++)
					codeLengths[i] = 9;
				for (   ; i<282; i++)
					codeLengths[i] = 7;
				for (   ; i<286; i++)
					codeLengths[i] = 8;
				for (   ; i<300; i++)
					codeLengths[i] = 8;
			}
			characters.Init(codeLengths, numChars);
			if (numChars<257) {
				// should never get here, but you never know
				numChars = 257;
			}

			unsigned int numDistances = 30;
			GenLengths(&counts[287], &codeLengths[numChars], numDistances, 15);
			if (numDistances==-1) {
				numDistances = 30;
				for (i=0; i<30; i++)
					codeLengths[numChars+i] = 5;
				codeLengths[numChars+2] = 4;
				codeLengths[numChars+3] = 4;
			}
			else if (numDistances<1) {
				numDistances = 1;
				codeLengths[numChars]=1;
				//codeLengths[numChars+1]=1;
				//codeLengths[1]=1;
			}
			distances.Init(&codeLengths[numChars], numDistances);
	/*if (1) {
		compressedBytes=1;
		return (unsigned char*)malloc(1);
	}//*/

			for (i=0; i<19; i++)
				counts[i] = 0;
			unsigned int temp = numDistances+numChars;
			for (i=0; i<temp; i++) {
				if (codeLengths[i]==0) {
					int matchLen = 1;
					for (j=i+1; j<temp; j++) {
						if (codeLengths[j]==0)
							matchLen++;
						else
							break;
					}
					if (matchLen >= 3) {
						if (matchLen>=11) {
							if(matchLen>138) matchLen = 138; 
							counts[18]++;
						}
						else {
							counts[17]++;
						}
						i+=matchLen-1;
					}
					else {
						counts[0]++;
					}
				}
				else if (i && i<temp-2 &&
						 codeLengths[i] == codeLengths[i-1] &&
						 codeLengths[i] == codeLengths[i+1] &&
						 codeLengths[i] == codeLengths[i+2]) {
					int matchLen = 3;
					for (j=i+3; j<temp && matchLen<6; j++) {
						if (codeLengths[j]==codeLengths[i])
							matchLen++;
						else
							break;
					}
					counts[16]++;
					i+=matchLen-1;
				}
				else counts[codeLengths[i]]++;
			}


			unsigned int numLengths = 19;
			GenLengths(counts, &codeLengths[330], numLengths, 7);
			if (numLengths==-1) {
				for (i=0;i<19;i++) {
					codeLengths[330+lengthOrder[i]] = 4;
				}
				codeLengths[330+18] = 5;
				codeLengths[330+17] = 5;
				codeLengths[330+16] = 5;
				codeLengths[330+15] = 5;
				codeLengths[330+14] = 5;
				codeLengths[330+13] = 5;
			}
			lengths.Init(&codeLengths[330], 19);
			numLengths = 4;
			for (i=4; i<19; i++) {
				if (lengths.codes[lengthOrder[i]].length) numLengths = i+1;
			}
			//numLengths = 19;

			WriteBytes(5, numChars-257, &buffer);
			WriteBytes(5, numDistances-1, &buffer);
			WriteBytes(4, numLengths-4, &buffer);

			for (i=0; i<numLengths; i++) {
				WriteBytes(3, lengths.codes[lengthOrder[i]].length, &buffer);
			}

			for (i=0; i<temp; i++) {
				if (codeLengths[i]==0) {
					int matchLen = 1;
					for (j=i+1; j<temp; j++) {
						if (codeLengths[j]==0)
							matchLen++;
						else
							break;
					}
					if (matchLen >= 3) {
						if (matchLen>=11) {
							if (matchLen>138) matchLen = 138;
							WriteBytes(lengths.codes[18].length, lengths.codes[18].code, &buffer);
							WriteBytes(7, matchLen-11, &buffer);
						}
						else {
							WriteBytes(lengths.codes[17].length, lengths.codes[17].code, &buffer);
							WriteBytes(3, matchLen-3, &buffer);
						}
						i+=matchLen-1;
					}
					else {
						WriteBytes(lengths.codes[0].length, lengths.codes[0].code, &buffer);
						//counts[0]++;
					}
				}
				else if (i && i<temp-2 &&
						 codeLengths[i] == codeLengths[i-1] &&
						 codeLengths[i] == codeLengths[i+1] &&
						 codeLengths[i] == codeLengths[i+2]) {
					//WriteBytes(lengths.codes[codeLengths[i]].length, lengths.codes[codeLengths[i]].code, ndata, pos, offset, compressed, size);
					int matchLen = 3;
					for (j=i+3; j<temp && matchLen<6; j++) {
						if (codeLengths[j]==codeLengths[i])
							matchLen++;
						else
							break;
					}
					WriteBytes(lengths.codes[16].length, lengths.codes[16].code, &buffer);
					WriteBytes(2, matchLen-3, &buffer);

					i+=matchLen-1;
				}
				else {
					WriteBytes(lengths.codes[codeLengths[i]].length, lengths.codes[codeLengths[i]].code, &buffer);
				}
			}
			
			/*i=i;
			for (i=0; i<temp; i++) {
				code = codeLengths[i];
				WriteBytes(lengths.codes[code].length, lengths.codes[code].code, ndata, pos, offset, compressed, size);
			}
			i=i;

			/*for (i=0; i<numDistances; i++) {
				code = distances.codes[i].length;
				WriteBytes(lengths.codes[code].length, lengths.codes[code].code, ndata, pos, offset, compressed, size);
			}//*/
				//WriteBytes(32, 0x00FF00FF, ndata, pos, offset, compressed, size);
			//i=i;

		}
		else {
			/*for (i=0; i<blockLen; i++) {
				s1 = (s1 + pos[i]) % BASE;
				s2 = (s2 + s1) % BASE;
			}*/
			for (i=0; i<144; i++)
				codeLengths[i] = 8;
			for (   ; i<256; i++)
				codeLengths[i] = 9;
			for (   ; i<280; i++)
				codeLengths[i] = 7;
			for (   ; i<288; i++)
				codeLengths[i] = 8;
			numChars = 288;
			characters.Init(codeLengths, numChars);

			for (i=0; i<30; i++)
				codeLengths[i] = 5;
			distances.Init(codeLengths, i);
		}


		Jump *jump = jumps;
		unsigned char * finalPos = pos+blockLen;
		while (pos<finalPos) {
			if (jump->pos == pos) {
				/*if (jumps[currentJump].len==258)
					code = 28;
				else
					code = LookupCode(lengthTable, jumps[currentJump].len);*/
				code = jump->lenCode;
				//WriteBytes(characters.codes[code+257].length, characters.codes[code+257].code, &buffer);
				//WriteBytes(lengthTable[code].bits, jump->len - lengthTable[code].lengths, &buffer);
				WriteBytes(characters.codes[code+257].length + lengthTable[code].bits,
					characters.codes[code+257].code + ((jump->len - lengthTable[code].lengths)<<characters.codes[code+257].length), &buffer);

				//code = LookupCode(distTable, jumps[currentJump].distance);
				code = jump->distCode;
				//WriteBytes(distances.codes[code].length, distances.codes[code].code, &buffer);
				//WriteBytes(distTable[code].bits, jump->distance - distTable[code].lengths, &buffer);
				WriteBytes(distances.codes[code].length + distTable[code].bits,
					distances.codes[code].code + ((jump->distance - distTable[code].lengths)<<distances.codes[code].length), &buffer);
				//*/
				/*WriteBytes(8, 0xA3, ndata, pos, offset, compressed, size);
				WriteBytes(5, 0x0, ndata, pos, offset, compressed, size);*/

				//pos2+=jump->len;
				pos+=jump->len;
				jump++;
			}
			else {
				// In retrospect, this doesn't seem to help much
				unsigned char* finalPosSub = finalPos;
				if (finalPosSub>jump->pos) finalPosSub = jump->pos;
				//int test = pos2;
				pos=pos;
				for (;pos<finalPosSub;) {
					//unsigned int c = pos[0];
					//unsigned int length=characters.codes[c].length, code=characters.codes[c].code;
					//unsigned int w = characters.codes[c].length;
					/*while (pos2!=end) {
						code = code + (characters.codes[c].code<<length);
						length += characters.codes[c].length;
						pos2++;
						c = uncompressed[pos2];
						if (length+characters.codes[c].length>32) break;
					}*/
					//WriteBytes(characters.codes[c].length, characters.codes[c].code, &buffer);
					unsigned int length3, code;
					//unsigned int finalValue = end;
					//unsigned char* pos2;
					__asm {
						mov edi, pos

						//mov ecx, 0
						//mov edx, 0
						xor ecx, ecx
						movzx eax, [edi] //uncompressed[pos2]
						xor edx, edx

						mov esi, [characters+8*eax] // length
						mov eax, [characters+8*eax+4] // code
next:
						sub esi, ecx
						shl eax, cl
						inc edi
						add ecx, esi
						add edx, eax


						//mov eax, [characters+8*esi+4] // code
						cmp edi, finalPosSub
						//mov esi, [characters+8*eax] // length
						movzx eax, [edi] //uncompressed[pos2]
						mov esi, ecx // length
						je stop


						add esi, [characters+8*eax]
						mov eax, [characters+8*eax+4] // code
						// 32 doesn't work because shift only uses the first 31 bits
						cmp esi, 32
						jl next
stop:
						mov length3, ecx
						mov code, edx
						mov pos, edi
					}
					WriteBytes(length3, code, &buffer);
				}
			}
		}
		remainingLength-= blockLen;
		startPos += blockLen;
		WriteBytes(characters.codes[256].length, characters.codes[256].code, &buffer);
	}
	WriteBytesForce(0, 0, &buffer);
	if (buffer.offset) {
		WriteBytesForce(8-buffer.offset, 0, &buffer);
	}
	unsigned int adler32 = (t2 << 16) + t1;
	bfinal = FlipLong((unsigned char*)&adler32);
	WriteBytesForce(32, bfinal, &buffer);
	free(jumps);


	compressedBytes = (int)(buffer.pos-buffer.data);
	return buffer.data;
}








unsigned char * Deflate(unsigned char* uncompressed, unsigned int uncompressedBytes, unsigned int &compressedBytes, unsigned int lineLength) {
	unsigned int s1=1, s2=0;
	unsigned int counts[330];
	//Tree trees[600];
	WriteBuffer buffer;
	memset(&buffer, 0, sizeof(buffer));

	buffer.size = uncompressedBytes*2+400;
	buffer.pos = buffer.data = (unsigned char*) malloc(sizeof(unsigned char)*(buffer.size));
	WriteBytes(16, 0x9C78, &buffer);
	//compressed[0] = 0x78;
	//compressed[1] = 0x9C;


	//unsigned int pos2=0;
	unsigned char *pos = uncompressed;

	unsigned int bfinal = 0;
	unsigned int type;
	unsigned int i, j;
	unsigned int dist;
	unsigned int codeLengths[360];
	HuffmanCode2 lengths, characters, distances;

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
	unsigned int blockLen;

	Jump *jumps = (Jump*) malloc(sizeof(Jump)*65);
	unsigned int numJumps;
	unsigned int maxNumJumps = 64;
	unsigned int code;

	type = 0;
	//unsigned int i;
	unsigned int distCheck[10] = {1, 3, lineLength-9, lineLength-6, lineLength-3, lineLength, lineLength+3, lineLength+6, lineLength+9, uncompressedBytes+100};
	while (!bfinal) {
		//if (uncompressedBytes-pos2 <= 32768)
		blockLen = uncompressedBytes;
		unsigned char* blockend = pos+blockLen;
		bfinal = 1;
		if (uncompressedBytes<250) {
			//type = 2;
			type = 1;
			//blockLen = 0;
		}
		else {
			//bfinal = 0;
			//type = 2;
			//type = 1;
			//bfinal = 1;
			//type = 2;
			type = 2;
		}
		WriteBytes(3, (type<<1) + bfinal, &buffer);
		/*WriteBytes(5, 0, ndata, pos, offset, compressed, size);
		unsigned int test = Flip((unsigned char*)&uncompressedBytes);
		WriteBytes(16, test, ndata, pos, offset, compressed, size);
		WriteBytes(16, test ^ 0xFFFFFFFF, ndata, pos, offset, compressed, size);
		memcpy(&compressed[7], uncompressed, uncompressedBytes);
		pos+= uncompressedBytes;
		break;*/
		numJumps = 0;


		unsigned int numChars;
		if (type==2) {
			for (i=0; i<330; i++)
				counts[i]=0;

			for (i=0; i<blockLen; i++) {
				unsigned char *pos2 = pos+i;
				s1 = (s1 + pos2[0]) % BASE;
				s2 = (s2 + s1)     % BASE;
			}

			for (i=0; i<blockLen; i++) {
				unsigned int matched = 0;
				unsigned int matchedDist = 0;
				unsigned char *pos2 = pos+i;
				//unsigned int pos3 = i + pos2;
				if (lineLength>20) {
					// not sure about this
					for (j=0;(dist=distCheck[j])<i;j++) {
					//for (dist=lineLength-9; dist<pos2+i && dist<32000; dist+=3) {
						for (len=0; len<=257 && len+pos2+4<blockend; len+=4) {
							if (((int*)&pos2[len])[0] !=
								((int*)&pos2[len-dist])[0]) break;
						}
						if (len) {
							len-=4;
							for (; len<=257 && len+pos2<blockend; len++) {
								if (pos2[len] !=
									pos2[len-dist]) break;
							}
							//len--;
							if (len>matched) {
								matched = len;
								matchedDist = dist;
								if (matched >= 258) {
									matched = 258;
									break;
								}
							}
						}
					/*	if (dist==3) dist = -2;
						else if (dist==lineLength+9) dist=0;
						else if (dist==1) break;
					}*/
					}
				}
				else {
					//Not sure if this should be i.  As I only make one block, dun matter
					for (dist=1; dist<i && dist<32000; dist++) {
						for (len=1; len<=258 && len+pos2<blockend; len++) {
							if (pos2[len-1] !=
								pos2[len-1-dist]) break;
						}
						len--;
						if (len>matched) {
							matched = len;
							matchedDist = dist;
						}
						if (matched == 258) break;
					}
				}
				if (matched<9) {
					/*while (matched) {
						counts[uncompressed[pos3]]++;
						i++;
						matched--;
					}*/
					counts[pos2[0]]++;
				}
				else {
					if (maxNumJumps == numJumps) {
						jumps = (Jump*) realloc(jumps, sizeof(Jump)*(1+(maxNumJumps*=2)));
					}
					jumps[numJumps].pos = pos2;
					jumps[numJumps].distance = matchedDist;
					jumps[numJumps].len = matched;
					if (matched == 258)
						jumps[numJumps].lenCode = 28;
					else
						jumps[numJumps].lenCode = LookupCode(lengthTable, jumps[numJumps].len);
					jumps[numJumps].distCode= LookupCode(distTable, matchedDist);
					counts[jumps[numJumps].lenCode+257]++;
					counts[jumps[numJumps].distCode+287]++;
					numJumps++;
					i+=matched-1;
					//break;
				}
			}
			jumps[numJumps].pos = pos+blockLen+10;
			counts[256] = 1;
			numChars = 286;
			GenLengths(counts, codeLengths, numChars, 15);
			if (numChars==-1) {
				numChars = 286;
				for (i=0; i<144; i++)
					codeLengths[i] = 8;
				for (   ; i<256; i++)
					codeLengths[i] = 9;
				for (   ; i<282; i++)
					codeLengths[i] = 7;
				for (   ; i<286; i++)
					codeLengths[i] = 8;
				for (   ; i<300; i++)
					codeLengths[i] = 8;
			}
			characters.Init(codeLengths, numChars);
			if (numChars<257) {
				// should never get here, but you never know
				numChars = 257;
			}

			unsigned int numDistances = 30;
			GenLengths(&counts[287], &codeLengths[numChars], numDistances, 15);
			if (numDistances==-1) {
				numDistances = 30;
				for (i=0; i<30; i++)
					codeLengths[numChars+i] = 5;
				codeLengths[numChars+2] = 4;
				codeLengths[numChars+3] = 4;
			}
			else if (numDistances<1) {
				numDistances = 1;
				codeLengths[numChars]=1;
				//codeLengths[numChars+1]=1;
				//codeLengths[1]=1;
			}
			distances.Init(&codeLengths[numChars], numDistances);
	/*if (1) {
		compressedBytes=1;
		return (unsigned char*)malloc(1);
	}//*/

			for (i=0; i<19; i++)
				counts[i] = 0;
			unsigned int temp = numDistances+numChars;
			for (i=0; i<temp; i++) {
				if (codeLengths[i]==0) {
					int matchLen = 1;
					for (j=i+1; j<temp; j++) {
						if (codeLengths[j]==0)
							matchLen++;
						else
							break;
					}
					if (matchLen >= 3) {
						if (matchLen>=11) {
							if(matchLen>138) matchLen = 138; 
							counts[18]++;
						}
						else {
							counts[17]++;
						}
						i+=matchLen-1;
					}
					else {
						counts[0]++;
					}
				}
				else if (i && i<temp-2 &&
						 codeLengths[i] == codeLengths[i-1] &&
						 codeLengths[i] == codeLengths[i+1] &&
						 codeLengths[i] == codeLengths[i+2]) {
					int matchLen = 3;
					for (j=i+3; j<temp && matchLen<6; j++) {
						if (codeLengths[j]==codeLengths[i])
							matchLen++;
						else
							break;
					}
					counts[16]++;
					i+=matchLen-1;
				}
				else counts[codeLengths[i]]++;
			}


			unsigned int numLengths = 19;
			GenLengths(counts, &codeLengths[330], numLengths, 7);
			if (numLengths==-1) {
				for (i=0;i<19;i++) {
					codeLengths[330+lengthOrder[i]] = 4;
				}
				codeLengths[330+18] = 5;
				codeLengths[330+17] = 5;
				codeLengths[330+16] = 5;
				codeLengths[330+15] = 5;
				codeLengths[330+14] = 5;
				codeLengths[330+13] = 5;
			}
			lengths.Init(&codeLengths[330], 19);
			numLengths = 4;
			for (i=4; i<19; i++) {
				if (lengths.codes[lengthOrder[i]].length) numLengths = i+1;
			}
			//numLengths = 19;

			WriteBytes(5, numChars-257, &buffer);
			WriteBytes(5, numDistances-1, &buffer);
			WriteBytes(4, numLengths-4, &buffer);

			for (i=0; i<numLengths; i++) {
				WriteBytes(3, lengths.codes[lengthOrder[i]].length, &buffer);
			}

			for (i=0; i<temp; i++) {
				if (codeLengths[i]==0) {
					int matchLen = 1;
					for (j=i+1; j<temp; j++) {
						if (codeLengths[j]==0)
							matchLen++;
						else
							break;
					}
					if (matchLen >= 3) {
						if (matchLen>=11) {
							if (matchLen>138) matchLen = 138;
							WriteBytes(lengths.codes[18].length, lengths.codes[18].code, &buffer);
							WriteBytes(7, matchLen-11, &buffer);
						}
						else {
							WriteBytes(lengths.codes[17].length, lengths.codes[17].code, &buffer);
							WriteBytes(3, matchLen-3, &buffer);
						}
						i+=matchLen-1;
					}
					else {
						WriteBytes(lengths.codes[0].length, lengths.codes[0].code, &buffer);
						//counts[0]++;
					}
				}
				else if (i && i<temp-2 &&
						 codeLengths[i] == codeLengths[i-1] &&
						 codeLengths[i] == codeLengths[i+1] &&
						 codeLengths[i] == codeLengths[i+2]) {
					//WriteBytes(lengths.codes[codeLengths[i]].length, lengths.codes[codeLengths[i]].code, ndata, pos, offset, compressed, size);
					int matchLen = 3;
					for (j=i+3; j<temp && matchLen<6; j++) {
						if (codeLengths[j]==codeLengths[i])
							matchLen++;
						else
							break;
					}
					WriteBytes(lengths.codes[16].length, lengths.codes[16].code, &buffer);
					WriteBytes(2, matchLen-3, &buffer);

					i+=matchLen-1;
				}
				else {
					WriteBytes(lengths.codes[codeLengths[i]].length, lengths.codes[codeLengths[i]].code, &buffer);
				}
			}
			
			/*i=i;
			for (i=0; i<temp; i++) {
				code = codeLengths[i];
				WriteBytes(lengths.codes[code].length, lengths.codes[code].code, ndata, pos, offset, compressed, size);
			}
			i=i;

			/*for (i=0; i<numDistances; i++) {
				code = distances.codes[i].length;
				WriteBytes(lengths.codes[code].length, lengths.codes[code].code, ndata, pos, offset, compressed, size);
			}//*/
				//WriteBytes(32, 0x00FF00FF, ndata, pos, offset, compressed, size);
			//i=i;

		}
		else {
			for (i=0; i<blockLen; i++) {
				s1 = (s1 + pos[i]) % BASE;
				s2 = (s2 + s1) % BASE;
			}
			for (i=0; i<144; i++)
				codeLengths[i] = 8;
			for (   ; i<256; i++)
				codeLengths[i] = 9;
			for (   ; i<280; i++)
				codeLengths[i] = 7;
			for (   ; i<288; i++)
				codeLengths[i] = 8;
			numChars = 288;
			characters.Init(codeLengths, numChars);

			for (i=0; i<30; i++)
				codeLengths[i] = 5;
			distances.Init(codeLengths, i);
		}


		Jump *jump = jumps;
		unsigned char * finalPos = pos+blockLen;
		while (pos<finalPos) {
			if (jump->pos == pos) {
				/*if (jumps[currentJump].len==258)
					code = 28;
				else
					code = LookupCode(lengthTable, jumps[currentJump].len);*/
				code = jump->lenCode;
				WriteBytes(characters.codes[code+257].length, characters.codes[code+257].code, &buffer);
				WriteBytes(lengthTable[code].bits, jump->len - lengthTable[code].lengths, &buffer);

				//code = LookupCode(distTable, jumps[currentJump].distance);
				code = jump->distCode;
				WriteBytes(distances.codes[code].length, distances.codes[code].code, &buffer);
				WriteBytes(distTable[code].bits, jump->distance - distTable[code].lengths, &buffer);
				//*/
				/*WriteBytes(8, 0xA3, ndata, pos, offset, compressed, size);
				WriteBytes(5, 0x0, ndata, pos, offset, compressed, size);*/

				//pos2+=jump->len;
				pos+=jump->len;
				jump++;
			}
			else {
				unsigned char c = pos++[0];
				WriteBytes(characters.codes[c].length, characters.codes[c].code, &buffer);
			}
		}
		WriteBytes(characters.codes[256].length, characters.codes[256].code, &buffer);
	}
	WriteBytesForce(0, 0, &buffer);
	if (buffer.offset) {
		WriteBytesForce(8-buffer.offset, 0, &buffer);
	}
	unsigned int adler32 = (s2 << 16) + s1;
	//bfinal = Adler32(uncompressed, uncompressedBytes);
	bfinal = FlipLong((unsigned char*)&adler32);
	WriteBytesForce(32, bfinal, &buffer);
	free(jumps);


	compressedBytes = (int)(buffer.pos-buffer.data);
	//unsigned int test = Adler32(uncompressed, pos);
	return buffer.data;
}


