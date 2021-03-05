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
#ifndef GRID_LAYER_H
#define GRID_LAYER_H

#include <set>
#include <unordered_map>
#include <algorithm>
#include <cassert>

#include "Array2D.h"

using Colors = std::set<uint8_t>;

//
// Represent a grid cell for color mapping purposes.
//
struct GridCell
{
    Colors colors;
    std::unordered_map<uint8_t, int> pixelCount;
    std::unordered_map<uint8_t, int> columnCount;
    size_t numColors() const
    {
        return colors.size();
    }
};

//
// Class to represent the division of an indexed image into discrete grid cells
//
class GridLayer : public Array2D<GridCell>
{
public:
    GridLayer();

    GridLayer(int width, int height);

    GridLayer(uint8_t backgroundColor, size_t cellWidth, size_t cellHeight, const Image2D& image);

    GridLayer(uint8_t backgroundColor, size_t cellWidth, size_t cellHeight, size_t width, size_t height);

    size_t cellWidth() const;

    size_t cellHeight() const;

    size_t maxColorsPerCell() const;

    size_t colorsPerCellSum() const;

    const std::set<uint8_t>& colors() const;

protected:
    void initializeFromImage(const Image2D& image);

private:
    uint8_t mBackgroundColor;
    size_t mCellWidth;
    size_t mCellHeight;
    size_t mMaxColorsPerCell;
    size_t mColorsPerCellSum;
    std::set<uint8_t> mColors;
};



#endif // GRID_LAYER_H
