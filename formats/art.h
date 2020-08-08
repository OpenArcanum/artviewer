/* ArtConverter by Alexey Stremov https://github.com/AxelStrem/ArtConverter/blob/master/artconverter.cpp
   Refactored for OpenArcanum https://github.com/OpenArcanum/artviewer */

#pragma once

#include <string>
#include <vector>
#include <fstream>

struct MissingFile
{
	std::string filename;
};

struct COLOR_3B
{
	unsigned char b, g, r, a;
};

bool in_palette(COLOR_3B col)
{
	return (col.a | col.b | col.g | col.r) != 0;
}

struct CTABLE_255
{
	COLOR_3B colors[256];
};

typedef COLOR_3B COLOR_4B;
using bytevec = std::vector<unsigned char>;
using bytemap = std::vector<bytevec>;

struct ARTheader
{
	unsigned long    h0[3]; //1,8,8,WTF
	COLOR_4B stupid_color[4];

	unsigned long    frame_num_low;
	unsigned long    frame_num;

	COLOR_4B palette_data1[8];
	COLOR_4B palette_data2[8];
	COLOR_4B palette_data3[8];
};

struct ARTFrameHeader
{
	unsigned long width;
	unsigned long height;
	unsigned long size;
	int c_x;
	int c_y;
	int d_x;
	int d_y;
};

struct ArtFrame
{
	ARTFrameHeader header;
	char*          data;
	bytemap        pixels;
	int px, py;

	auto Inc() -> bool;
	auto Dec() -> void;
	auto Reset() -> void;
	auto EOD() -> bool;

	auto GetHeader() -> ARTFrameHeader& { return header; }
	auto LoadHeader(std::ifstream& source) -> void;
	auto SaveHeader(std::ofstream& dest) -> void;

	auto Load(std::ifstream& source) -> void;
	auto Save(std::ofstream& source) -> void;

	auto GetValue(int x, int y) -> unsigned char;
	auto GetValueI(int x, int y) -> unsigned char;
	auto SetValue(int x, int y, unsigned char ch) -> void;
	auto SetSize(int w, int h) -> void;

	auto Encode() -> void;
	auto Decode() -> void;
};

struct bmpHeader
{
	unsigned short    bfType;
	unsigned long   bfSize;
	unsigned short    bfReserved1;
	unsigned short    bfReserved2;
	unsigned long   bfOffBits;
};

struct bmpInfoHeader
{
	unsigned long  biSize;
	long           biWidth;
	long           biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long  biCompression;
	unsigned long  biSizeImage;
	long           biXPelsPerMeter;
	long           biYPelsPerMeter;
	unsigned long  biClrUsed;
	unsigned long  biClrImportant;
};

struct ArtFile
{
	ARTheader header;
	std::vector<ArtFrame> frame_data;
	std::vector<CTABLE_255> palette_data;

	int palettes;
	int frames;
	int key_frame;
	bool animated;

	bmpHeader hdr;
	bmpInfoHeader ihdr;

	auto LoadArt(const std::string &fname) -> void;
	auto SaveArt(const std::string &fname) -> void;

	auto LoadBMPS(const std::string &fname) -> void;
	auto SaveBMPS(const std::string &fname) -> void;

	auto LoadDWORD(std::ifstream& dst, unsigned long &data) -> void;
	auto SaveDWORD(std::ofstream& dst, unsigned long data) -> void;
	
	auto LoadCOLOR(std::ifstream& dst, COLOR_4B &data) -> void;
	auto SaveCOLOR(std::ofstream& dst, COLOR_4B col) -> void;
	
	auto LoadHeader(std::ifstream& dst, ARTheader& h) -> void;
	auto SaveHeader(std::ofstream& dst, ARTheader& h) -> void;
};
