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
