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

#include <cstdint>
#include <set>

#include "Array2D.h"
#include "GridLayer.h"

//
// Created a shifted image from an original image
//
Image2D shiftImage(const Image2D&, int shiftX, int shiftY);

//
// Find optimal shift for an image
//
Image2D shiftImageOptimal(const Image2D& image2D, uint8_t backgroundColor, int cellWidth, int cellHeight, int minX, int maxX, int minY, int maxY, int& shiftX, int& shiftY);

//
// Optimize palette index continuity by switching palette indices so that they are horizontally continuous where possible
//
void optimizeContinuity(const GridLayer& layer, Array2D<uint8_t>& paletteIndices, uint8_t paletteIndicesOffset, const std::vector<std::set<uint8_t>>& palettes, uint8_t backgroundColor);

//
// Move overlay colors back to background where possible, changing the palette index
// of the background cell if needed.
//
// Reasons for this function is when CBC stops on a timeout. The best-solution-so-far
// found can sometimes have obvious suboptimal results that are easy to undo here.
//
void optimizeUnnecessaryOverlayColors(GridLayer& layerBackground,
                                      GridLayer& layerOverlay,
                                      Array2D<uint8_t>& paletteIndices,
                                      uint8_t paletteIndicesOffset,
                                      const std::vector<Colors>& palettes);

//
// Optimize unnecessary palettes by identifying and merging two palettes that could fit into one
//
void optimizeUnnecessaryPalettes(Array2D<uint8_t>& paletteIndices,
                                 uint8_t paletteIndicesOffset,
                                 std::vector<Colors>& palettes,
                                 size_t maxColors);

#endif // IMAGE_UTILS_H
