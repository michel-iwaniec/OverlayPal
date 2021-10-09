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

#include <unordered_map>
#include <map>
#include <tuple>
#include <limits>

#include "Array2D.h"
#include "GridLayer.h"

//---------------------------------------------------------------------------------------------------------------------

struct ContinuousPaletteRange
{
    size_t xLeft;
    size_t xRight;
    uint8_t paletteIndex;
};

//---------------------------------------------------------------------------------------------------------------------

Image2D shiftImage(const Image2D& image, int shiftX, int shiftY)
{
    const int w = image.width();
    const int h = image.height();
    Image2D shiftedImage(w, h);

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            uint8_t c = image((x + w - shiftX) % w, (y + h - shiftY) % h);
            shiftedImage(x, y) = c;
        }
    }
    return shiftedImage;
}

//---------------------------------------------------------------------------------------------------------------------

Image2D shiftImageOptimal(const Image2D& image, uint8_t backgroundColor, int cellWidth, int cellHeight, int minX, int maxX, int minY, int maxY, int& shiftX, int& shiftY)
{
    const int w = image.width();
    const int h = image.height();
    std::map<std::pair<int, int>, int> shiftCosts;
    auto bestCostXY = std::make_pair(minX, minY);
    shiftCosts[bestCostXY] = std::numeric_limits<int>::max();
    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            Image2D shiftedImage = shiftImage(image, x, y);
            GridLayer layer(backgroundColor, cellWidth, cellHeight, shiftedImage);
            // Just use sum of colors per cell as cost for now
            int cost = layer.colorsPerCellSum();
            auto costXY = std::make_pair(x, y);
            shiftCosts[costXY] = cost;
            if(cost < shiftCosts[bestCostXY])
            {
                bestCostXY = costXY;
            }
        }
    }
    shiftX = bestCostXY.first;
    shiftY = bestCostXY.second;
    Image2D shiftedImage = shiftImage(image, shiftX, shiftY);
    return shiftedImage;
}

//---------------------------------------------------------------------------------------------------------------------

bool isSubSet(const std::set<uint8_t>& s1, const std::set<uint8_t>& s2)
{
    for(uint8_t c : s1)
    {
        if(s2.count(c) == 0)
        {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<ContinuousPaletteRange> getBestContinuousRanges(const GridLayer& layer,
                                                            size_t y,
                                                            uint8_t paletteIndicesOffset,
                                                            const std::vector<std::set<uint8_t>>& palettes,
                                                            uint8_t backgroundColor)
{
    std::vector<ContinuousPaletteRange> ranges;
    std::map<size_t, std::set<size_t>> validPaletteIndices;
    for(size_t x = 0; x < layer.width(); x++)
    {
        for(size_t i = paletteIndicesOffset; i < palettes.size(); i++)
        {
            const std::set<uint8_t>& cellColors = layer(x, y).colors;
            if(cellColors.size() > 0 &&
               isSubSet(cellColors, palettes[i]))
            {
                validPaletteIndices[x].insert(i);
            }
        }
    }
    for(size_t x = 0; x < layer.width(); x++)
    {
        for(size_t i : validPaletteIndices[x])
        {
            ContinuousPaletteRange r;
            r.xLeft = x;
            r.xRight = x;
            r.paletteIndex = i;
            while(r.xLeft > 0 && validPaletteIndices[r.xLeft - 1].count(i) > 0)
                r.xLeft--;
            while((r.xRight < layer.width() - 1) && validPaletteIndices[r.xRight + 1].count(i) > 0)
                r.xRight++;
            if(r.xRight - r.xLeft > 0)
            {
                ranges.push_back(r);
            }
        }
    }
    std::sort(ranges.begin(),
              ranges.end(),
              [](const ContinuousPaletteRange& a, const ContinuousPaletteRange& b)
              {
                  size_t aLength = a.xRight - a.xLeft + 1;
                  size_t bLength = b.xRight - b.xLeft + 1;
                  return aLength > bLength;
              });
    return ranges;
}

//---------------------------------------------------------------------------------------------------------------------

void optimizeContinuity(const GridLayer& layer,
                        Array2D<uint8_t>& paletteIndices,
                        uint8_t paletteIndicesOffset,
                        const std::vector<std::set<uint8_t>>& palettes,
                        uint8_t backgroundColor)
{
    int cellWidth = layer.cellWidth();
    int cellHeight = layer.cellHeight();
    for(size_t y = 0; y < layer.height(); y++)
    {
        // Get alternative palette indices for each cell in row
        std::vector<bool> finalized;
        finalized.resize(layer.width(), false);
        //
        std::vector<ContinuousPaletteRange> ranges = getBestContinuousRanges(layer,
                                                                             y,
                                                                             paletteIndicesOffset,
                                                                             palettes,
                                                                             backgroundColor);
        for(auto& range : ranges)
        {
            // Set all to the best palette
            for(size_t x = range.xLeft; x <= range.xRight; x++)
            {
                if(!finalized[x])
                {
                    // Remap cell pixels
                    paletteIndices(x, y) = range.paletteIndex;
                    finalized[x] = true;
                }
            }
        }
    }
}
