#include "../global.h"
#include "../screen.h"
#include "ScriptImage.h"
#include "ScriptObjects.h"
#include "../unicode.h"
#include "../Images/bmp.h"
#include "../vm.h"

unsigned int ImageType;
unsigned int Image32Type;

void LoadImage32(ScriptValue &s, ScriptValue *args) {
	ScriptValue sv;
	if (LoadColorImage(sv, args[0].stringVal->value)) {
		if (CreateObjectValue(s, Image32Type)) {
			s.objectVal->values[0] = sv;
			return;
		}
		sv.stringVal->Release();
	}
}

void LoadMemoryImage32(ScriptValue &s, ScriptValue *args) {
	ScriptValue sv;
	if (LoadMemoryColorImage(sv, args[0].stringVal->value, args[0].stringVal->len)) {
		if (CreateObjectValue(s, Image32Type)) {
			s.objectVal->values[0] = sv;
			return;
		}
		sv.stringVal->Release();
	}
}

void Image32Size(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	if (CreateListValue(s, 3)) {
		GenericImage<unsigned char> *b = (GenericImage<unsigned char>*) o->values[0].stringVal->value;
		ScriptValue sv;
		CreateIntValue(sv, b->width);
		s.listVal->PushBack(sv);
		CreateIntValue(sv, b->height);
		s.listVal->PushBack(sv);
		CreateIntValue(sv, b->spp);
		s.listVal->PushBack(sv);
		return;
	}
	CreateNullValue(s);
}

void Image32SaveBMP(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	GenericImage<unsigned char> *b = (GenericImage<unsigned char>*) o->values[0].stringVal->value;
	CreateIntValue(s, 0);
	wchar_t *file;
	int len = args[0].stringVal->len;
	if (file = UTF8toUTF16Alloc(args[0].stringVal->value, &len)) {
		s.i32 = SaveBMP(_wfopen(file, L"wb"), b);
		free(file);
	}
}

void Image32Zoom(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	GenericImage<unsigned char> *b = (GenericImage<unsigned char> *)o->values[0].stringVal;
	ScriptValue sv;
	if (Zoom(o->values[0], sv, args[0].doubleVal)) {
		if (CreateObjectValue(s, Image32Type)) {
			s.objectVal->values[0] = sv;
			return;
		}
		sv.stringVal->Release();
	}
	CreateNullValue(s);
}

void Image32ToBitImage(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	ScriptValue sv;
	o->values[0].AddRef();
	if (ConvertTo1ColorImage(o->values[0], sv)) {
		if (args[0].doubleVal > 0.00000001) {
			ScriptValue temp;
			if (Zoom(sv, temp, args[0].doubleVal)) {
				sv.Release();
				sv = temp;
			}
		}

		ScriptValue sv2;
		int res = Convert1ColorToBitImage(sv, sv2, args[1].i32);
		sv.stringVal->Release();
		if (res) {
			if (CreateObjectValue(s, ImageType)) {
				s.objectVal->values[0] = sv2;
				return;
			}
			sv2.stringVal->Release();
		}
	}
	CreateNullValue(s);
}


void MyLoadImage(ScriptValue &s, ScriptValue *args) {
	ScriptValue sv;
	if (LoadBitImage(sv, args[0].stringVal->value, args[1].doubleVal, args[2].i32)) {
		if (CreateObjectValue(s, ImageType)) {
			s.objectVal->values[0] = sv;
			return;
		}
		sv.stringVal->Release();
	}
}

void LoadMemoryImage(ScriptValue &s, ScriptValue *args) {
	ScriptValue sv;
	if (LoadMemoryImage(sv, args[0].stringVal->value, args[0].stringVal->len, args[1].doubleVal, args[2].i32)) {
		if (CreateObjectValue(s, ImageType)) {
			s.objectVal->values[0] = sv;
			return;
		}
		sv.stringVal->Release();
	}
}

void IntersectImage(ScriptValue &s, ScriptValue* args) {
	if (args[0].type == SCRIPT_OBJECT && args[0].objectVal->type == ImageType) {
		BitImage* b = (BitImage*) args[0].objectVal->values[0].stringVal->value;
		if (!args[5].i32) args[5].i32 = b->width;
		if (!args[6].i32) args[6].i32 = b->height;
		CreateIntValue(s, activeScreen->IntersectImage(args[1].i32, args[2].i32, args[3].i32, args[4].i32, args[5].i32, args[6].i32, b));
		return;
	}
}

void DrawImage(ScriptValue &s, ScriptValue* args) {
	if (args[0].type == SCRIPT_OBJECT) {
		if (args[0].objectVal->type == ImageType) {
			BitImage* b = (BitImage*) args[0].objectVal->values[0].stringVal->value;
			if (!args[5].i32) args[5].i32 = b->width;
			if (!args[6].i32) args[6].i32 = b->height;
			activeScreen->DisplayImage(args[1].i32, args[2].i32, args[3].i32, args[4].i32, args[5].i32, args[6].i32, b);
			CreateIntValue(s, 1);
		}
		else if (args[0].objectVal->type == Image32Type) {
			GenericImage<unsigned char> *img = (GenericImage<unsigned char> *)args[0].objectVal->values[0].stringVal->value;
			if (!args[5].i32) args[5].i32 = img->width;
			if (!args[6].i32) args[6].i32 = img->height;
			activeScreen->DisplayImage(args[1].i32, args[2].i32, args[3].i32, args[4].i32, args[5].i32, args[6].i32, img);
			CreateIntValue(s, 1);
		}
	}
}

void DrawTransformedImage(ScriptValue &s, ScriptValue* args) {
	if (args[0].type == SCRIPT_OBJECT) {
		if (args[0].objectVal->type == Image32Type) {
			GenericImage<unsigned char> *img = (GenericImage<unsigned char> *)args[0].objectVal->values[0].stringVal->value;
			//activeScreen->DisplayImage(args[1].i32, args[2].i32, args[3].i32, args[4].i32, args[5].i32, args[6].i32, img);
			DoubleQuad src, dst;
			int fullImage = 1;
			for (int i=0; i<4; i++) {
				dst.p[i].x = args[1+2*i].doubleVal;
				dst.p[i].y = args[1+2*i+1].doubleVal;
				src.p[i].x = args[9+2*i].doubleVal;
				src.p[i].y = args[9+2*i+1].doubleVal;
				if (src.p[i].x != 0 || src.p[i].y != 0) fullImage = 0;
			}
			if (fullImage) {
				src.p[0].x = 0;
				src.p[0].y = 0;

				src.p[1].x = img->width-1;
				src.p[1].y = 0;

				src.p[2].x = img->width-1;
				src.p[2].y = img->height-1;

				src.p[3].x = 0;
				src.p[3].y = img->height-1;
			}
			else {
				for (int i=0; i<4; i++) {
					if (src.p[i].x < -0.5) src.p[i].x = -0.5;
					if (src.p[i].x > img->width-0.5) src.p[i].x = img->width-0.5;
					if (src.p[i].y < -0.5) src.p[i].y = -0.5;
					if (src.p[i].y > img->height-0.5) src.p[i].y = img->height-0.5;
				}
			}
			activeScreen->DisplayTransformedImage(&dst, &src, img);
			CreateIntValue(s, 1);
		}
	}
}

void DrawRotatedScaledImage(ScriptValue &s, ScriptValue* args) {
	if (args[0].type == SCRIPT_OBJECT) {
		if (args[0].objectVal->type == Image32Type) {
			GenericImage<unsigned char> *img = (GenericImage<unsigned char> *)args[0].objectVal->values[0].stringVal->value;
			DoubleQuad src, dst;
			double x = args[1].doubleVal;
			double y = args[2].doubleVal;
			double rotate = args[3].doubleVal;
			double scaleX = args[4].doubleVal;
			double scaleY = args[5].doubleVal;
			if (!scaleX) scaleX = 1.0;
			if (!scaleY) scaleY = scaleX;

			src.p[0].x = 0;
			src.p[0].y = 0;

			src.p[1].x = img->width-1;
			src.p[1].y = 0;

			src.p[2].x = img->width-1;
			src.p[2].y = img->height-1;

			src.p[3].x = 0;
			src.p[3].y = img->height-1;

			double rx = scaleX * img->width/2.0;
			double ry = scaleY * img->width/2.0;

			double cx = x + rx;
			double cy = y + ry;
			double cr = cos(rotate);
			double sr = sin(rotate);

			dst.p[0].x = cx - cr*rx + sr*ry-1.0;
			dst.p[1].x = cx + cr*rx + sr*ry-1.0;
			dst.p[2].x = cx + cr*rx - sr*ry;
			dst.p[3].x = cx - cr*rx - sr*ry;

			dst.p[0].y = cy - sr*rx - cr*ry;
			dst.p[1].y = cy + sr*rx - cr*ry;
			dst.p[2].y = cy + sr*rx + cr*ry-1.0;
			dst.p[3].y = cy - sr*rx + cr*ry-1.0;

			activeScreen->DisplayTransformedImage(&dst, &src, img);

			CreateIntValue(s, 1);
		}
	}
}

void InvertImage(ScriptValue &s, ScriptValue* args) {
	if (args[0].type == SCRIPT_OBJECT && args[0].objectVal->type == ImageType) {
		BitImage* b = (BitImage*) args[0].objectVal->values[0].stringVal->value;
		if (!args[5].i32) args[5].i32 = b->width;
		if (!args[6].i32) args[6].i32 = b->height;
		activeScreen->InvertImage(args[1].i32, args[2].i32, args[3].i32, args[4].i32, args[5].i32, args[6].i32, b);
		CreateIntValue(s, 1);
		return;
	}
}

void ClearImage(ScriptValue &s, ScriptValue* args) {
	if (args[0].type == SCRIPT_OBJECT && args[0].objectVal->type == ImageType) {
		BitImage* b = (BitImage*) args[0].objectVal->values[0].stringVal->value;
		if (!args[5].i32) args[5].i32 = b->width;
		if (!args[6].i32) args[6].i32 = b->height;
		activeScreen->ClearImage(args[1].i32, args[2].i32, args[3].i32, args[4].i32, args[5].i32, args[6].i32, b);
		CreateIntValue(s, 1);
		return;
	}
}

void ImageSize(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	if (CreateListValue(s, 2)) {
		BitImage* b = (BitImage*) o->values[0].stringVal->value;
		ScriptValue sv;
		CreateIntValue(sv, b->width);
		s.listVal->Set(sv, 0);
		CreateIntValue(sv, b->height);
		s.listVal->Set(sv, 1);
		return;
	}
	CreateNullValue(s);
}

void ScriptGetClipboardData(ScriptValue &s, ScriptValue *args) {
	int type = args[0].i32;
	int alignment = 0;
	int override = 0;
	if (type) {
		if (!IsClipboardFormatAvailable(type)) {
			CreateIntValue(s, -1);
			return;
		}
		alignment = args[1].i32;
		if (alignment <-1 || alignment == 0 || alignment > 4 || alignment == 3) alignment = 0;
		else override = 1;
	}
	else {
		alignment = 0;
		if (IsClipboardFormatAvailable(CF_DIB)) {
			type = CF_DIB;
		}
		else if (IsClipboardFormatAvailable(CF_HDROP)) {
			type = CF_HDROP;
		}
		else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			type = CF_UNICODETEXT;
		}
		else {
			CreateIntValue(s, -1);
			return;
		}
	}
	if (!alignment) {
		if (type == CF_UNICODETEXT) alignment = 2;
		else alignment = -1;
	}
	if (!OpenClipboard(ghWnd)) {
		return;
	}
	HANDLE h = GetClipboardData(type);
	if (h) {
		unsigned char * data = (unsigned char*) GlobalLock(h);
		if (data) {
			size_t size = GlobalSize(h);
			if (size >= 0 && size <= 100*1024*1024) {
				if (CreateListValue(s, 2)) {
					ScriptValue sv;
					sv.type = SCRIPT_NULL;
					s.listVal->PushBack(type);
					if (!override && type == -9) {
						// Image stuff here.
					}
					else {
						unsigned int len = 0;
						if (alignment == 1) {
							while (len < size && data[len]) len++;
						}
						else if (alignment == 2) {
							while (len < size/2 && ((wchar_t*)data)[len]) len++;
						}
						else if (alignment == -1) {
							len = (int) size;
						}
						else if (alignment == 4) {
							while (len < size/4 && ((int*)data)[len]) len++;
						}
						if (override) {
							CreateStringValue(sv, data, len*alignment);
						}
						else if (type == CF_UNICODETEXT) {
							CreateStringValue(sv, (wchar_t*)data, len);
						}
						else if (type == CF_HDROP && sizeof(HDROP) <= len) {
							HDROP hDrop = (HDROP) data;
							int num = DragQueryFileW(hDrop, -1, 0, 0);
							int size = 12, i;
							for (i=0; i<num; i++) {
								size += DragQueryFileW(hDrop, i, 0, 0) + 3;
							}
							wchar_t *names = (wchar_t*) malloc(size*sizeof(wchar_t));
							if (names) {
								int p = 0;
								for (i=0; i<num; i++) {
									//names[p++] = ' ';
									p += DragQueryFileW(hDrop, i, names+p, size-p-1);
									names[p++] = '\r';
									names[p++] = '\n';
								}
								if (p) CreateStringValue(sv, names, p-2);
								free(names);
							}
						}
						else if (type == CF_DIB) {
							ScriptValue sv2;
							if (LoadMemoryBMP(sv2, (unsigned char*)data, len, 0)) {
								if (CreateObjectValue(sv, Image32Type)) {
									sv.objectVal->values[0] = sv2;
								}
								else {
									sv2.stringVal->Release();
								}
							}
						}
						else {
							CreateStringValue(sv, data, len*alignment);
						}
					}
					if (sv.type != SCRIPT_NULL) {
						s.listVal->PushBack(sv);
					}
					else {
						s.listVal->Release();
						CreateNullValue(s);
					}
				}
			}
			GlobalUnlock(h);
		}
	}
	CloseClipboard();
}

void ScriptSetClipboardData(ScriptValue &s, ScriptValue *args) {
	if (args->type == SCRIPT_LIST && args->listVal->size >= 1) {
		int format = CF_UNICODETEXT;
		ScriptValue sv, sv2;
		if (args->listVal->size > 1) {
			CoerceIntNoRelease(args->listVal->vals[1], sv);
			if (sv.intVal > 0) {
				format = (int) sv.intVal;
			}
		}
		sv = args[0].listVal->vals[0];

		if (!OpenClipboard(ghWnd)) {
			return;
		}

		int happy = 0;

		void *data = 0, *clipData;
		int size = -1;
		if (format == CF_UNICODETEXT) {
			sv.AddRef();
			CoerceString(sv, sv2);
			size = sv2.stringVal->len;
			data = UTF8toUTF16Alloc(sv2.stringVal->value, &size);
			if (!data) size = -1;
			else size = 2*(size + 1);
			sv2.Release();
		}
		if (size >= 0) {
			HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
			if (hGlobal) {
				int happy = 0;
				if (clipData = GlobalLock(hGlobal)) {
					memcpy(clipData, data, size);
					happy = 1;
				}
				GlobalUnlock(clipData);
				if (!happy)
					GlobalFree(hGlobal);
				else {
					EmptyClipboard();
					if (!SetClipboardData(CF_UNICODETEXT, hGlobal))
						GlobalFree(hGlobal);
					else
						CreateIntValue(s, 1);
				}
			}
			free(data);
		}

		CloseClipboard();
	}
}
