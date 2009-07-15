#pragma once

#include "..\global.h"
#include <malloc.h>

unsigned char * Deflate(unsigned char* uncompressed, unsigned int uncompressedBytes, unsigned int &compressedBytes, unsigned int lineLength);

unsigned char * DeflateSSE2(unsigned char* uncompressed, unsigned int uncompressedBytes, unsigned int &compressedBytes, unsigned int lineLength);


/*
unsigned char * DeflateSSE2(unsigned char* uncompressed, unsigned int uncompressedBytes, unsigned int &compressedBytes, unsigned int lineLength) {
	__declspec(align(16)) int table[16384][2];
	__declspec(align(16)) unsigned int counts[336];
	__declspec(align(16)) unsigned int distCheck[4] = {35000, 35000, 35000, 35000};
	unsigned int s1=1, s2=0;
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
	unsigned int dist;

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
					mov esi, i
					mov ecx, s1
					movzx eax, [edi+esi]


					mov edx, s2
					add ecx, eax
					add edi, esi
					add edx, ecx

					mov eax, ecx
					mov esi, edx

					sub eax, BASE
					mov pos2, edi
					cmovge ecx, eax
					mov eax, i

					sub esi, BASE
					mov s1, ecx
					cmovge edx, esi

					cmp eax, safetyEnd
					mov s2, edx

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

						//sub edi, dword ptr table[eax*8]
						sub esi, dword ptr table[eax*8+4]
						sub edi, ecx

						mov dword ptr table[eax*8], edx
						mov dword ptr table[eax*8+4], ecx
						mov [distCheck], edi
						mov [distCheck+4], esi

						//prefetcht0 [edi]
						//prefetcht0 [esi]
						//xor ecx, ecx
shipHashUpdate:
					//}
				}

					for (j=0;(dist=distCheck[j])<32767;j++) {
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
					j=i+matched-1;
					//i++;
					if (j<safetyEnd) {
						//while(i<j) {
							__asm {
								mov eax, i
hashLoop:
								mov edi, pos
								inc eax
								mov ecx, s1
								movzx esi, [edi+eax]
								//inc eax
								//add edi, eax
								add ecx, esi

								add edi, eax
								mov i, eax
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

								mov s2, edx

							


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
					else {
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
					}
					//i+=matched-1;
					//break;
				}
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


		Jump *jump = jumps;
		unsigned char * finalPos = pos+blockLen;
		while (pos<finalPos) {
			if (jump->pos == pos) {
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
	unsigned int adler32 = (s2 << 16) + s1;
	bfinal = FlipLong((unsigned char*)&adler32);
	WriteBytesForce(32, bfinal, &buffer);
	free(jumps);


	compressedBytes = (int)(buffer.pos-buffer.data);
	return buffer.data;
}
*/
