/* ArtConverter by Alexey Stremov https://github.com/AxelStrem/ArtConverter/blob/master/artconverter.cpp
   Refactored for OpenArcanum https://github.com/OpenArcanum/artviewer */

#include "formats/art.h"

#include <iostream>
#include <sstream>


auto ArtFrame::Inc() -> bool
{
	px++;
	if (px >= static_cast<int>(header.width))
	{
		px = 0;
		py++;
	}
	if (py >= static_cast<int>(header.height)) return false;
	if (py < 0) return false;
	return true;
}

auto ArtFrame::Dec() -> void
{
	px--;
	if (px < 0)
	{
		px = header.width - 1;
		py--;
	}
}

auto ArtFrame::Reset() -> void
{
	px = py = 0;
}

auto ArtFrame::EOD() -> bool
{
	if (py < static_cast<int>(header.height)) return false;
	return true;
}

auto ArtFrame::LoadHeader(std::ifstream& source) -> void
{
	source.read(reinterpret_cast<char*>(&header), sizeof(header));
}

auto ArtFrame::SaveHeader(std::ofstream& dest) -> void
{
	dest.write(reinterpret_cast<char*>(&header), sizeof(header));
}

auto ArtFrame::Load(std::ifstream& source) -> void
{
	data = new char[header.size];
	source.read(data, header.size);
}

auto ArtFrame::Save(std::ofstream& source) -> void
{
	source.write(data, header.size);
}

auto ArtFrame::GetValue(int x, int y) -> unsigned char
{
	return pixels[y][x];
}

auto ArtFrame::GetValueI(int x, int y) -> unsigned char
{
	return pixels[pixels.size() - 1 - y][x];
}

auto ArtFrame::SetValue(int x, int y, unsigned char ch) -> void
{
	pixels[y][x] = ch;
}

auto ArtFrame::SetSize(int w, int h) -> void
{
	header.width = w;
	header.height = h;
	pixels = bytemap(header.height, bytevec(header.width));
}

auto ArtFrame::Encode() -> void
{
	std::string data_compressed;
	std::string data_raw;

	Reset();
	do
	{
		char clones = 0;
		char val = GetValueI(px, py);
		if (!Inc())
		{
			data_compressed += static_cast<unsigned char>(0x81);
			data_compressed += val;
		}
		else
		{
			if (val == GetValueI(px, py))
			{
				clones = 2;
				while (Inc() && (val == GetValueI(px, py)) && (clones < 0x7F))
				{
					clones++;
				}
				data_compressed += clones;
				data_compressed += val;
			}
			else
			{
				clones = 2;
				data_compressed += '\0';
				data_compressed += val;
				data_compressed += GetValueI(px, py);
				while (Inc() && (GetValueI(px, py) != data_compressed.back()) && (clones < 0x7F))
				{
					data_compressed += GetValueI(px, py);
					clones++;
				}
				if ((!EOD()) && (GetValueI(px, py) == data_compressed.back()))
				{
					clones--;
					data_compressed.resize(data_compressed.size() - 1);
					Dec();
				}
				data_compressed[data_compressed.size() - clones - 1] = static_cast<unsigned char>(0x80) | static_cast<unsigned char>(clones);
			}
		}
	} while (!EOD());

	Reset();
	while (!EOD())
	{
		data_raw += GetValueI(px, py);
		Inc();
	}
	Reset();

	if (data_raw.size() <= data_compressed.size())
	{
		data = new char[data_raw.size()];
		memcpy(data, data_raw.c_str(), data_raw.size());
		header.size = static_cast<unsigned long>(data_raw.size());
	}
	else
	{
		data = new char[data_compressed.size()];
		memcpy(data, data_compressed.c_str(), data_compressed.size());
		header.size = static_cast<unsigned long>(data_compressed.size());
	}
}

auto ArtFrame::Decode() -> void
{
	pixels = bytemap(header.height, bytevec(header.width));
	Reset();
	if (header.size < (header.height*header.width))
	{
		for (int p = 0; p < static_cast<int>(header.size); p++)
		{
			unsigned char ch = static_cast<unsigned char>(data[p]);
			if (ch & 0x80)
			{
				int to_copy = ch & (0x7F);
				while (to_copy--)
				{
					p++;
					pixels[py][px] = data[p];
					Inc();
				}
			}
			else
			{
				int to_clone = ch & (0x7F);
				p++;
				unsigned char src = static_cast<unsigned char>(data[p]);
				while (to_clone--)
				{
					pixels[py][px] = src;
					Inc();
				}
			}
		}
	}
	else
	{
		for (int p = 0; p < static_cast<int>(header.size); p++)
		{
			pixels[py][px] = data[p];
			Inc();
		}
	}
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

auto ArtFile::LoadArt(const std::string &fname) -> void
{
	std::ifstream source;
	source.open(fname, std::ios_base::binary);

	if (!source)
		throw MissingFile{ fname };

	source.read(reinterpret_cast<char*>(&header), sizeof(header));

	animated = ((header.h0[0] & 0x1) == 0);

	palettes = 0;
	for (auto col : header.stupid_color)
	{
		if (in_palette(col)) palettes++;
	}
	frames = header.frame_num;
	key_frame = header.frame_num_low;

	if (animated) frames *= 8;

	for (int i = 0; i < palettes; i++)
	{
		palette_data.push_back(CTABLE_255());
		source.read(reinterpret_cast<char*>(&palette_data.back()), sizeof(CTABLE_255));
	}

	for (int i = 0; i < frames; i++)
	{
		frame_data.push_back(ArtFrame());
		frame_data.back().LoadHeader(source);
	}

	for (auto &af : frame_data)
	{
		af.Load(source);
		af.Decode();
	}
}

auto ArtFile::SaveArt(const std::string &fname) -> void
{
	std::ofstream source;
	source.open(fname, std::ios_base::binary);
	source.write(reinterpret_cast<char*>(&header), sizeof(header));

	for (int i = 0; i < palettes; i++)
	{
		source.write(reinterpret_cast<char*>(&palette_data[i]), sizeof(CTABLE_255));
	}

	for (auto &af : frame_data)
	{
		af.Encode();
		af.SaveHeader(source);
	}

	for (auto &af : frame_data)
	{
		af.Save(source);
	}

	source.close();
}

auto ArtFile::LoadBMPS(const std::string &fname) -> void
{
	std::ifstream ctrl;
	ctrl.open(fname);

	if (!ctrl)
		throw MissingFile{ fname };

	std::string str;
	ctrl >> str;
	ctrl >> frames;
	header.frame_num = frames;
	ctrl >> str;
	ctrl >> key_frame;
	header.frame_num_low = key_frame;
	ctrl >> str;
	ctrl >> palettes;

	ctrl >> str;//header:

	LoadHeader(ctrl, header);

	animated = ((header.h0[0] & 0x1) == 0);
	if (animated)
		header.frame_num /= 8;

	palette_data.resize(palettes);

	for (int i = 0; i < palettes; i++)
	{
		ctrl >> str; //palette
		ctrl >> str; //palette number
		for (auto &c : palette_data[i].colors)
		{
			LoadCOLOR(ctrl, c);
		}
	}

	frame_data.resize(frames);
	for (int i = 0; i < frames; i++)
	{
		ctrl >> str; //frame
		ctrl >> str; //frame number
		ctrl >> str; //center_x
		ctrl >> frame_data[i].GetHeader().c_x;
		ctrl >> str; //center_y
		ctrl >> frame_data[i].GetHeader().c_y;

		ctrl >> str; //offset_x
		ctrl >> frame_data[i].GetHeader().d_x;
		ctrl >> str; //offset_y
		ctrl >> frame_data[i].GetHeader().d_y;
	}

	ctrl.close();

	std::string base_name = fname.substr(0, fname.size() - 4);
	int frame_num = 0;
	for (auto &af : frame_data)
	{
		std::ostringstream oss;
		if (!animated)
			oss << base_name << "_" << frame_num << ".bmp";
		else
			oss << base_name << "_" << (frame_num / 8) << (frame_num % 8) << ".bmp";
		frame_num++;

		std::ifstream src;
		src.open(oss.str(), std::ios_base::binary);

		if (src)
		{
			src.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
			src.read(reinterpret_cast<char*>(&ihdr), sizeof(ihdr));

			CTABLE_255 ignore;
			src.read(reinterpret_cast<char*>(&ignore), sizeof(ignore));

			int offset = sizeof(hdr) + sizeof(ihdr) + sizeof(ignore);
			unsigned char ch;

			int height = ihdr.biHeight;
			int width = ihdr.biWidth;
			//int offbits = hdr.bfOffBits;

			int stride = ((width + 3) / 4) * 4;

			while (offset < static_cast<int>(hdr.bfOffBits))
			{
				offset++;
				src.read(reinterpret_cast<char*>(&ch), 1);
			}

			af.SetSize(width, height);

			for (int i = 0; i < height; i++)
			{
				for (int j = 0; j < width; j++)
				{
					src.read(reinterpret_cast<char*>(&ch), 1);
					af.SetValue(j, i, ch);
				}
				offset = width;
				while (offset < stride)
				{
					offset++;
					src.read(reinterpret_cast<char*>(&ch), 1);
				}
			}

			src.close();
		}
		else
		{
			af.SetSize(6, 6);
			for (int i = 0; i < 6; i++)
			{
				for (int j = 0; j < 6; j++)
				{
					af.SetValue(j, i, 0);
				}
			}
		}
	}
}

auto ArtFile::SaveDWORD(std::ofstream& dst, unsigned long data) -> void
{
	for (int i = 0; i < 8; i++)
	{
		unsigned char ch = (data >> (i * 4)) & 0xF;
		ch = "0123456789ABCDEF"[ch];
		dst << ch;
	}
	dst << " ";
}

auto ArtFile::LoadDWORD(std::ifstream& dst, unsigned long &data) -> void
{
	std::string str;
	dst >> str;

	std::reverse(str.begin(), str.end());

	unsigned long res = 0;

	for (int i = 0; i < 8; i++)
	{
		res <<= 4;
		unsigned char ch = str[i];
		switch (ch)
		{
		case '0': res += 0; break;
		case '1': res += 1; break;
		case '2': res += 2; break;
		case '3': res += 3; break;
		case '4': res += 4; break;
		case '5': res += 5; break;
		case '6': res += 6; break;
		case '7': res += 7; break;
		case '8': res += 8; break;
		case '9': res += 9; break;
		case 'A': res += 10; break;
		case 'B': res += 11; break;
		case 'C': res += 12; break;
		case 'D': res += 13; break;
		case 'E': res += 14; break;
		case 'F': res += 15; break;
		default:return;
		}
	}
	data = res;
}

auto ArtFile::LoadCOLOR(std::ifstream& dst, COLOR_4B &data) -> void
{
	unsigned long d;
	LoadDWORD(dst, d);
	data.a = static_cast<unsigned char>((d & 0xFF000000) >> 24);
	data.b = static_cast<unsigned char>((d & 0x00FF0000) >> 16);
	data.g = static_cast<unsigned char>((d & 0x0000FF00) >> 8);
	data.r = static_cast<unsigned char>((d & 0x000000FF) >> 0);
}

auto ArtFile::SaveCOLOR(std::ofstream& dst, COLOR_4B col) -> void
{
	unsigned long d = (col.a << 24) | (col.b << 16) | (col.g << 8) | col.r;
	SaveDWORD(dst, d);
}

auto ArtFile::SaveHeader(std::ofstream& dst, ARTheader& h) -> void
{
	for (int i = 0; i < 3; i++)
		SaveDWORD(dst, h.h0[i]);
	for (int i = 0; i < 4; i++)
		SaveCOLOR(dst, h.stupid_color[i]);
	for (int i = 0; i < 8; i++)
		SaveCOLOR(dst, h.palette_data1[i]);
	for (int i = 0; i < 8; i++)
		SaveCOLOR(dst, h.palette_data2[i]);
	for (int i = 0; i < 8; i++)
		SaveCOLOR(dst, h.palette_data3[i]);
}

auto ArtFile::LoadHeader(std::ifstream& dst, ARTheader& h) -> void
{
	for (int i = 0; i < 3; i++)
		LoadDWORD(dst, h.h0[i]);
	for (int i = 0; i < 4; i++)
		LoadCOLOR(dst, h.stupid_color[i]);
	for (int i = 0; i < 8; i++)
		LoadCOLOR(dst, h.palette_data1[i]);
	for (int i = 0; i < 8; i++)
		LoadCOLOR(dst, h.palette_data2[i]);
	for (int i = 0; i < 8; i++)
		LoadCOLOR(dst, h.palette_data3[i]);
}

auto ArtFile::SaveBMPS(const std::string &fname) -> void
{
	std::ostringstream gss;
	gss << fname << ".ini";
	std::ofstream ctrl;
	ctrl.open(gss.str());
	ctrl << "frames: " << frames << "\r\n";
	ctrl << "key_frame: " << key_frame << "\r\n";
	ctrl << "palettes: " << palettes << "\r\n";

	ctrl << "header: \r\n";
	SaveHeader(ctrl, header);

	ctrl << "\r\n";
	for (int i = 0; i < palettes; i++)
	{
		ctrl << "palette " << i << ":\r\n";
		for (auto c : palette_data[i].colors)
		{
			SaveCOLOR(ctrl, c);
			ctrl << "\r\n";
		}
	}

	for (int i = 0; i < frames; i++)
	{
		if (animated)
			ctrl << "frame " << (i / 8) << "_" << (i % 8) << ":\r\n";
		else
			ctrl << "frame " << i << ":\r\n";
		ctrl << "center_x: " << frame_data[i].GetHeader().c_x << "\r\n";
		ctrl << "center_y: " << frame_data[i].GetHeader().c_y << "\r\n";
		ctrl << "offset_x: " << frame_data[i].GetHeader().d_x << "\r\n";
		ctrl << "offset_y: " << frame_data[i].GetHeader().d_y << "\r\n";
	}

	ctrl.close();

	int frame_num = 0;
	for (auto& af : frame_data)
	{
		std::ostringstream oss;
		if (!animated)
			oss << fname << "_" << frame_num << ".bmp";
		else
			oss << fname << "_" << (frame_num / 8) << (frame_num % 8) << ".bmp";
		frame_num++;

		hdr.bfOffBits = sizeof(hdr) + sizeof(ihdr) + sizeof(CTABLE_255);
		hdr.bfReserved1 = 28020;
		hdr.bfReserved2 = 115;

		int stride = ((af.GetHeader().width + 3) / 4) * 4;

		hdr.bfSize = sizeof(hdr) + sizeof(ihdr) + sizeof(CTABLE_255) + af.GetHeader().height*stride - 40;
		hdr.bfType = 19778;

		ihdr.biBitCount = 8;
		ihdr.biClrImportant = 0;
		ihdr.biClrUsed = 0;
		ihdr.biCompression = 0;
		ihdr.biHeight = af.GetHeader().height;
		ihdr.biPlanes = 1;
		ihdr.biSize = 40;
		ihdr.biWidth = af.GetHeader().width;

		std::ofstream dst;
		dst.open(oss.str(), std::ios_base::binary);
		dst.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
		dst.write(reinterpret_cast<char*>(&ihdr), sizeof(ihdr));

		dst.write(reinterpret_cast<char*>(&palette_data[0]), sizeof(CTABLE_255));

		int offset = sizeof(hdr) + sizeof(ihdr) + sizeof(CTABLE_255);

		char ch = 0;
		while (offset < static_cast<int>(hdr.bfOffBits))
		{
			offset++;
			dst.write(&ch, 1);
		}

		for (int y = af.GetHeader().height - 1; y >= 0; y--)
		{
			for (int x = 0; x < static_cast<int>(af.GetHeader().width); x++)
			{
				ch = af.GetValue(x, y);
				dst.write(&ch, 1);
			}
			ch = 0;
			offset = static_cast<int>(af.GetHeader().width);
			while (offset < stride)
			{
				offset++;
				dst.write(&ch, 1);
			}
		}

		dst.close();
	}
}

/*
void Draw(int p_num, int f_num)
{
	HDC hdc = GetDC(NULL);
	ArtFrame& af = frame_data[f_num];
	CTABLE_255& ap = palette_data[p_num];
	for (int y = 0; y < static_cast<int>(af.GetHeader().height); y++)
		for (int x = 0; x < static_cast<int>(af.GetHeader().width); x++)
		{
			BYTE b = af.GetValue(x, y);
			SetPixelV(hdc, x, y, RGB(ap.colors[b].r, ap.colors[b].g, ap.colors[b].b));
		}

}

int main(int argc, char* argv[])
{
	ArtFile af;

	if (argc < 2)
	{
		std::cout << "please specify target file path\r\n";
		return 0;
	}

	string src_name = argv[1];

	string dst_name;
	if (argc < 3)
	{
		return 0;
	}

	dst_name = argv[2];

	try
	{

		string ext = src_name.substr(src_name.size() - 3, 3);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		if (ext == std::string("art"))
		{
			af.LoadArt(src_name);
			af.SaveBMPS(dst_name);
		}
		else
		{
			af.LoadBMPS(src_name);
			af.SaveArt(dst_name);
		}
	}
	catch (MissingFile mf)
	{
		std::cout << "Missing file : " << mf.filename << "\r\n";
	}
}
*/
