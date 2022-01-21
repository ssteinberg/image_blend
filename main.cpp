
/*
*
* Image diff tool
* Copyright  Shlomi Steinberg
*
*/

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cassert>
#include <getopt.h>
#include <vector>

#include "load2d.hpp"
#include "write2d.hpp"

#include <glm/glm.hpp>

void print_usage() {
	std::cout << "Usage: image_blend <options> --out=<output> <input1> <input2> ... <inputN>" << std::endl;
	std::cout << "-w <weight1,weight2,...,weightN>\t\tWeights (defaults to all 1s)." << std::endl;
}

int main(int argc, char* argv[]) {
	const char* path = nullptr;
	std::vector<char*> input_names;
	std::vector<surface<>> inputs;
	std::vector<f_t> weights;

	if (argc > 3) {
		option long_options[] = {
			{"out", required_argument, nullptr, 'o'},
			{"weights", required_argument, nullptr, 'w'},
			{nullptr, 0, nullptr, 0}
		};
		char ch;
		while ((ch = getopt_long(argc, argv, "o:w:", long_options, NULL)) != -1) {
			switch (ch) {
			case 'o':
				path = optarg;
				break;
			case 'w': {
				auto *x = optarg;
				while(!!x) {
					const auto w = atof(x);
					if (w<=0) {
						std::cerr << "Invalid weight." << std::endl;
						return 1;
					}
					weights.emplace_back(w);
					x = strstr(x,",");
					if (x) ++x;
				}
			}
				break;
			default: {
				std::cerr << "Unknown option." << std::endl;
				print_usage();
				return 1;
			}
			}
		}
			
		for (auto idx=optind;idx<argc;++idx) {
			auto s = load_2d_hdr(argv[idx]);
			if (!s.data) {
				std::cerr << "Couldn't load \"" << argv[idx] << "\"." << std::endl;
				return 1;
			}
			inputs.emplace_back(std::move(s));
			input_names.emplace_back(argv[idx]);
		}
		
		if (!weights.size())
			while(weights.size()<inputs.size()) weights.emplace_back(1.);
		if (weights.size()!=inputs.size()) {
			std::cerr << "Mismatched weight and input count!" << std::endl;
			return 1;
		}
	}

	if (!path || inputs.size()<2) {
		print_usage();
		return 1;
	}
	
	std::size_t w = inputs.front().width;
	std::size_t h = inputs.front().height;
	std::size_t c = inputs.front().components;
	auto W = weights.front();
	for (std::size_t i=1;i<inputs.size();++i) {
		if (inputs[i].width!=w || inputs[i].height!=h || inputs[i].components!=c) {
			std::cerr << "Mismatched input width/height/components." << std::endl;
			return 1;
		}
		W += weights[i];
	}
	
	std::cout << "Blending (" << std::to_string(w) << "x" << std::to_string(h) << ", " << std::to_string(c) << " components):" << std::endl;
	for (std::size_t i=0;i<inputs.size();++i)
		std::cout << "w:" << std::to_string(weights[i])
				  << "\t -- \t'" << input_names[i] << "'" << std::endl;
	std::cout << "Total weight: " << std::to_string(W) << std::endl;

	surface<> out;
	out.width = w;
	out.height = h;
	out.components = c;
	out.data = std::make_unique<f_t[]>(w*h*c);
	std::fill(out.data.get(),out.data.get()+w*h*c,(f_t)0);

	for (std::size_t i=0;i<inputs.size();++i) {
		const auto in = inputs[i].data.get();
		const auto we = weights[i];
		for (std::size_t p=0;p<w*h*c;++p) 
			out.data[p] += we/W * in[p];
	}

	write_2d_hdr_convert(path, out);

	return 0;
}
