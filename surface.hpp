
#pragma once

#include <memory>
#include <cstddef>
#include <cassert>
#include <algorithm>

#include "common.hpp"

template <typename T = f_t>
struct surface {
	std::unique_ptr<T[]> data;
	std::size_t width{}, height{}, components{};
	
	auto& operator+=(const surface &s) {
		if (s.width!=width || s.height!=height || s.components!=components) {
			assert(false);
			return *this;
		}
		for (std::size_t idx=0;idx<width*height*components;++idx)
			data[idx] += s.data[idx];
		return *this;
	}
	void fill(const T& val) {
		std::fill(data.get(),data.get()+width+height,val);
	}
};

template <typename T>
auto create_surface(std::size_t w, std::size_t h, std::size_t c=1) {
	return surface<T>{ std::make_unique<T[]>(w*h*c), w,h,c };
}
