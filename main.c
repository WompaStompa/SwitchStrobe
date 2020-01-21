#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#ifdef DISPLAY_IMAGE
#include "image_bin.h"
#endif

#define FB_WIDTH  1280
#define FB_HEIGHT 720

#define MAX_COL 50 //maximum number of custom colors

/*
Remove above and uncomment below for 1080p
#define FB_WIDTH  1920
#define FB_HEIGHT 1080

>1080p strobing

*/
int main(int argc, char* argv[])
{
	NWindow* win = nwindowGetDefault();
	
	Framebuffer fb;
	framebufferCreate(&fb, win, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
	framebufferMakeLinear(&fb);

#ifdef DISPLAY_IMAGE
	u8* imageptr = (u8*)image_bin;
#endif

	FILE *colorFile = fopen("colors.txt", "r"); //try to open the file

	int lines = 8;
	bool colorExist = false; //used for logic depending on the existence of the file

	if (colorFile != NULL) { //count number of lines in file if it exists
		lines = 1;
		colorExist = true;
		int chr = fgetc(colorFile); //fgetc is an int, not char. 
		while ((chr != EOF) && (lines < MAX_COL)) { //go through file characterwise and see if any are newlines until End or MAX_COL, whichever is first
			if (chr == '\n') {																					//    Of
				lines++;																						//    File
			}
			chr = fgetc(colorFile);
		}
		fseek(colorFile, 0, SEEK_SET); //reset file stream pointer thingy's position to the beginning of the file
	}
	
	int colors[lines]; //assume number of lines equals number of colors, make array of that dimension

	if (colorExist) { //if the file exists, use its values. if not, use generic color array
		for (int i = 0; i < lines; i++) { //define each element as a line from the file one at a time
			fscanf(colorFile, "%x ", &colors[i]); // %x for hex, the space after is for newline detection
		}
	} else {
		int a, b, j;
		for (int i = 0; i < lines; i++) { //generate a value for each element one at a time
			j = i + 1;
			a = (i%2 + (i/2)%2 + ((i/4)*i)%3 + i/6 + i/4) % 2; //0-7 maps to 0, 1, 1, 0, 0, 0, 1, 1
			b = (j%2 + (j/2)%2 + ((j/4)*j)%3 + j/6 + j/4 + 1) % 2; //0-7 maps to 0, 0, 1, 1, 1, 0, 0, 1
			colors[i] = 0x11*a + 0x1100*b + 0x110000*(i/4); //integer division is just truncate/floor round
		}
	}

	fclose(colorFile); //close the file

	u32 cnt = 0;
	int duration = 10; //number of frames each color is active for
	int colSize = sizeof(colors) / sizeof(colors[0]); //how many elements in color array, could probably get away with just using the 'lines' variable from earlier
	int col1 = 1; //first color's index
	int col2 = 3; //second color's index
	int bright1 = 15; //how bright first color is, only used with generic color array
	int bright2 = 15; //how bright second color is, only used with generic color array
	int isOn = 1; //pseudo bool used for switching between first and second color
	int stripes = 1; //pseudo bool used for checking whether to make stripes or not
	int b; //pseudo bool used for determining where to place stripes
	float xIntercept = 80; //point on x-axis used to determine where stripes are placed, spacing, regularity, etc. has to be float to avoid integer division truncation
	float yIntercept = 60; //point on y-axis used to determine where stripes are placed, spacing, regularity, etc. has to be float to avoid integer division truncation
	int scroll = 0; //timer used to scroll stripes
		
	while (appletMainLoop())
	{
		hidScanInput();

		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO); //switch player one controller input

		if (kDown & KEY_PLUS) { //safely exit app
			break;
		}

		switch (kDown) {
		case KEY_MINUS: //reset timing values
			duration = 10;
			cnt = 0;
			break;
		case KEY_DUP: //increase color duration
			if (duration < 60) { duration++; }
			break;
		case KEY_DDOWN: //decrease color duration
			if (duration > 1) {	duration--; }
			break;
		case KEY_DRIGHT: //reset color one brightness
			bright1 = 15;
			break;
		case KEY_DLEFT: //reset color two brightness
			bright2 = 15;
			break;
		case KEY_A: //cycle color one positively through array
			col1 = (col1 + 1) % colSize;
			break;
		case KEY_Y: //cycle color one negatively through array
			col1 = (col1 - 1 + colSize) % colSize;
			break;
		case KEY_X: //cycle color two positively through array
			col2 = (col2 + 1) % colSize;
			break;
		case KEY_B: //cycle color two negatively through array
			col2 = (col2 - 1 + colSize) % colSize;
			break;
		case KEY_R: //increase color one brightness
			if (bright1 < 15) { bright1++; }
			break;
		case KEY_ZR: //decrease color one brightness
			if (bright1 > 0) { bright1--; }
			break;
		case KEY_L: //increase color two brightness
			if (bright2 < 15) { bright2++; }
			break;
		case KEY_ZL: //decrease color two brightness
			if (bright2 > 0) { bright2--; }
			break;
		case KEY_DLEFT + KEY_DRIGHT: //toggle stripes
			stripes = (stripes + 1) % 2;
			break;
		}

		u32 stride;
		u32* framebuf = (u32*)framebufferBegin(&fb, &stride);

		cnt = (cnt + 1) % (duration + 1); //frame counter, used to determine when to change colors
		scroll = (scroll + 1) % 120;
		if (cnt == 0) { //change colors
			isOn = (isOn + 1) % 2;
			//col1 = (col1 + 1) % colSize;
		}

		for (u32 y = 0; y < FB_HEIGHT; y++)
		{
			for (u32 x = 0; x < FB_WIDTH; x++)
			{
				u32 pos = ((y - scroll + 120) % FB_HEIGHT) * stride / sizeof(u32) + x;
#ifdef DISPLAY_IMAGE
				framebuf[pos] = RGBA8_MAXALPHA(imageptr[pos * 3 + 0] + (cnt * 4), imageptr[pos * 3 + 1], imageptr[pos * 3 + 2]);
#else			
				
				b = x / xIntercept + y / yIntercept; //finds b-value for each point in y = b - (yIntercept/xIntercept)*x. because b is integer, this results in a sequence of diagonal bars
				b %= 2; //classifies each diagonal bar in to one of two groups based on whether its b-value is even or odd
				/*
				the following determines which color to put at each point by multiplying one color by 1, the other by 0, and then adding these products
				this results in something like "color at this (x,y)" = 0*(one color) + 1*(other color) = other color
				determining whether to multiply by 1 or 0 depends on the variables isOn, stripes, b
				for example, if isOn=0, stripes=1, and b=1, then i want to multiply col1 by 1 and col2 by 0
				the "logic" goes; color 1 isn't supposed to be shown on this frame, but i want stripes and this position is in a suitable diagonal bar, so let's show color 1 here
				you can apply similar "logic" to the other 7 cases
				to do this in general, i multiply col1 by isOn XOR (stripes AND b) and multiply col2 by (NOT isON) XOR (stripes AND b)
				*/
				if (colorExist) { framebuf[pos] = (isOn ^ (stripes & b))*colors[col1] + ((!isOn) ^ (stripes & b))*colors[col2]; }
				else { framebuf[pos] = (isOn ^ (stripes & b))*colors[col1]*bright1 + ((!isOn) ^ (stripes & b))*colors[col2]*bright2; }
				/*
				      input      ||   output
				=================||===========
				isOn |stripes| b ||*col1|*col2
				=====|=======|===||=====|=====
				  0  |   0   | 0 ||  0  |  1
				  0  |   0   | 1 ||  0  |  1
				  0  |   1   | 0 ||  0  |  1
				  0  |   1   | 1 ||  1  |  0
				=====|=======|===||=====|=====
				  1  |   0   | 0 ||  1  |  0
				  1  |   0   | 1 ||  1  |  0
				  1  |   1   | 0 ||  1  |  0
				  1  |   1   | 1 ||  0  |  1
				*/
				//framebuf[pos] = colors[col1] * bright1;
#endif
			}
		}
		
		framebufferEnd(&fb);
	}

	framebufferClose(&fb);
	return 0;
}