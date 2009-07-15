/*inline unsigned int Hash2(unsigned int a, unsigned int b,unsigned int c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
  return c;
}
*/
inline unsigned int QuickHash(unsigned int key) {
	__asm {
		mov eax, key
		mov ecx, eax
		mov edx, eax

		rol eax, 3
		ror ecx, 13
		ror edx, 25

		sub eax, ecx
		xor eax, edx
		mov key, eax
	}
	return key;
  //return (key<<3) - (key >> 13)+(key>>25);
}

/*inline unsigned int Hash(unsigned int key)
{
  key += ~(key << 15);
  key ^=  (key >> 10);
  key +=  (key << 3);
  key ^=  (key >> 6);
  key += ~(key << 11);
  key ^=  (key >> 16);
  return key;
}

struct HashEntry {
	int value;
	HashEntry *next;
};



				__asm {
					mov edi, pos
					mov eax, i
					mov ecx, s1
					add edi, eax
					mov edx, s2

					movzx eax, [edi]

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

					mov s2, edx
				}


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

*/