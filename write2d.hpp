
#pragma once

#include <stdexcept>
#include <cstddef>
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <type_traits>

#include "surface.hpp"

#include <png.h>

#define throw(...)
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>
#undef throw

// Writes EXR
inline void write_2d_hdr(const std::filesystem::path &path, const surface<float> &s) {
	using namespace Imf;

	Rgba *pixels = new Rgba[s.width*s.height];
	for (size_t p=0; p<s.width*s.height; ++p) {
		using T = std::remove_cv_t<decltype(pixels[p].r)>;

		pixels[p] = {};
		pixels[p].r = static_cast<T>(s.data[p*s.components]);
		if (s.components >= 3) {
			pixels[p].g = static_cast<T>(s.data[p*s.components + 1]);
			pixels[p].b = static_cast<T>(s.data[p*s.components + 2]);
		}
		if (s.components == 4) {
			pixels[p].a = static_cast<T>(s.data[p*s.components + 3]);
		}
	}

	RgbaOutputFile file(path.string().c_str(), s.width, s.height, s.components>=3 ? WRITE_RGBA : WRITE_Y);
	file.setFrameBuffer(pixels, 1, s.width);
	file.writePixels(s.height);
}

template <typename T>
void write_2d_hdr_convert(const std::filesystem::path &path, const surface<T> &surf) {
	if constexpr (std::is_same_v<T,float>) {
		write_2d_hdr(path,surf);
		return;
	}
	
	auto s = create_surface<float>(surf.width, surf.height, surf.components);
	for (std::size_t i=0;i<s.width*s.height*s.components;++i)
		s.data[i] = (float)surf.data[i];
	write_2d_hdr(path, s);
}

// Writes PNG
inline void write_2d(const std::filesystem::path &path, const surface<std::uint8_t> &s) {
	const auto& file_name = path.string();

	const uint8_t *image_data = s.data.get();
	int components = s.components, width = static_cast<int>(s.width), height = static_cast<int>(s.height);

	FILE *fp = fopen(file_name.c_str(), "wb");
	if (!fp) {
		std::cerr << file_name << " can't be opened for writing" << std::endl;
		throw std::runtime_error("PNG save error");
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) {
		std::cerr << file_name << " png_create_write_struct failed" << std::endl;
		fclose(fp);
		throw std::runtime_error("PNG save error");
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		std::cerr << file_name << " png_create_info_struct failed" << std::endl;
		fclose(fp);
		throw std::runtime_error("PNG save error");
	}

	if (setjmp(png_jmpbuf(png))) {
		std::cerr << file_name << " png_jmpbuf failed" << std::endl;
		fclose(fp);
		throw std::runtime_error("PNG save error");
	}

	png_byte ** const row_pointers = reinterpret_cast<png_byte **>(malloc(height * sizeof(png_byte *)));
	if (row_pointers == nullptr) {
		std::cerr << file_name << " could not allocate memory for PNG row pointers" << std::endl;
		fclose(fp);
		throw std::runtime_error("PNG save error");
	}

	// set the individual row_pointers to point at the correct offsets of image_data
	// To maintain compatibility png_write_image requests a non-const double pointer, hack the const away...
	for (int i = 0; i < height; i++)
		row_pointers[i] = const_cast<png_byte*>(image_data + i * width * components);

	png_init_io(png, fp);

	int color_type;
	switch (components) {
	case 1: color_type = PNG_COLOR_TYPE_GRAY; break;
	case 3: color_type = PNG_COLOR_TYPE_RGB; break;
	case 4: color_type = PNG_COLOR_TYPE_RGBA; break;
	default: {
		std::cerr << "Invalid component count" << std::endl;
		throw std::runtime_error("PNG save error");
	}
	}
	png_set_IHDR(png,
		info,
		width, height,
		8,
		color_type,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);

	png_write_image(png, row_pointers);
	png_write_end(png, nullptr);

	free(row_pointers);
	fclose(fp);
}

