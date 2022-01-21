
#pragma once

#include <stdexcept>
#include <cstddef>
#include <iostream>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <vector>

#include "surface.hpp"

#include <png.h>

#define throw(...)
#include <OpenEXR/OpenEXRConfig.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>
#undef throw

surface<f_t> load_2d_hdr(const std::filesystem::path &path) {
	using namespace Imf;

	surface<f_t> out;

	RgbaInputFile file(path.string().c_str());
	const auto& dw = file.dataWindow();
	out.width  = static_cast<std::size_t>(dw.max.x - dw.min.x + 1);
	out.height = static_cast<std::size_t>(dw.max.y - dw.min.y + 1);
	out.components = !!(file.channels()&WRITE_RGBA) ? 4 :
					 !!(file.channels()&WRITE_RGB)  ? 3 :
					 1;

	std::vector<Rgba> pixels;
	pixels.resize(out.width*out.height);
	file.setFrameBuffer(pixels.data(), 1, out.width);
	file.readPixels(dw.min.y, dw.max.y);

	out.data = std::make_unique<f_t[]>(out.width*out.height*out.components);
	for (std::size_t p=0;p<out.width*out.height;++p) {
		out.data[p*out.components + 0] = (f_t)pixels[p].r;
		if (out.components>1) {
			out.data[p*out.components + 1] = (f_t)pixels[p].g;
			out.data[p*out.components + 2] = (f_t)pixels[p].b;
		}
		if (out.components>3) 
			out.data[p*out.components + 3] = (f_t)pixels[p].a;
	}

	return std::move(out);
}

surface<std::uint8_t> load2d(const std::filesystem::path &file_name) {
	png_byte header[8];

	FILE *fp = fopen(file_name.string().data(), "rb");
	if (!fp) {
		throw std::runtime_error("Could not open file");
	}

	// read the header
	if (!fread(header, 1, 8, fp)) {
		std::cerr << file_name << " is not a PNG" << std::endl;
		fclose(fp);
		throw std::runtime_error("Not a valid PNG");
	}

	if (png_sig_cmp(header, 0, 8)) {
		std::cerr << file_name << " is not a PNG" << std::endl;
		fclose(fp);
		throw std::runtime_error("Not a valid PNG");
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		std::cerr << file_name << " png_create_read_struct returned 0" << std::endl;
		fclose(fp);
		throw std::runtime_error("Not a valid PNG");
	}

	// create png info struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		std::cerr << file_name << " png_create_info_struct returned 0" << std::endl;
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		fclose(fp);
		throw std::runtime_error("Not a valid PNG");
	}

	// create png info struct
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		std::cerr << file_name << " png_create_info_struct returned 0" << std::endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		fclose(fp);
		throw std::runtime_error("Not a valid PNG");
	}

	// the code in this if statement gets called if libpng encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
		std::cerr << file_name << " error from libpng" << std::endl;
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		throw std::runtime_error("libpng error");
	}

	// init png reading
	png_init_io(png_ptr, fp);

	// let libpng know you already read the first 8 bytes
	png_set_sig_bytes(png_ptr, 8);

	// read all the info up to the image data
	png_read_info(png_ptr, info_ptr);

	// variables to pass to get info
	int bit_depth, color_type;
	png_uint_32 temp_width, temp_height;

	// get info about png
	png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type,
				 nullptr, nullptr, nullptr);

	if (bit_depth != 8 && (bit_depth != 1 || color_type != PNG_COLOR_TYPE_GRAY)) {
		std::cerr << file_name << " Unsupported bit depth " << std::to_string(bit_depth) << ".  Must be 8" << std::endl;
		throw std::runtime_error("Unsupported bit depth");
	}

	int components;
	switch (color_type) {
	case PNG_COLOR_TYPE_GRAY:
		components = 1;
		break;
	case PNG_COLOR_TYPE_RGB:
		components = 3;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		components = 4;
		break;
	default:
		std::cerr << file_name << " Unknown libpng color type " << std::to_string(color_type) << std::endl;
		fclose(fp);
		throw std::runtime_error("Unsupported PNG color type");
	}

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);

	// Row size in bytes.
	auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	// Allocate the image_data as a big block
	const size_t w = rowbytes / components;
	const auto level0_size = rowbytes * temp_height;
	std::unique_ptr<uint8_t[]> image_data = std::make_unique<uint8_t[]>(level0_size);

	// row_pointers is for pointing to image_data for reading the png with libpng
	png_byte ** row_pointers = reinterpret_cast<png_byte **>(malloc(temp_height * sizeof(png_byte *)));
	if (bit_depth == 8) {
		// set the individual row_pointers to point at the correct offsets of image_data
		for (unsigned int i = 0; i < temp_height; i++)
			row_pointers[i] = image_data.get() + i * w * components;

		// read the png into image_data through row_pointers
		png_read_image(png_ptr, row_pointers);
	}
	else {
		std::cerr << file_name << " Unsupported bit depth" << std::endl;
		fclose(fp);
		throw std::runtime_error("Unsupported PNG bit depth");
	}

	fclose(fp);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	free(row_pointers);

	return surface<std::uint8_t>{ std::move(image_data), w, temp_height, static_cast<std::size_t>(components) };
}

