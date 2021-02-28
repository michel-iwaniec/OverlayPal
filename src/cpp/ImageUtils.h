//
// This file is part of OverlayPal ( https://github.com/michel-iwaniec/OverlayPal )
// Copyright (c) 2021 Michel Iwaniec.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#pragma once
#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include "Array2D.h"

//
// Created a shifted image from an original image
//
Image2D shiftImage(const Image2D&, int shiftX, int shiftY);

//
// Find optimal shift for an image
//
Image2D shiftImageOptimal(const Image2D& image2D, uint8_t backgroundColor, int cellWidth, int cellHeight, int minX, int maxX, int minY, int maxY, int& shiftX, int& shiftY);

#endif // IMAGE_UTILS_H
