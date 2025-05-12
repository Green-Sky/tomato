#pragma once

// qoirdo.hpp
// Copyright (C) 2022 Richard Geldreich, Jr. All Rights Reserved.
// Copyright (C) 2025 Erik Scholz
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdint>
#include <vector>

bool init_qoi_rdo(void);
bool quit_qoi_rdo(void);

struct qoi_rdo_desc {
	unsigned int width;
	unsigned int height;
	unsigned char channels;
	unsigned char colorspace;
};

// quality 1-100
std::vector<uint8_t> encode_qoi_rdo_simple(const uint8_t* data, const qoi_rdo_desc& desc, int quality);

// TODO: finetuneable
//uint8_t* encode_qoi_rdo_advanced(const uint8_t* data, const qoi_rdo_desc* desc, int* out_len);
