/* Constains #defined constants only.  Note that this file
 * has no effect on memory usage, as the table of defined
 * values is removed from memory when compiling the .c files
 * is complete.  The .h extension could just as easily be
 * .c, but as it has no actual code in it, .h seemed more
 * appropriate.
 */

// File types for reading.
#define READER_CONVERT_TEXT 3
#define READER_UNCOMPRESS 1
#define READER_NORMAL 0

// Flags for writing.
#define WRITER_RAW		0
#define WRITER_UTF8		1
#define WRITER_ASCII	2

#define WRITER_APPEND	8

// Direct connection device types.  Only the G15 V1 and
// V2 have been tested. Direct connections probably don't
// work for the G11.
#define LCD_G11			1
#define LCD_G15_V1		2
#define LCD_G15_V2		3
#define LCD_Z10			4
#define LCD_GAME_PANEL	5
#define LCD_G19			6
#define SDK_160_43_1	64
#define SDK_320_240_32	65
#define SYSTRAY_ICON	128

#define G15_SOFT1		  0x01
#define G15_SOFT2		  0x02
#define G15_SOFT3		  0x04
#define G15_SOFT4		  0x08
#define G15_SOFT5		  0x10
#define G15_SOFT6		  0x20

#define G15_LEFT		  0x01
#define G15_RIGHT		  0x02
#define G15_OK			  0x04
#define G15_CANCEL		  0x08
#define G15_UP			  0x10
#define G15_DOWN		  0x20

#define G15_MENU		  0x40

#define G15_M1			0x0100
#define G15_M2			0x0200
#define G15_M3			0x0400
#define G15_MR			0x0800

#define G15LIGHT		0x1000

#define G15_G1		0x00004000
#define G15_G2		0x00008000
#define G15_G3		0x00010000
#define G15_G4		0x00020000
#define G15_G5		0x00040000
#define G15_G6		0x00080000
#define G15_G7		0x00100000
#define G15_G8		0x00200000
#define G15_G9		0x00400000
#define G15_G10		0x00800000
#define G15_G11		0x01000000
#define G15_G12		0x02000000
#define G15_G13		0x04000000
#define G15_G14		0x08000000
#define G15_G15		0x10000000
#define G15_G16		0x20000000
#define G15_G17		0x40000000
#define G15_G18		0x80000000

#define VK_BACK                 8
#define VK_TAB                  9

#define VK_RETURN              13

#define VK_CAPITAL             20

#define VK_ESCAPE			   27

#define  VK_F1				  112
#define  VK_F2				  113
#define  VK_F3				  114
#define  VK_F4				  115
#define  VK_F5				  116
#define  VK_F6				  117
#define  VK_F7				  118
#define  VK_F8				  119
#define  VK_F9				  120
#define  VK_F10				  121
#define  VK_F11				  122
#define  VK_F12				  123

#define	VK_SPACE         	   32
#define VK_PRIOR               33
#define VK_NEXT                34
#define VK_END                 35
#define VK_HOME                36
#define	VK_LEFT          	   37
#define	VK_UP            	   38
#define	VK_RIGHT         	   39
#define	VK_DOWN          	   40
#define	VK_INSERT        	   45
#define	VK_DELETE        	   46
#define	VK_HELP          	   47

#define	VK_0             	   48
#define	VK_1             	   49
#define	VK_2             	   50
#define	VK_3             	   51
#define	VK_4             	   52
#define	VK_5             	   53
#define	VK_6             	   54
#define	VK_7             	   55
#define	VK_8             	   56
#define	VK_9             	   57

#define	VK_A             	   65
#define	VK_B             	   66
#define	VK_C             	   67
#define	VK_D             	   68
#define	VK_E             	   69
#define	VK_F             	   70
#define	VK_G             	   71
#define	VK_H             	   72
#define	VK_I             	   73
#define	VK_J             	   74
#define	VK_K             	   75
#define	VK_L             	   76
#define	VK_M             	   77
#define	VK_N             	   78
#define	VK_O             	   79
#define	VK_P             	   80
#define	VK_Q             	   81
#define	VK_R             	   82
#define	VK_S             	   83
#define	VK_T             	   84
#define	VK_U             	   85
#define	VK_V             	   86
#define	VK_W             	   87
#define	VK_X             	   88
#define	VK_Y             	   89
#define	VK_Z             	   90

#define VK_NUMPAD0             96
#define VK_NUMPAD1             97
#define VK_NUMPAD2             98
#define VK_NUMPAD3             99
#define VK_NUMPAD4            100
#define VK_NUMPAD5            101
#define VK_NUMPAD6            102
#define VK_NUMPAD7            103
#define VK_NUMPAD8            104
#define VK_NUMPAD9            105
#define VK_MULTIPLY           106
#define VK_ADD                107
#define VK_SEPARATOR          108
#define VK_SUBTRACT           109
#define VK_DECIMAL            110
#define VK_DIVIDE             111

#define	VK_VOLUME_MUTE   	  173
#define	VK_VOLUME_DOWN   	  174
#define	VK_VOLUME_UP     	  175
#define	VK_MEDIA_NEXT_TRACK	  176
#define	VK_MEDIA_PREV_TRACK	  177
#define	VK_MEDIA_STOP    	  178
#define	VK_MEDIA_PLAY_PAUSE	  179



#define CF_UNICODETEXT      13
#define CF_TEXT      		 1
#define CF_HDROP			15


#define SMART_ATTRIBUTE_TEMPERATURE 194

#define FILE_ATTRIBUTE_READONLY  0x0001
#define FILE_ATTRIBUTE_DIRECTORY 0x0010


#define FD_READ         0x0001
#define FD_WRITE        0x0002
#define FD_OOB          0x0004
#define FD_ACCEPT       0x0008
#define FD_CONNECT      0x0010
#define FD_CLOSE        0x0020


#define SOCKET_CONNECTED	2
#define SOCKET_CONNECTING	1
#define SOCKET_DISCONNECTED	0

#define IPPROTO_TCP     6
#define IPPROTO_UDP    17

#define XML_FAIL			0
#define XML_EAT_WHITESPACE	1
#define XML_CLOSED			2
#define XML_HAPPY			3

#define XML_TAG_AND_BODY	0
#define XML_SINGLE_TAG		1





#define DRIVE_UNKNOWN		0
#define DRIVE_NO_ROOT_DIR	1
#define DRIVE_REMOVABLE		2
#define DRIVE_FIXED			3
#define DRIVE_REMOTE		4
#define DRIVE_CDROM			5
#define DRIVE_RAMDISK		6



#define XTYP_POKE    0x00004090
#define XTYP_REQUEST 0x000020b0
#define XTYP_EXECUTE 0x00004050


#define NONANTIALIASED_QUALITY		3
#define ANTIALIASED_QUALITY			4
#define CLEARTYPE_QUALITY			5
#define CLEARTYPE_NATURAL_QUALITY	6


#define DT_NOCLIP                   0x00000100


#define WM_MOUSEMOVE                    0x0200
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_LBUTTONDBLCLK                0x0203
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_RBUTTONDBLCLK                0x0206
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MBUTTONDBLCLK                0x0209
