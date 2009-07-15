#include "Images.h"
//#include <windows.h>
//#include <gl\gl.h>

#ifdef CHICKEN
int ScreenShotGL(const char *path, int w, int h, int format) {
	if (!path) path = "";
	unsigned int len = 0;
	while (path[len]) len++;
	char *fileName = (char*) malloc((len+32)*sizeof(char));
	if (!fileName) return 0;
	int i, j;
	if (len) {
		len=0;
		while (path[len]) {
			fileName[len]=path[len];
			len++;
		}
		fileName[len++] = '\\';
	}
	strcpy(&fileName[len], "ScreenShot");
	/*	sprintf(fileName, "%s\\ScreenShot", path);
	else
		sprintf(fileName, "ScreenShot");
		*/
	while(fileName[len]) len++;
	for (i=0; i<10000;i++) {
		j=1000;
		int k=0;
		while (j) {
			fileName[len+k] = '0'+((i/j)%10);
			j/=10;
			k++;
		}
		sprintf(&fileName[len+k], ".png");
		FILE *f = fopen(fileName, "rb");
		if (f) {
			fclose(f);
			continue;
		}
		sprintf(&fileName[len+k], ".bmp");
		f = fopen(fileName, "rb");
		if (f) {
			fclose(f);
			continue;
		}
		if (format==0)
			sprintf(&fileName[len+k], ".png");

		f = fopen(fileName, "wb");
		if (!f) continue;
		glFinish();
		glReadBuffer(GL_BACK);
		if (format==0) {
			Image *image = new Image(w, h, 3);
			unsigned char *pixels = (unsigned char*) malloc(sizeof(unsigned char)*h*(w+3)*3);
			glReadPixels(0,0,w,h,GL_RGB, GL_UNSIGNED_BYTE, pixels);
			int w2 = w*3;
			//note that each row is 4-byte aligned
			if (w2%4) w2 += 4-(w2%4);
			for (int i=h-1; i>=0; i--) {
				//int iSource = h-1-i;
				//int p1 = i*w;
				//int p2 = 0;
				int p1 = i*w;
				int p2 = (h-1-i)*w2;
				for (int j=0; j<w; j++) {
					((Color3*)image->pixels)[p1] = ((Color3*)&pixels[p2])[0];
					p1++;
					p2+=3;
				}
			}
			/*for (int i=h/2; i>=0; i--) {
				int p1 = i*w;
				int p2 = (h-1-i)*w;
				for (int j=0; j<w; j++) {
					//int p1 = j+i*w;
					//int p2 = j+(h-1-i)*w;
					Color3 temp = image->pixels[p1];
					image->pixels[p1++] = image->pixels[p2];
					image->pixels[p2++] = temp;
				}
			}*/
			((int*)(image->pixels+w*h))[0] = 0;
			((int*)(image->pixels+w*h))[1] = 0;
			((int*)(image->pixels+w*h))[2] = 0;
			((int*)(image->pixels+w*h))[3] = 0;
			//memset(&image->pixels[w*h], 0, 15);
			SavePNG(f, image);
			delete image;
			free(pixels);
		}
		else {	
			ImageRGBA *image = new ImageRGBA(w, h);
			//unsigned char *pixels = (unsigned char*) malloc(sizeof(unsigned char)*h*w*4);
			glReadPixels(0,0,w,h,GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
			for (int i=h/2; i>=0; i--) {
				int p1 = i*w;
				int p2 = (h-1-i)*w;
				for (int j=0; j<w; j++) {
					//int p1 = j+i*w;
					//int p2 = j+(h-1-i)*w;
					Color4 temp = image->pixels[p1];
					image->pixels[p1++] = image->pixels[p2];
					image->pixels[p2++] = temp;
				}
			}
			SaveBMP(f, image);
			delete image;
		}
		fclose(f);
		free(fileName);
		return 1;
	}
	free(fileName);
	return 0;
}
#endif

