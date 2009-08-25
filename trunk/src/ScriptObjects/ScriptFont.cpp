#include "../global.h"
#include "../screen.h"
#include "ScriptFont.h"
#include "ScriptObjects.h"
#include "../unicode.h"
#include "../vm.h"
#include "../Config.h"

unsigned int FontType;
unsigned int f04b08[] =
#ifndef X64
						{0x00000001, 0x00000468, 0x00000006, 0x00000006, 0x00000000, 0xfffffffe, 0x00000002, 0x00000006, 0x00000006, 0x000000c8, 0x00000000, 0x00000060, 0x00000060, 0x20100020, 0x0020001f, 0x07000000, 0x00a69000, 0x00000000, 0x05000400, 0x03000000, 0x05030401, 0x02000004, 0x05030402, 0x03000008, 0x03010401, 0x0200000c, 0x03010402, 0x02000010, 0x05010402, 0x02000014, 0x03030402, 0x02000018, 0x03020402, 0x0200001c, 0x04010402, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x03000020, 0x05010401, 0x03000024, 0x05000401, 0x00000000, 0x05000400, 0x03000028, 0x04010401, 0x0200002c, 0x05010402, 0x03000030, 0x05010401, 0x02000034, 0x04020402, 0x02000038, 0x04010402, 0x0200003c, 0x04010402, 0x02000040, 0x04010402, 0x03000044, 0x03010401, 0x03000048, 0x05030401, 0x0200004c, 0x05010402, 0x02000050, 0x04010402, 0x03000054, 0x05010401, 0x02000058, 0x04020402, 0x0200005c, 0x03030402, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x02000060, 0x05010402, 0x03000064, 0x02010401, 0x05000068, 0x05010601, 0x0500006e, 0x05010601, 0x05000074, 0x05010601, 0x0600007a, 0x05010701, 0x01000081, 0x02010201, 0x02000083, 0x05010301, 0x02000086, 0x05010301, 0x03000089, 0x03010401, 0x0500008d, 0x05010601, 0x01000093, 0x05040201, 0x05000095, 0x03030601, 0x0100009b, 0x05050201, 0x0500009d, 0x05010601, 0x050000a3, 0x05010601, 0x030100a9, 0x05010602, 0x050000ad, 0x05010601, 0x050000b3, 0x05010601, 0x050000b9, 0x05010601, 0x050000bf, 0x05010601, 0x050000c5, 0x05010601, 0x050000cb, 0x05010601, 0x050000d1, 0x05010601, 0x050000d7, 0x05010601, 0x010000dd, 0x04020201, 0x010000df, 0x05020201, 0x030000e1, 0x05010401, 0x050000e5, 0x04020601, 0x030000eb, 0x05010401, 0x050000ef, 0x05010601, 0x050000f5, 0x05010601, 0x050000fb, 0x05010601, 0x05000101, 0x05010601, 0x05000107, 0x05010601, 0x0500010d, 0x05010601, 0x05000113, 0x05010601, 0x05000119, 0x05010601, 0x0500011f, 0x05010601, 0x05000125, 0x05010601, 0x0300012b, 0x05010401, 0x0500012f, 0x05010601, 0x05000135, 0x05010601, 0x0500013b, 0x05010601, 0x05000141, 0x05010601, 0x05000147, 0x05010601, 0x0500014d, 0x05010601, 0x05000153, 0x05010601, 0x05000159, 0x05010601, 0x0500015f, 0x05010601, 0x05000165, 0x05010601, 0x0500016b, 0x05010601, 0x05000171, 0x05010601, 0x05000177, 0x05010601, 0x0500017d, 0x05010601, 0x05000183, 0x05010601, 0x05000189, 0x05010601, 0x0500018f, 0x05010601, 0x02000195, 0x05010301, 0x05000198, 0x05010601, 0x0200019e, 0x05010301, 0x030001a1, 0x02010401, 0x050001a5, 0x05050601, 0x020001ab, 0x02010301, 0x050001ae, 0x05010601, 0x050001b4, 0x05010601, 0x050001ba, 0x05010601, 0x050001c0, 0x05010601, 0x050001c6, 0x05010601, 0x050001cc, 0x05010601, 0x050001d2, 0x05010601, 0x050001d8, 0x05010601, 0x030001de, 0x05010401, 0x050001e2, 0x05010601, 0x050001e8, 0x05010601, 0x050001ee, 0x05010601, 0x050001f4, 0x05010601, 0x050001fa, 0x05010601, 0x05000200, 0x05010601, 0x05000206, 0x05010601, 0x0500020c, 0x05010601, 0x05000212, 0x05010601, 0x05000218, 0x05010601, 0x0500021e, 0x05010601, 0x05000224, 0x05010601, 0x0500022a, 0x05010601, 0x05000230, 0x05010601, 0x05000236, 0x05010601, 0x0500023c, 0x05010601, 0x05000242, 0x05010601, 0x03000248, 0x05010401, 0x0100024c, 0x05010201, 0x0300024e, 0x05010401, 0x04000252, 0x02010501, 0x02000257, 0x04010402, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000000};
#else
                        {0x00000001, 0x00000488, 0x00000006, 0x00000006, 0x00000000, 0xfffffffe, 0x00000002, 0x00000006, 0x00000006, 0x000000c8, 0x00000000, 0x00000060, 0x00000060, 0x20100020, 0x0020001f, 0x07000000, 0x0012ea00, 0x00000000, 0x00000000, 0x05000400, 0x03000000, 0x05030401, 0x02000004, 0x05030402, 0x03000008, 0x03010401, 0x0200000c, 0x03010402, 0x02000010, 0x05010402, 0x02000014, 0x03030402, 0x02000018, 0x03020402, 0x0200001c, 0x04010402, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x03000020, 0x05010401, 0x03000024, 0x05000401, 0x00000000, 0x05000400, 0x03000028, 0x04010401, 0x0200002c, 0x05010402, 0x03000030, 0x05010401, 0x02000034, 0x04020402, 0x02000038, 0x04010402, 0x0200003c, 0x04010402, 0x02000040, 0x04010402, 0x03000044, 0x03010401, 0x03000048, 0x05030401, 0x0200004c, 0x05010402, 0x02000050, 0x04010402, 0x03000054, 0x05010401, 0x02000058, 0x04020402, 0x0200005c, 0x03030402, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x00000000, 0x05000400, 0x02000060, 0x05010402, 0x03000064, 0x02010401, 0x05000068, 0x05010601, 0x0500006e, 0x05010601, 0x05000074, 0x05010601, 0x0600007a, 0x05010701, 0x01000081, 0x02010201, 0x02000083, 0x05010301, 0x02000086, 0x05010301, 0x03000089, 0x03010401, 0x0500008d, 0x05010601, 0x01000093, 0x05040201, 0x05000095, 0x03030601, 0x0100009b, 0x05050201, 0x0500009d, 0x05010601, 0x050000a3, 0x05010601, 0x030100a9, 0x05010602, 0x050000ad, 0x05010601, 0x050000b3, 0x05010601, 0x050000b9, 0x05010601, 0x050000bf, 0x05010601, 0x050000c5, 0x05010601, 0x050000cb, 0x05010601, 0x050000d1, 0x05010601, 0x050000d7, 0x05010601, 0x010000dd, 0x04020201, 0x010000df, 0x05020201, 0x030000e1, 0x05010401, 0x050000e5, 0x04020601, 0x030000eb, 0x05010401, 0x050000ef, 0x05010601, 0x050000f5, 0x05010601, 0x050000fb, 0x05010601, 0x05000101, 0x05010601, 0x05000107, 0x05010601, 0x0500010d, 0x05010601, 0x05000113, 0x05010601, 0x05000119, 0x05010601, 0x0500011f, 0x05010601, 0x05000125, 0x05010601, 0x0300012b, 0x05010401, 0x0500012f, 0x05010601, 0x05000135, 0x05010601, 0x0500013b, 0x05010601, 0x05000141, 0x05010601, 0x05000147, 0x05010601, 0x0500014d, 0x05010601, 0x05000153, 0x05010601, 0x05000159, 0x05010601, 0x0500015f, 0x05010601, 0x05000165, 0x05010601, 0x0500016b, 0x05010601, 0x05000171, 0x05010601, 0x05000177, 0x05010601, 0x0500017d, 0x05010601, 0x05000183, 0x05010601, 0x05000189, 0x05010601, 0x0500018f, 0x05010601, 0x02000195, 0x05010301, 0x05000198, 0x05010601, 0x0200019e, 0x05010301, 0x030001a1, 0x02010401, 0x050001a5, 0x05050601, 0x020001ab, 0x02010301, 0x050001ae, 0x05010601, 0x050001b4, 0x05010601, 0x050001ba, 0x05010601, 0x050001c0, 0x05010601, 0x050001c6, 0x05010601, 0x050001cc, 0x05010601, 0x050001d2, 0x05010601, 0x050001d8, 0x05010601, 0x030001de, 0x05010401, 0x050001e2, 0x05010601, 0x050001e8, 0x05010601, 0x050001ee, 0x05010601, 0x050001f4, 0x05010601, 0x050001fa, 0x05010601, 0x05000200, 0x05010601, 0x05000206, 0x05010601, 0x0500020c, 0x05010601, 0x05000212, 0x05010601, 0x05000218, 0x05010601, 0x0500021e, 0x05010601, 0x05000224, 0x05010601, 0x0500022a, 0x05010601, 0x05000230, 0x05010601, 0x05000236, 0x05010601, 0x0500023c, 0x05010601, 0x05000242, 0x05010601, 0x03000248, 0x05010401, 0x0100024c, 0x05010201, 0x0300024e, 0x05010401, 0x04000252, 0x02010501, 0x02000257, 0x04010402, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000000};
#endif

unsigned int f04b08img[] = {0x0000025b, 0x00000006, 0x00000000, 0x00000070, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x30022200, 0x22022654, 0x00222023, 0x3d37ca52, 0x00008a52, 0x98fbe672, 0x0fbefbef, 0xfbef8808, 0xbef9ef9e, 0x0a2e3a2f, 0xbefbe8a2, 0xa8a2fbef, 0xc16fa28a, 0x7cf7c804, 0xd17df7cf, 0x45105171, 0xdf7df7df, 0x14554517, 0x01a8d67d, 0x33022200, 0x22223652, 0x02222023, 0x24b15f52, 0x0000848a, 0x94820489, 0xa8a28020, 0x8a2813e4, 0x820a20a2, 0x09281220, 0xa28a29b6, 0xa8a22028, 0x82242252, 0x0514500a, 0x91041051, 0x4db04940, 0x01451451, 0x12954511, 0x01949221, 0x33323636, 0x22373653, 0x33623773, 0x9847ca02, 0x83e3ea88, 0x92fbe488, 0x0fbe43ef, 0xfbae2002, 0xbefa20be, 0x08e813ee, 0xa2fa2aaa, 0xa8a223ef, 0x84221422, 0x05f7c000, 0x9f75f7d1, 0x55504740, 0x1f7d17d1, 0xa1154511, 0x01811110, 0x30020022, 0x22223253, 0x02222202, 0x65a51f00, 0x40088088, 0x3e802488, 0xa8222228, 0x8aa013e4, 0x820a20a2, 0x09289228, 0xb20a2ca2, 0xa5222204, 0x88210852, 0x05144000, 0x91441051, 0x65104944, 0x10259051, 0x42952911, 0x01809208, 0x00020022, 0x00022073, 0x00202200, 0xbd97ca02, 0x28088050, 0x90fbee70, 0x8fbe13ef, 0x8be20808, 0x82f9ef9e, 0xfa273a2f, 0xbe0be8a2, 0x423e23ec, 0xd06f8889, 0x7cf443e0, 0xd17c17cf, 0x4517d139, 0x1f65f05f, 0x444a11f1, 0x0000d67c, };
/*
unsigned int f04b03[] =
#ifndef X64
						{0x00000001, 0x0000045c, 0x00000008, 0x00000006, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000190, 0x00000000, 0x00000060, 0x00000060, 0x20100020, 0x0020001f, 0x07000000, 0x0012e300, 0x00000000, 0x07000400, 0x04000000, 0x07040400, 0x03000004, 0x07040401, 0x04000008, 0x04010400, 0x0300000c, 0x04010401, 0x03000010, 0x07010401, 0x03000014, 0x04040401, 0x03000018, 0x05030401, 0x0300001c, 0x06020401, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x03000020, 0x07020401, 0x03000024, 0x05000401, 0x00000000, 0x07000400, 0x04000028, 0x06020400, 0x0300002c, 0x07010401, 0x04000030, 0x07010400, 0x03000034, 0x06020401, 0x04000038, 0x07010400, 0x0300003c, 0x06020401, 0x03000040, 0x07010401, 0x04000044, 0x04010400, 0x04000048, 0x07040400, 0x0300004c, 0x07010401, 0x04000050, 0x07010400, 0x04000054, 0x07010400, 0x03000058, 0x04040401, 0x0300005c, 0x04040401, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x01000060, 0x05010201, 0x03000062, 0x02010401, 0x05000066, 0x05010601, 0x0400006c, 0x06010501, 0x05000071, 0x05010601, 0x05000077, 0x05010601, 0x0100007d, 0x02010201, 0x0200007f, 0x05010301, 0x02000082, 0x05010301, 0x03000085, 0x03010401, 0x03000089, 0x04020401, 0x0200008d, 0x06050301, 0x03000090, 0x03030401, 0x01000094, 0x05050201, 0x05000096, 0x05010601, 0x0400009c, 0x05010501, 0x020000a1, 0x05010301, 0x040000a4, 0x05010501, 0x040000a9, 0x05010501, 0x040000ae, 0x05010501, 0x040000b3, 0x05010501, 0x040000b8, 0x05010501, 0x040000bd, 0x05010501, 0x040000c2, 0x05010501, 0x040000c7, 0x05010501, 0x010000cc, 0x04020201, 0x010000ce, 0x05020201, 0x030000d0, 0x05010401, 0x030000d4, 0x04020401, 0x030000d8, 0x05010401, 0x040000dc, 0x05010501, 0x050000e1, 0x05010601, 0x040000e7, 0x05010501, 0x040000ec, 0x05010501, 0x030000f1, 0x05010401, 0x040000f5, 0x05010501, 0x030000fa, 0x05010401, 0x030000fe, 0x05010401, 0x04000102, 0x05010501, 0x04000107, 0x05010501, 0x0300010c, 0x05010401, 0x04000110, 0x05010501, 0x04000115, 0x05010501, 0x0300011a, 0x05010401, 0x0500011e, 0x05010601, 0x04000124, 0x05010501, 0x04000129, 0x05010501, 0x0400012e, 0x05010501, 0x04000133, 0x06010501, 0x04000138, 0x05010501, 0x0400013d, 0x05010501, 0x03000142, 0x05010401, 0x04000146, 0x05010501, 0x0400014b, 0x05010501, 0x05000150, 0x05010601, 0x04000156, 0x05010501, 0x0400015b, 0x05010501, 0x03000160, 0x05010401, 0x02000164, 0x05010301, 0x05000167, 0x05010601, 0x0200016d, 0x05010301, 0x03000170, 0x02010401, 0x04000174, 0x05050501, 0x02000179, 0x02010301, 0x0400017c, 0x05020501, 0x04000181, 0x05010501, 0x03000186, 0x05020401, 0x0400018a, 0x05010501, 0x0400018f, 0x05020501, 0x03000194, 0x05010401, 0x04000198, 0x07020501, 0x0400019d, 0x05010501, 0x010001a2, 0x05010201, 0x020001a4, 0x07010301, 0x040001a7, 0x05010501, 0x010001ac, 0x05010201, 0x050001ae, 0x05020601, 0x040001b4, 0x05020501, 0x040001b9, 0x05020501, 0x040001be, 0x07020501, 0x040001c3, 0x07020501, 0x030001c8, 0x05020401, 0x040001cc, 0x05020501, 0x030001d1, 0x05010401, 0x040001d5, 0x05020501, 0x040001da, 0x05020501, 0x050001df, 0x05020601, 0x030001e5, 0x05020401, 0x040001e9, 0x07020501, 0x040001ee, 0x05020501, 0x030001f3, 0x05010401, 0x010001f7, 0x05010201, 0x030001f9, 0x05010401, 0x040001fd, 0x02010501, 0x03000202, 0x06020401, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000000};
#else
						{0x00000001, 0x0000045c, 0x00000008, 0x00000006, 0x00000002, 0x00000000, 0x00000000, 0x00000004, 0x00000005, 0x00000190, 0x00000000, 0x00000060, 0x00000060, 0x20100020, 0x0020001f, 0x07000000, 0x0012e300, 0x00000000, 0x00000000, 0x07000400, 0x04000000, 0x07040400, 0x03000004, 0x07040401, 0x04000008, 0x04010400, 0x0300000c, 0x04010401, 0x03000010, 0x07010401, 0x03000014, 0x04040401, 0x03000018, 0x05030401, 0x0300001c, 0x06020401, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x03000020, 0x07020401, 0x03000024, 0x05000401, 0x00000000, 0x07000400, 0x04000028, 0x06020400, 0x0300002c, 0x07010401, 0x04000030, 0x07010400, 0x03000034, 0x06020401, 0x04000038, 0x07010400, 0x0300003c, 0x06020401, 0x03000040, 0x07010401, 0x04000044, 0x04010400, 0x04000048, 0x07040400, 0x0300004c, 0x07010401, 0x04000050, 0x07010400, 0x04000054, 0x07010400, 0x03000058, 0x04040401, 0x0300005c, 0x04040401, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x00000000, 0x07000400, 0x01000060, 0x05010201, 0x03000062, 0x02010401, 0x05000066, 0x05010601, 0x0400006c, 0x06010501, 0x05000071, 0x05010601, 0x05000077, 0x05010601, 0x0100007d, 0x02010201, 0x0200007f, 0x05010301, 0x02000082, 0x05010301, 0x03000085, 0x03010401, 0x03000089, 0x04020401, 0x0200008d, 0x06050301, 0x03000090, 0x03030401, 0x01000094, 0x05050201, 0x05000096, 0x05010601, 0x0400009c, 0x05010501, 0x020000a1, 0x05010301, 0x040000a4, 0x05010501, 0x040000a9, 0x05010501, 0x040000ae, 0x05010501, 0x040000b3, 0x05010501, 0x040000b8, 0x05010501, 0x040000bd, 0x05010501, 0x040000c2, 0x05010501, 0x040000c7, 0x05010501, 0x010000cc, 0x04020201, 0x010000ce, 0x05020201, 0x030000d0, 0x05010401, 0x030000d4, 0x04020401, 0x030000d8, 0x05010401, 0x040000dc, 0x05010501, 0x050000e1, 0x05010601, 0x040000e7, 0x05010501, 0x040000ec, 0x05010501, 0x030000f1, 0x05010401, 0x040000f5, 0x05010501, 0x030000fa, 0x05010401, 0x030000fe, 0x05010401, 0x04000102, 0x05010501, 0x04000107, 0x05010501, 0x0300010c, 0x05010401, 0x04000110, 0x05010501, 0x04000115, 0x05010501, 0x0300011a, 0x05010401, 0x0500011e, 0x05010601, 0x04000124, 0x05010501, 0x04000129, 0x05010501, 0x0400012e, 0x05010501, 0x04000133, 0x06010501, 0x04000138, 0x05010501, 0x0400013d, 0x05010501, 0x03000142, 0x05010401, 0x04000146, 0x05010501, 0x0400014b, 0x05010501, 0x05000150, 0x05010601, 0x04000156, 0x05010501, 0x0400015b, 0x05010501, 0x03000160, 0x05010401, 0x02000164, 0x05010301, 0x05000167, 0x05010601, 0x0200016d, 0x05010301, 0x03000170, 0x02010401, 0x04000174, 0x05050501, 0x02000179, 0x02010301, 0x0400017c, 0x05020501, 0x04000181, 0x05010501, 0x03000186, 0x05020401, 0x0400018a, 0x05010501, 0x0400018f, 0x05020501, 0x03000194, 0x05010401, 0x04000198, 0x07020501, 0x0400019d, 0x05010501, 0x010001a2, 0x05010201, 0x020001a4, 0x07010301, 0x040001a7, 0x05010501, 0x010001ac, 0x05010201, 0x050001ae, 0x05020601, 0x040001b4, 0x05020501, 0x040001b9, 0x05020501, 0x040001be, 0x07020501, 0x040001c3, 0x07020501, 0x030001c8, 0x05020401, 0x040001cc, 0x05020501, 0x030001d1, 0x05010401, 0x040001d5, 0x05020501, 0x040001da, 0x05020501, 0x050001df, 0x05020601, 0x030001e5, 0x05020401, 0x040001e9, 0x07020501, 0x040001ee, 0x05020501, 0x030001f3, 0x05010401, 0x010001f7, 0x05010201, 0x030001f9, 0x05010401, 0x040001fd, 0x02010501, 0x03000202, 0x06020401, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000000};
#endif

unsigned int f04b03img[] = {0x00000206, 0x00000008, 0x00000000, 0x00000070, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00044400, 0x04044050, 0x00444047, 0x23124295, 0x640000a5, 0xe6790e76, 0x71040319, 0xdcec731c, 0x452c74b9, 0xc731cc94, 0x4a514a5d, 0x020260b7, 0x20402002, 0x000010a4, 0x00040000, 0x46b00000, 0x00000001, 0x70044400, 0x6e446e54, 0x004e4047, 0xa090e7d5, 0x92000448, 0x01099084, 0x827254a5, 0x452294a2, 0xc4a82484, 0x294a52b6, 0x4a554a48, 0xe4054114, 0xee23398e, 0xcc73d480, 0xa52ee571, 0xa493d2aa, 0x0000001c, 0x57044400, 0x64646e52, 0x00444047, 0x8b083281, 0x91070ea8, 0x87394c64, 0x64010718, 0xdd2274ba, 0x446827b5, 0xc94a52d5, 0x71952a48, 0x90004212, 0x2976a452, 0x529552a5, 0xa524334a, 0x0889124a, 0x00000014, 0x57747c7c, 0x647f6a53, 0x77c47ff7, 0x8484c7c0, 0x90800408, 0x4943d014, 0x02725424, 0x452297aa, 0x44a924a4, 0x0749d294, 0x42552a49, 0x90004411, 0x2921a452, 0x529553a5, 0x1524c14a, 0x04909245, 0x00000014, 0x57040044, 0x04646e73, 0x00444406, 0x0b247281, 0x60504005, 0x46390ef4, 0x21044318, 0x5cec749c, 0x5d2674b8, 0xe9304c94, 0x324a1188, 0xe0f06837, 0x2e23398e, 0xcc9554a5, 0x09c87171, 0x06b3dca5, 0x00000014, 0x70040044, 0x64446703, 0x00444406, 0x00004000, 0x00002000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00400000, 0x00000000, 0x00000000, 0x08000000, 0x40000020, 0x00000040, 0x00001000, 0x0000001c, 0x00040044, 0x0e044003, 0x00444406, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x40000010, 0x00000040, 0x00000c00, 0x00000000};
//*/

ObjectValue *internalFonts[1];

inline int CreateInternalFont(void *s, void *img, int i) {
	ScriptValue obj;
	StringValue *Font = (StringValue *)s;
	((DynamicFont*)Font->value)->chars.img = (BitImage*)img;
	if (!CreateObjectValue(obj, FontType)) return 0;
	internalFonts[i] = obj.objectVal;
	CreateStringValue(internalFonts[i]->values[0], Font);
	DynamicFont *f = GetNewFontMetrics(internalFonts[0]);
	ScriptValue sv;
	CreateStringValue(sv, "04b08");
	f->name = sv.stringVal;
	return 1;
}

int InitFonts() {
	return CreateInternalFont(f04b08, f04b08img, 0);
		//&& CreateInternalFont(f04b03, f04b03img, 1);
}

ObjectValue **fonts = 0;
int numFonts = 0;

void CleanupFonts() {
	DynamicFont *f = GetNewFontMetrics(internalFonts[0]);
	f->name->Release();
	if (((size_t)f->block)&~1) {
		f->block->Cleanup();
		free(f->block);
	}
	if (((size_t)f->blockBlock)&~1) {
		f->blockBlock->Cleanup();
		free(f->blockBlock);
	}
	free(internalFonts[0]);
	//free(internalFonts[1]);
	free(fonts);
	//internalFonts[1]->Release();
}


/*
#define FONT 0
#define HEIGHT 1
#define WIDTH 2
#define BOLD 3
#define ITALIC 4
#define METRICS 5
#define IMAGE 6
*/

DynamicFont *GetNewFontMetrics(ObjectValue *o) {
	if (!o->values->stringVal) return 0;
	return (DynamicFont*)&o->values->stringVal->value;
}

CharData *GetCharData(DynamicFont *f, unsigned long l, BitImage* &b, int bold) {
	if (bold && !f->bold) {
		if (!f->boldFont) {
			ScriptValue sv, sv2;
			CreateNullValue(sv);
			if (CreateListValue(sv2, 5)) {
				sv2.listVal->PushBack(0);

				f->name->AddRef();
				CreateStringValue(sv2.listVal->vals[0], f->name);
				sv2.listVal->PushBack(f->height);
				sv2.listVal->PushBack(f->width);
				sv2.listVal->PushBack(1);
				sv2.listVal->PushBack(f->italics);
				Font(sv, sv2.listVal->vals);
				sv2.listVal->Release();
				if (sv.type == SCRIPT_OBJECT) {
					f->obj = sv.objectVal;
					f->boldFont = GetNewFontMetrics(sv.objectVal);
				}
				else sv.Release();
			}
		}
		if (f->boldFont) return GetCharData(f->boldFont, l, b, 0);
	}
	if (l <= 127) {
		b = f->chars.img;
		return &f->chars.chars[l];
	}
	else if (l <= 128*128-1) {
		int block = l/128;
		int c = l&127;
		if (!f->block) {
			if (!f->name) goto fail;
			if (!(f->block = (CharBlockBlock*) calloc(1, sizeof(CharBlockBlock)))) goto fail;
		}
		if (!(((size_t)f->block->blocks[block])&~1)) {
			if (((size_t)f->block->blocks[block]) == 1) goto fail;
			if (!(f->block->blocks[block] = (CharBlock*) calloc(1, sizeof(CharBlock)))) goto fail;
			if (!f->block->blocks[block]->TryInit(l&~127, f)) {
				f->block->blocks[block]->Cleanup();
				free(f->block->blocks[block]);
				f->block->blocks[block] = (CharBlock*)1;
				goto fail;
			}
		}
		b = f->block->blocks[block]->img;
		return &f->block->blocks[block]->chars[c];
	}
	else if (l <= 128*128*128-1) {
		int block = l/128/128;
		int subblock = (l/128)%128;
		int c = l%128;
		if (!f->blockBlock) {
			if (!f->name) goto fail;
			if (!(f->blockBlock = (CharBlockBlockBlock*) calloc(1, sizeof(CharBlockBlockBlock)))) goto fail;
		}
		if (!f->blockBlock->blocks[block]) {
			if (!(f->blockBlock->blocks[block] = (CharBlockBlock*) calloc(1, sizeof(CharBlockBlock)))) goto fail;
		}
		if (!(((size_t)f->blockBlock->blocks[block]->blocks[subblock])&~1)) {
			if (((size_t)f->blockBlock->blocks[block]->blocks[subblock]) == 1) goto fail;
			if (!(f->blockBlock->blocks[block]->blocks[subblock] = (CharBlock*) calloc(1, sizeof(CharBlock)))) goto fail;
			if (!f->blockBlock->blocks[block]->blocks[subblock]->TryInit(l&~127, f)) {
				f->blockBlock->blocks[block]->blocks[subblock]->Cleanup();
				free(f->blockBlock->blocks[block]->blocks[subblock]);
				f->blockBlock->blocks[block]->blocks[subblock] = (CharBlock*)1;
				goto fail;
			}
		}
		b = f->blockBlock->blocks[block]->blocks[subblock]->img;
		return &f->blockBlock->blocks[block]->blocks[subblock]->chars[c];
	}
fail:
	b = f->chars.img;
	return &f->chars.chars['?'];
}

int GetWidth(ObjectValue *o, unsigned char *s, int length, int *height) {
	DynamicFont * f = GetNewFontMetrics(o);
	if (!f) return 0;
	if (length < 0) {
		length = 0;
		while (s[length]) length++;
	}
	*height = 1;
	int tout = 0;
	int out = 0;
	unsigned long start = 0;
	unsigned long last = 0;
	//CharData *data2, *data3;
	BitImage *b;
	int bold = 0;
	while (length > 0) {
		int c;
		unsigned long l = NextUnicodeChar(s, &c);
		if (s[0] == '\r' || s[0] == '\n') {
			/*if (tout) {
				data2 = GetCharData(f, start, b);
				data3 = GetCharData(f, last, b);
				if (data2) {
					tout -= data2->abcA;
				}
				if (data3) {
					tout -= data3->abcC;
				}
			}//*/


			if (out < tout) out = tout;
			if (s[0] == '\r' && s[1] == '\n') {
				s++;
				length--;
			}
			s++;
			length --;
			if (length <= 0) break;
			tout = 0;
			height[0] ++;
			continue;
		}
		if (l==2) {
			bold ^= 1;
			s++;
			length--;
			continue;
		}

		if (!start) start = l;
		last = l;
		CharData *data;
		data = GetCharData(f, l, b, bold);
		if (data) {
			if (l == '\t') {
				tout += data->width - (tout % data->width);
			}
			else {
				tout += data->width;
			}
		}
		s += c;
		length -= c;
	}
	/*if (tout) {
		data2 = GetCharData(f, start, b);
		data3 = GetCharData(f, last, b);
		if (data2) {
			tout -= data2->abcA;
		}
		if (data3) {
			tout -= data3->abcC;
		}//*/
		if (out < tout) out = tout;
	//}
	*height *= f->tmHeight;
	return out;
}


static HFONT CreateFont(DynamicFont *f) {
	wchar_t face[LF_FACESIZE*3+1];
	int len = UTF8toUTF16(face, f->name->value, -1);
	//int t = GetDeviceCaps (hDC, LOGPIXELSY);
	if (len >= LF_FACESIZE) return 0;
	//return CreateFontW(-MulDiv(10,96,72), 0, 0, 0, 900, 1800, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, 0, L"Arial");
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	memcpy(lf.lfFaceName, face, 2*LF_FACESIZE);
	lf.lfQuality = NONANTIALIASED_QUALITY;
	if (f->quality) lf.lfQuality = f->quality;
	lf.lfHeight = f->height;
	lf.lfWidth = f->width;
	lf.lfWeight = f->bold;
	lf.lfItalic = (char) f->italics;
	return CreateFontIndirect(&lf);
}

int CharBlock::TryInit(int rangeStart, DynamicFont *f) {
	int res = 0;
	HDC hDC;
	if (hDC = CreateCompatibleDC(0)) {
		HFONT hFont = f->hFont;
		if (hFont || (hFont = CreateFont(f))) {
			HANDLE hPrevFont = (HFONT) SelectObject(hDC, hFont);
			if (hPrevFont) {
				if (Init(rangeStart, f, hDC)) {
					res = 1;
				}
				SelectObject(hDC, hPrevFont);
			}

			if (!f->hFont) DeleteObject(hFont);
		}
		DeleteDC(hDC);
	}
	return res;
}

#include <stdio.h>

int CharBlock::Init(int rangeStart, DynamicFont *tm, HDC hDC) {
	//if (!rangeStart) return 1;
	union {
		wchar_t name[1000];
		char name2[1000];
	};
	if (GetTextFaceW(hDC, sizeof(name), name)) {
		wchar_t face[LF_FACESIZE*3+1];
		int len = UTF8toUTF16(face, tm->name->value, -1);
		if (wcsicmp(face, name) && GetTextFaceA(hDC, sizeof(name2), name2)) {
			errorPrintf(0, "Script Warning: Tried to create font %s, got %s.\n", tm->name->value, name2);
		}
	}
	{
		TEXTMETRIC cmp;
		if (!GetTextMetrics(hDC, &cmp) ||
			cmp.tmAscent != tm->tmAscent ||
			cmp.tmDescent != tm->tmDescent ||
			cmp.tmHeight != tm->tmHeight) {
				return 0;
		}
	}
	memset(chars, 0, sizeof(chars));
	GLYPHMETRICS gm;
	MAT2 mat2;
	memset(&mat2, 0, sizeof(mat2));
	mat2.eM11.value = 1;
	mat2.eM22.value = 1;
	DWORD maxMem = 0;
	int len = 0;
	int minHeight = 1000;
	int maxHeight = 0;
	for (int i=0; i<127; i++) {
		DWORD mem = GetGlyphOutline(
			hDC,
			rangeStart+i,
			GGO_BITMAP,
			&gm,
			0,
			0,
			&mat2
		);
		if (mem == GDI_ERROR) {
			continue;
		}
		gm.gmptGlyphOrigin.y = tm->tmAscent-gm.gmptGlyphOrigin.y;

		if (!mem) gm.gmBlackBoxX = 0;

		// Done here because of space character, which fails when actually given memory.
		chars[i].abcA = (char)gm.gmptGlyphOrigin.x;
		chars[i].abcB = (char)gm.gmBlackBoxX;
		chars[i].abcC = (char)(gm.gmCellIncX - gm.gmptGlyphOrigin.x - gm.gmBlackBoxX);
		chars[i].width = (char)gm.gmCellIncX;
		if (rangeStart+i == ' ') {
			chars['\t'].abcA = 0;
			chars['\t'].abcB = 0;
			chars['\t'].abcC = 0;
			RECT r = {0,0,0,0};
			// Extra call here probably isn't necessary, but some MS draw functions
			// can leave trailing whitespace out of reported width, so best to be
			// careful.
			DrawTextW(hDC, L"\t1", -1, &r, DT_NOCLIP | DT_EXPANDTABS | DT_CALCRECT);
			chars['\t'].width = (char)r.right;
			DrawTextW(hDC, L"1", -1, &r, DT_NOCLIP | DT_EXPANDTABS | DT_CALCRECT);
			chars['\t'].width -= (char)r.right;
		}

		len+=gm.gmBlackBoxX;
		minHeight = min(minHeight, gm.gmptGlyphOrigin.y);
		maxHeight = max(maxHeight, gm.gmptGlyphOrigin.y + (LONG)gm.gmBlackBoxY);
		maxMem = max(mem, maxMem);
	}
	if (tm->hFont) {
		img = 0;
		return 1;
	}
	if (!len) {
		return 0;
	}
	if (1) {
		img = MakeAllocatedBitImage(((len+31)&~31), maxHeight-minHeight);
		int width = (((len+31)&~31)/32);
		if (!img) return 0;
		memset(img->data, 0, 4*width * (maxHeight-minHeight));
		width *= 32;
		char *temp = (char*)malloc(maxMem);
		if (!temp) {
			free(img);
			return 0;
		}
		int pos = 0;
		for (int i=0; i<127; i++) {
			DWORD mem = GetGlyphOutline(
				hDC,
				rangeStart+i,
				GGO_BITMAP,
				&gm,
				maxMem,
				temp,
				&mat2
			);
			if (mem == GDI_ERROR) {
				// End up here on space.
				continue;
			}
			gm.gmptGlyphOrigin.y = tm->tmAscent-gm.gmptGlyphOrigin.y;

			chars[i].inc = (char)minHeight;
			chars[i].start = pos;
			chars[i].top = (char)gm.gmptGlyphOrigin.y;
			chars[i].bottom = (char)(gm.gmptGlyphOrigin.y + gm.gmBlackBoxY-1);

			int bmpWidth = 4*((gm.gmBlackBoxX+31)/32);
			int ypos = width*(gm.gmptGlyphOrigin.y-minHeight)+pos;
			int srcypos = 0;
			for (unsigned int y=0; y<gm.gmBlackBoxY; y++) {
				int destpos = ypos;
				for (unsigned int x=0; x<gm.gmBlackBoxX; x++) {
					img->data[destpos/32] |= ((((int)(temp[srcypos + x/8]>>(7-x%8))&1)) << (destpos%32));
					destpos++;
				}
				srcypos += bmpWidth;
				ypos += width;
			}
			pos+=gm.gmBlackBoxX;
		}
		free(temp);
		return 1;
	}
#ifdef GOAT
	//*/
	ABC abc[128];
	int res = GetCharABCWidths(hDC, rangeStart, rangeStart+127, abc);
	if (!res) {
		if (!GetCharWidth32(hDC, rangeStart, rangeStart+127, (int*)abc)) return 0;
		for (int i = 127; i>=0; i--) {
			abc[i].abcB = ((int*)abc)[i];
			abc[i].abcC = abc[i].abcA = 0;
		}
	}//*/

	int p = 0;
	/*DWORD dwAlignOrig = GetTextAlign(hDC);
	SetTextAlign(hDC, dwAlignOrig | TA_UPDATECP);
	MoveToEx(hDC, 0, 0, 0);
	for (int i = 127; i>=0; i--) {
		unsigned char junk[5];
		unsigned int index = rangeStart+i;
		int l = UTF32toUTF8(junk, &index, 1);
		wchar_t c[5];
		l = UTF8toUTF16(c, junk, l);
		TextOut(hDC, 0, 0, c, l);
		POINT p;
		MoveToEx(hDC, 0, 0, &p);
		abc[i].abcA = -6;
		abc[i].abcC = 0;
		abc[i].abcB = p.x+12;
	}
	SetTextAlign(hDC, dwAlignOrig &~TA_UPDATECP);//*/ 
	//abc['A'].abcB = 8;
	DWORD dwAlignOrig = GetTextAlign(hDC);
	SetTextAlign(hDC, dwAlignOrig | TA_UPDATECP);
	MoveToEx(hDC, 0, 0, 0);
	char deltas[128];
	memset(deltas,-1, sizeof(deltas));
	for (int i = 0; i<=127; i++) {
		unsigned char junk[5];
		unsigned int index = rangeStart+i;
		int l = UTF32toUTF8(junk, &index, 1);
		wchar_t c[5];
		l = UTF8toUTF16(c, junk, l);
		TextOut(hDC, 0, 0, c, l);
		POINT p2;
		MoveToEx(hDC, 0, 0, &p2);

		chars[i].start = p;
		if(abc[i].abcA > 0) {
			abc[i].abcB+=abc[i].abcA;
			abc[i].abcA=0;
		}
		if (abc[i].abcA < 0) {
			deltas[i] += abc[i].abcA;
		}
		else if(abc[i].abcC > 0) {
			abc[i].abcB+=abc[i].abcC;
			abc[i].abcC=0;
		}
		chars[i] = abc[i];
		// ???
		int len = 0;
		if (abc[i].abcA > 0) len += abc[i].abcA;
		if (abc[i].abcC > 0) len += abc[i].abcC;
		if (chars[i].abcB < len + p2.x) {
			chars[i].width += ((char)p2.x - len) - chars[i].abcB;
			chars[i].abcB = (char)p2.x - len;
		}
		chars[i].abcB += 10;
		chars[i].abcC-=10;
		chars[i].abcA--;
		chars[i].abcB++;
		if (chars[i].abcB > 127) chars[i].abcB = 0;
		p += chars[i].abcB;
		chars[i].top = 0;
		chars[i].bottom = (char)tm->tmHeight-1;
	}
	SetTextAlign(hDC, dwAlignOrig &~TA_UPDATECP);//
	//free(img);
	struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[256];
	} info;
	memset(&info, 0, sizeof(info));
	info.bmiHeader.biSize = sizeof(info.bmiHeader);
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 1;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biSizeImage = BI_RGB;
	info.bmiHeader.biXPelsPerMeter = 3200;
	info.bmiHeader.biYPelsPerMeter = 3200;
	info.bmiHeader.biClrUsed = 2;
	info.bmiHeader.biClrImportant = 2;
	info.bmiColors[0].rgbRed =
		info.bmiColors[0].rgbGreen =
		info.bmiColors[0].rgbBlue = 255;
	info.bmiColors[1].rgbRed =
		info.bmiColors[1].rgbGreen =
		info.bmiColors[1].rgbBlue = 0;
	unsigned char *src;
	SetBkColor(hDC, RGB(255, 255, 255));
	SetBkMode(hDC, OPAQUE);
	SetTextColor(hDC, RGB(0, 0, 0));
	SetMapMode(hDC, MM_TEXT);

	int b = tm->tmHeight;
	int miny = 0;
	for (int count=0; count<2; count++) {
		info.bmiHeader.biWidth = (p+31)&~31;
		info.bmiHeader.biHeight = 2*(-b+miny);

		HBITMAP hBitMap = CreateDIBSection(hDC, (BITMAPINFO*)&info, DIB_RGB_COLORS, (void**)&src, 0, 0);
		HBITMAP hPrevBitmap = 0;
		if (!hBitMap || !(hPrevBitmap = (HBITMAP) SelectObject(hDC, hBitMap))) {
			free(img);
			if (hBitMap) DeleteObject(hBitMap);
			img = 0;
			return 0;
		}
		//p = 0;
		for (int i=0; i<=127; i++) {
			if (i == 'g') {
				i=i;
			}
			//if (i == 'B') i++;
			if (chars[i].abcB) {
				unsigned char junk[5];
				unsigned int index = rangeStart+i;
				int l = UTF32toUTF8(junk, &index, 1);
				wchar_t c[5];
				l = UTF8toUTF16(c, junk, l);
				//if (i == 'M' || i == 'N')
				//	TextOut(hDC, p, -1, c, l);
				//else
				//if (i == 'm' || i == 'l' || i == 'n')
				int res = TextOut(hDC, chars[i].start-deltas[i], -miny, c, l);
				chars[i].inc = miny;
				//chars[i].start -= miny * info.bmiHeader.biWidth;
				//p += chars[i].width;
			}
		}
		GdiFlush();
		/*{
			int nBMISize = sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD);
			BITMAPINFO *m_pBitmapInfo = (BITMAPINFO *) new BYTE [nBMISize];
		    
			ZeroMemory(m_pBitmapInfo, nBMISize);
			m_pBitmapInfo->bmiHeader.biSize = sizeof(m_pBitmapInfo->bmiHeader);
			m_pBitmapInfo->bmiHeader.biWidth = 160;
			m_pBitmapInfo->bmiHeader.biHeight = -43;
			m_pBitmapInfo->bmiHeader.biPlanes = 1;
			m_pBitmapInfo->bmiHeader.biBitCount = 8;
			m_pBitmapInfo->bmiHeader.biCompression = BI_RGB;
			m_pBitmapInfo->bmiHeader.biSizeImage = 
				(160 * 
				43 * 
				8) / 8;
			m_pBitmapInfo->bmiHeader.biXPelsPerMeter = 3200;
			m_pBitmapInfo->bmiHeader.biYPelsPerMeter = 3200;
			m_pBitmapInfo->bmiHeader.biClrUsed = 256;
			m_pBitmapInfo->bmiHeader.biClrImportant = 256;

			HDC m_hDC = CreateCompatibleDC(NULL);
		    
			for(int nColor = 0; nColor < 256; ++nColor)
			{
				m_pBitmapInfo->bmiColors[nColor].rgbRed = (BYTE)((nColor > 128) ? 255 : 0);
				m_pBitmapInfo->bmiColors[nColor].rgbGreen = (BYTE)((nColor > 128) ? 255 : 0);
				m_pBitmapInfo->bmiColors[nColor].rgbBlue = (BYTE)((nColor > 128) ? 255 : 0);
				m_pBitmapInfo->bmiColors[nColor].rgbReserved = 0;
			}
			unsigned int *m_pBitmapBits;
			HBITMAP m_hBitmap = CreateDIBSection(m_hDC, m_pBitmapInfo, DIB_RGB_COLORS, (PVOID *) &m_pBitmapBits, NULL, 0);
			SetTextColor(m_hDC, RGB(255, 255, 255));
			SetBkColor(m_hDC, RGB(0, 0, 0));
			HBITMAP hPrevBitmap = (HBITMAP) SelectObject(m_hDC, m_hBitmap);

			int res = TextOut(m_hDC, 0, 0, L"A", 1);
			SetPixel(m_hDC, 1, 1, RGB(255, 255, 255));
			int res2 = FillRect(m_hDC, &rect, (HBRUSH) GetStockObject (WHITE_BRUSH));
			res = res;
		}//*/

		if (count) {
			img = MakeAllocatedBitImage(p, b-miny);
			if (!img) {
				SelectObject(hDC, hPrevBitmap);
				DeleteObject(hBitMap);
				return 0;
			}
			for (int index=(img->width+31)/32 * sizeof(int) * img->height-1; index>=0; index--) {
				unsigned char in = src[index];
				((unsigned char*)img->data)[index] =
					(in<<7) +
					((in&0x2)<<5) +
					((in&0x4)<<3) +
					((in&0x8)<<1) +
					((in&0x10)>>1) +
					((in&0x20)>>3) +
					((in&0x40)>>5) +
					(in>>7);
			}
			SelectObject(hDC, hPrevBitmap);
			DeleteObject(hBitMap);
			break;
		}
		int neg = 0;
		b = 0;
		miny = tm->tmHeight;
		for (int i = 0; i<=127; i++) {
			//if (i <'A' || i>'A') continue;
			int oldNeg = neg;
			//if (i >= 10) {
				int right = 0;
				int left = 30000;
				int top = 30000;
				int bottom = 0;
				for( int y = 0; y < tm->tmHeight; y++) {
					unsigned char *p = &((unsigned char*)src)[y*info.bmiHeader.biWidth/32*4];
					for (int x=chars[i].start; x <chars[i].abcB+chars[i].start; x++) {
						if (p[x/8] & (0x80>>(x&7))) {
							if (y > bottom) {
								bottom = y;
							}
							if (y < top) {
								top = y;
							}
							if (x > right) {
								right = x;
							}
							if (x < left) {
								left = x;
							}
						}
					}
				}
				if (top < miny) miny = top;
				if (bottom >= b) b = bottom+1;
				if (right>=left && top <= bottom) {
					//bottom++;
					right++;
					chars[i].top = top;
					chars[i].bottom = bottom;
					if (left !=chars[i].start) {
					//*
						int diff = left - chars[i].start;
						deltas[i]+=diff;
						chars[i].abcA += diff;
						chars[i].abcB -= diff;
						//chars[i].width -= diff;
						//chars[i].start -= diff;
						p-=diff;
						neg+=diff;
					//*/
					}
					if (right < chars[i].start+chars[i].abcB) {
					//*
						int diff = (chars[i].start+chars[i].abcB) - right;
						chars[i].abcC += diff;
						chars[i].abcB -= diff;
						neg += diff;
						p-=diff;
						//p-=diff;
						//neg+=diff;
						//chars[i].width -= diff;
						//chars[i].start += diff;
					//*/
					}
				}
				else {
					//*
					neg += chars[i].abcB;
					p -= chars[i].abcB;
					//chars[i].start -= chars[i].abcB;
					chars[i].start = oldNeg;
					chars[i].abcB = 0;//*/
					
				}
			//}
			chars[i].start -= oldNeg;
		}
		if (miny >3) miny = 3;
		//miny = 0;
		//while (miny&&(miny+1) * p >= 30000) miny--;
		/*
		for (int i=0; i<=127; i++) {
			int max = tm->tmHeight;
			int min = tm->tmHeight
			for (int 
		}//*/
		//free(img);
		//img = 0;
		SelectObject(hDC, hPrevBitmap);
		DeleteObject(hBitMap);
	}
	/*res = GetCharABCWidths(hDC, rangeStart, rangeStart+127, abc);
	if (res) {
		for (int i=127; i>=0; i--) {
			if (abc[i].abcA < 0) {
				abc[i].abcA = abc[i].abcA;
			}
			if (abc[i].abcC < 0) {
				abc[i].abcA = abc[i].abcA;
			}
			if (abc[i].abcA >= 0) {
				if (chars[i].abcA > abc[i].abcA)
					chars[i].abcA = abc[i].abcA;
			}
			if (abc[i].abcC >= 0) {
				if (chars[i].abcC > abc[i].abcC)
					chars[i].abcC = abc[i].abcC;
			}
		}
	}//*/
	return 1;
#endif
}


void Font(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal <= 0 || args[1].intVal >= 50 ||
		args[2].intVal < 0 || args[2].intVal >= 70) {
		return;
	}
	if (args[3].intVal == 1) args[3].i32 = 700;
	if (args[4].intVal) args[4].i32 = 1;
	for (int i=0; i<numFonts; i++) {
		DynamicFont *f2 = GetNewFontMetrics(fonts[i]);
		if (scriptstricmp(f2->name, args[0].stringVal) == 0 &&
			f2->height == args[1].i32 &&
			f2->width == args[2].i32 &&
			f2->bold == args[3].i32 &&
			f2->italics == args[4].i32 &&
			f2->quality == args[5].i32) {
				fonts[i]->AddRef();
				CreateObjectValue(s, fonts[i]);
				return;
		}
	}
	int happy = 0;
	if (args[0].stringVal->len <= LF_FACESIZE*3 && CreateObjectValue(s, FontType)) {
		if (AllocateStringValue(s.objectVal->values[0], sizeof(DynamicFont))) {
			args[0].stringVal->AddRef();
			DynamicFont *f = GetNewFontMetrics(s.objectVal);
			memset(f, 0, sizeof(DynamicFont));
			f->name = args[0].stringVal;
			f->height = args[1].i32;
			f->width = args[2].i32;
			f->bold = args[3].i32;
			f->italics = args[4].i32;
			f->quality = args[5].i32;
			HDC hDC;
			if (hDC = CreateCompatibleDC(0)) {
				HFONT hFont;
				if (hFont = CreateFont(f)) {
					HANDLE hPrevFont = (HFONT) SelectObject(hDC, hFont);
					if (hPrevFont) {
						if (GetTextMetrics(hDC, f) && f->tmHeight <= 100 && f->tmMaxCharWidth <= 100 && f->chars.Init(0, f, hDC)) {
	/*
	FILE *out = fopen("Font.txt", "wb");
	fprintf(out, "{");
	BitImage *temp = f->chars.img;
	StringValue *n = f->name;
	f->name = 0;
	f->chars.img = 0;
	for (int i=0; i<(sizeof(*f) + sizeof(StringValue)-4)/sizeof(int); i++) {
		fprintf(out, "0x%08x, ", ((unsigned int*)s.objectVal->values[0].stringVal)[i]);
	}
	f->name = n;
	f->chars.img = temp;
	fprintf(out, "0x00000000};\n\n");
	fprintf(out, "{");
	for (int i=0; i<(int)((sizeof(AllocatedBitImage) + (((f->chars.img->width+31)/32)*f->chars.img->height-1) * sizeof(int))/sizeof(int)); i++) {
		fprintf(out, "0x%08x, ", ((unsigned int*)f->chars.img)[i]);
	}
	fprintf(out, "};\n");
	fclose(out);//*/
							if (srealloc(fonts, sizeof(DynamicFont**) * (1+numFonts))) {
								fonts[numFonts] = s.objectVal;
								numFonts ++;
							}//*/
							if (f->quality) {
								f->hFont = hFont;
								hFont = 0;
							}
							happy = 1;
						}
						SelectObject(hDC, hPrevFont);
					}
					if (hFont) DeleteObject(hFont);
				}
				DeleteDC(hDC);
			}
		}
		if (!happy)
			s.objectVal->Release();
	}
	if (!happy)
		CreateNullValue(s);
}

/*
void GetFontVals(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	DynamicFont *f = GetNewFontMetrics(o);
	if (f && CreateListValue(s, 5)) {
		f->name->AddRef();
		CreateStringValue(s.listVal->vals[0], f->name);
		CreateIntValue(s.listVal->vals[1], f->height);
		CreateIntValue(s.listVal->vals[2], f->width);
		CreateIntValue(s.listVal->vals[3], f->height);
		CreateIntValue(s.listVal->vals[4], f->width);
		for (int i=0; i<3; i++)
			s.listVal->Set(o->values[i], i);
		return;
	}
	CreateNullValue(s);
}
///*/

void DeleteFont(ScriptValue &s, ScriptValue *args) {
	DynamicFont *f = GetNewFontMetrics(s.objectVal);
	for (int i=0; i<numFonts; i++) {
		if (fonts[i] == s.objectVal) {
			fonts[i] = fonts[--numFonts];
			break;
		}
	}
	if (f) f->Cleanup();
}

void UseFont(ScriptValue &s, ScriptValue *args) {

	CreateNullValue(s);
	if (args->type == SCRIPT_OBJECT) {
		if (args->objectVal->type == FontType) {
			args->objectVal->AddRef();
			activeScreen->SetFont(args->objectVal, s);
		}
	}
	else {
		ScriptValue sv;
		CoerceIntNoRelease(args[0], sv);
		if ((unsigned __int64)args[0].intVal == 0) {
			internalFonts[args[0].i32]->AddRef();
			activeScreen->SetFont(internalFonts[args[0].i32], s);
		}
		//else if ((unsigned __int64)args->intVal <= 1)
		//	activeScreen->SetFont((int)args->intVal, s);
	}
}

void GetFontHeight(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_OBJECT) {
		if (args->objectVal->type == FontType) {
			CreateIntValue(s, GetNewFontMetrics(args->objectVal)->height);
		}
	}
	else
		CreateIntValue(s, GetNewFontMetrics(activeScreen->activeNewFont)->height);
}

