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

#include "GridLayer.h"

//---------------------------------------------------------------------------------------------------------------------

GridLayer::GridLayer():
    Array2D()
{
}

//---------------------------------------------------------------------------------------------------------------------

GridLayer::GridLayer(int width, int height):
    Array2D(width, height)
{
}

//---------------------------------------------------------------------------------------------------------------------

GridLayer::GridLayer(uint8_t backgroundColor, size_t cellWidth, size_t cellHeight, const Image2D &image):
    Array2D<GridCell>((image.width() + cellWidth - 1) / cellWidth, (image.height() + cellHeight - 1) / cellHeight),
    mBackgroundColor(backgroundColor),
    mCellWidth(cellWidth),
    mCellHeight(cellHeight),
    mMaxColorsPerCell(0),
    mColorsPerCellSum(0)
{
    initializeFromImage(image);
}

//---------------------------------------------------------------------------------------------------------------------

GridLayer::GridLayer(uint8_t backgroundColor, size_t cellWidth, size_t cellHeight, size_t width, size_t height):
    Array2D<GridCell>(width, height),
    mBackgroundColor(backgroundColor),
    mCellWidth(cellWidth),
    mCellHeight(cellHeight),
    mMaxColorsPerCell(0),
    mColorsPerCellSum(0)
{
}

//---------------------------------------------------------------------------------------------------------------------

size_t GridLayer::cellWidth() const
{
    return mCellWidth;
}

//---------------------------------------------------------------------------------------------------------------------

size_t GridLayer::cellHeight() const
{
    return mCellHeight;
}

//---------------------------------------------------------------------------------------------------------------------

size_t GridLayer::maxColorsPerCell() const
{
    return mMaxColorsPerCell;
}

//---------------------------------------------------------------------------------------------------------------------

size_t GridLayer::colorsPerCellSum() const
{
    return mColorsPerCellSum;
}

//---------------------------------------------------------------------------------------------------------------------

const std::set<uint8_t> &GridLayer::colors() const
{
    return mColors;
}

//---------------------------------------------------------------------------------------------------------------------

void GridLayer::initializeFromImage(const Image2D &image)
{
    const int imageWidth = cellWidth() * width();
    const int imageHeight = cellHeight() * height();
    mMaxColorsPerCell = 0;
    mColorsPerCellSum = 0;
    mColors.clear();
    for(int Y = 0; Y < height(); Y++)
    {
        for(int X = 0; X < width(); X++)
        {
            GridCell cell;
            for(int x = 0; x < cellWidth(); x++)
            {
                std::set<uint8_t> columnColors;
                for(int y = 0; y < cellHeight(); y++)
                {
                    int srcX = X * cellWidth() + x;
                    int srcY = Y * cellHeight() + y;
                    if(srcX >= 0 && srcX < imageWidth && srcY >= 0 && srcY < imageHeight)
                    {
                        uint8_t c = image(srcX, srcY);
                        if(c != mBackgroundColor)
                        {
                            cell.colors.insert(c);
                            cell.pixelCount[c]++;
                            columnColors.insert(c);
                            mColors.insert(c);
                        }
                    }
                }
                for(uint8_t c : columnColors)
                {
                    cell.columnCount[c]++;
                }
            }
            (*this)(X, Y) = cell;
            // Update caches
            std::set<uint8_t> colors = cell.colors;
            size_t numColorsInCell = colors.size();
            mMaxColorsPerCell = std::max(mMaxColorsPerCell, numColorsInCell);
            mColorsPerCellSum += numColorsInCell;
        }
    }
}
