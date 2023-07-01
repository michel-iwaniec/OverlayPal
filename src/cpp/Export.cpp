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

#include <functional>
#include <unordered_map>

#include "Export.h"

struct TileNES_8x8 {
    uint64_t p0;    // plane0
    uint64_t p1;    // plane1
};

struct TileNES_8x8_hash
{
    std::size_t operator() (const TileNES_8x8& t) const
    {
        return std::hash<uint64_t>()(t.p0) ^ std::hash<uint64_t>()(t.p1);
    }
};

struct TileNES_8x8_equal
{
    bool operator() (const TileNES_8x8& a, const TileNES_8x8& b) const
    {
        return a.p0 == b.p0 && a.p1 == b.p1;
    }
};

struct TileNES_8x16 {
    uint64_t tUp0;  // upper tile plane0
    uint64_t tUp1;  // upper tile plane1
    uint64_t tLp0;  // lower tile plane0
    uint64_t tLp1;  // lower tile plane1
};

struct TileNES_8x16_hash
{
    std::size_t operator() (const TileNES_8x16& t) const
    {
        return std::hash<uint64_t>()(t.tUp0) ^
               std::hash<uint64_t>()(t.tUp1) ^
               std::hash<uint64_t>()(t.tLp0) ^
               std::hash<uint64_t>()(t.tLp1);
    }
};

struct TileNES_8x16_equal
{
    bool operator() (const TileNES_8x16& a, const TileNES_8x16& b) const
    {
        return a.tUp0 == b.tUp0 && a.tUp1 == b.tUp1 &&
               a.tLp0 == b.tLp0 && a.tLp1 == b.tLp1;
    }
};

//---------------------------------------------------------------------------------------------------------------------

TileNES_8x8 extractTileNES_8x8(const Image2D& image, int paletteMask, int x, int y, int w, int h, uint8_t p)
{
    TileNES_8x8 t;
    t.p0 = 0;
    t.p1 = 0;
    uint8_t* p0 = reinterpret_cast<uint8_t*>(&t.p0);
    uint8_t* p1 = reinterpret_cast<uint8_t*>(&t.p1);
    for(int i = 0; i < h; i++)
    {
        for(int j = 0; j < w; j++)
        {
            uint8_t c = image(x + j, y + i);
            int palIndex = c >> 2;
            bool enabled = (1 << palIndex) & paletteMask;
            if(enabled && palIndex == p)
            {
                uint8_t b0 = (c >> 0) & 0x1;
                uint8_t b1 = (c >> 1) & 0x1;
                p0[i] |= b0 << (w - 1 - j);
                p1[i] |= b1 << (w - 1 - j);
            }
        }
    }
    return t;
}

using TileMap = std::unordered_map<TileNES_8x8, size_t, TileNES_8x8_hash, TileNES_8x8_equal>;

static void OutputTileRow(int y,
                          const Image2D& image,
                          int paletteMask,
                          TileMap& tileDataToIndex,
                          const Array2D<uint8_t>& paletteIndicesBackground,
                          std::vector<uint8_t>& nametable,
                          std::vector<uint8_t>& exRAM,
                          std::vector<uint8_t>& chr)
{

    int tileWidth = 8;
    int tileHeight = 8;
    int nametableGridHeight = 30;
    int nametableGridWidth = 32;
    int scaleWidth = nametableGridWidth / paletteIndicesBackground.width();
    int scaleHeight = nametableGridHeight / paletteIndicesBackground.height();
    for(size_t x = 0; x < nametableGridWidth; x++)
    {
        uint8_t p = paletteIndicesBackground(x / scaleWidth, y / scaleHeight);
        TileNES_8x8 t = extractTileNES_8x8(image, paletteMask, tileWidth * x, tileHeight * y, tileWidth, tileHeight, p);
        if(!tileDataToIndex.count(t))
        {
            tileDataToIndex[t] = tileDataToIndex.size();
            uint8_t* p = reinterpret_cast<uint8_t*>(&t.p0);
            for(int y = 0; y < 2 * tileHeight; y++)
            {
                chr.push_back(p[y]);
            }
        }
        int tileIndex = tileDataToIndex[t];
        nametable[nametableGridWidth * y + x] = tileIndex & 0xFF;
        exRAM[nametableGridWidth * y + x] = (p << 6) | (tileIndex >> 8);
    }
}

static void OutputAttributes(std::vector<uint8_t>& nametable,
                             std::vector<uint8_t>& exRAM)
{

    int nametableGridWidth = 32;
    int nametableGridHeight = 30;
    int attributeGridWidth = 16;
    int attributeGridHeight = 15;
    // Write non-MMC5 16x16 attributes
    Array2D<uint8_t> colorTable(attributeGridWidth, attributeGridHeight + 1);
    for(int y = 0; y < attributeGridHeight; y++)
    {
        for(int x = 0; x < attributeGridWidth; x++)
        {
            colorTable(x, y) = (exRAM[nametableGridWidth * (y << 1) + (x << 1)] >> 6) & 0x3;
        }
    }
    // Encode into 64-byte attribute table
    for(int y = 0; y < 8; y++)
    {
        for(int x = 0; x < 8; x++)
        {
            uint8_t a00 = colorTable(2 * x + 0, 2 * y + 0);
            uint8_t a10 = colorTable(2 * x + 1, 2 * y + 0);
            uint8_t a01 = colorTable(2 * x + 0, 2 * y + 1);
            uint8_t a11 = colorTable(2 * x + 1, 2 * y + 1);
            uint8_t a = (a11 << 6) | (a01 << 4) | (a10 << 2) | a00;
            int attributesOffset = nametableGridWidth * nametableGridHeight;
            nametable[attributesOffset + 8 * y + x] = a;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void buildDataNES_BG(const Image2D& image,
                     int paletteMask,
                     const Array2D<uint8_t>& paletteIndicesBackground,
                     std::vector<uint8_t>& nametable,
                     std::vector<uint8_t>& exRAM,
                     std::vector<uint8_t>& chr)
{
    nametable.clear();
    nametable.resize(1024);
    exRAM.clear();
    exRAM.resize(1024);
    chr.clear();
    TileMap tileDataToIndex;
    constexpr int nametableGridHeight = 30;
    for(size_t y = 0; y < nametableGridHeight; y++)
    {
        OutputTileRow(y, image, paletteMask, tileDataToIndex, paletteIndicesBackground, nametable, exRAM, chr);
    }
    OutputAttributes(nametable, exRAM);
}

//---------------------------------------------------------------------------------------------------------------------

void buildDataNES_BGBanked(const Image2D& image,
                     int paletteMask,
                     int exportBankSize,
                     const Array2D<uint8_t>& paletteIndicesBackground,
                     std::vector<uint8_t>& nametable,
                     std::vector<uint8_t>& exRAM,
                     std::vector<std::vector<uint8_t>>& chr)
{
    exRAM.clear();
    exRAM.resize(1024);
    nametable.clear();
    nametable.resize(1024);
    chr.clear();
    chr.push_back({});
    using TileMap = std::unordered_map<TileNES_8x8, size_t, TileNES_8x8_hash, TileNES_8x8_equal>;
    TileMap tileDataToIndex = {};
    int currentBank = 0;
    int tileWidth = 8;
    int tileHeight = 8;
    int nametableGridWidth = 32;
    int nametableGridHeight = 30;
    int scaleWidth = nametableGridWidth / paletteIndicesBackground.width();
    int scaleHeight = nametableGridHeight / paletteIndicesBackground.height();
    for(size_t y = 0; y < nametableGridHeight; y++)
    {
        // make a copy of the hash map
        auto bankMap = tileDataToIndex;
        bool nextBank = false;
        // loop through this row of tiles to see if it can fit in the current bank
        for(size_t x = 0; x < nametableGridWidth; x++)
        {
            uint8_t p = paletteIndicesBackground(x / scaleWidth, y / scaleHeight);
            TileNES_8x8 t = extractTileNES_8x8(image, paletteMask, tileWidth * x, tileHeight * y, tileWidth, tileHeight, p);
            if(!bankMap.count(t))
            {
                bankMap[t] = bankMap.size();
            }
            if (bankMap.size() > (exportBankSize / 16)) {
                nextBank = true;
                break;
            }
        }

        // if it overflows this bank, bump to the next bank and start fresh
        if (nextBank) {
            tileDataToIndex.clear();
            currentBank++;
            chr.push_back({});
        }

        OutputTileRow(y, image, paletteMask, tileDataToIndex, paletteIndicesBackground, nametable, exRAM, chr[currentBank]);
    }
    OutputAttributes(nametable, exRAM);
}

//---------------------------------------------------------------------------------------------------------------------

void buildDataNES_OAM_8x8(const Image2D& image,
                          int paletteMask,
                          const std::vector<Sprite>& sprites,
                          std::vector<uint8_t>& oam,
                          std::vector<uint8_t>& oamCHR)
{
    oam.clear();
    oamCHR.clear();
    std::unordered_map<TileNES_8x8, size_t, TileNES_8x8_hash, TileNES_8x8_equal> tileDataToIndex;
    for(const Sprite& s : sprites )
    {
        TileNES_8x8 t = extractTileNES_8x8(image, paletteMask, s.x, s.y, 8, 8, s.p);
        if(!tileDataToIndex.count(t))
        {
            tileDataToIndex[t] = tileDataToIndex.size();
            uint8_t* p = reinterpret_cast<uint8_t*>(&t.p0);
            for(int y = 0; y < 2 * 8; y++)
            {
                oamCHR.push_back(p[y]);
            }
        }
        oam.push_back(static_cast<uint8_t>(s.y - 1));
        oam.push_back(static_cast<uint8_t>(tileDataToIndex[t]));
        oam.push_back(static_cast<uint8_t>(s.p));
        oam.push_back(static_cast<uint8_t>(s.x));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void buildDataNES_OAM_8x16(const Image2D& image,
                           int paletteMask,
                           const std::vector<Sprite> sprites,
                           std::vector<uint8_t>& oam,
                           std::vector<uint8_t>& oamCHR)
{
    oam.clear();
    oamCHR.clear();
    std::unordered_map<TileNES_8x16, size_t, TileNES_8x16_hash, TileNES_8x16_equal> tileDataToIndex;
    for(const Sprite& s : sprites )
    {
        TileNES_8x8 tU = extractTileNES_8x8(image, paletteMask, s.x, s.y, 8, 8, s.p);
        TileNES_8x8 tL = extractTileNES_8x8(image, paletteMask, s.x, s.y + 8, 8, 8, s.p);
        TileNES_8x16 t;
        t.tUp0 = tU.p0;
        t.tUp1 = tU.p1;
        t.tLp0 = tL.p0;
        t.tLp1 = tL.p1;
        if(!tileDataToIndex.count(t))
        {
            tileDataToIndex[t] = tileDataToIndex.size();
            uint8_t* pU = reinterpret_cast<uint8_t*>(&t.tUp0);
            for(int y = 0; y < 2 * 8; y++)
            {
                oamCHR.push_back(pU[y]);
            }
            uint8_t* pL = reinterpret_cast<uint8_t*>(&t.tLp0);
            for(int y = 0; y < 2 * 8; y++)
            {
                oamCHR.push_back(pL[y]);
            }
        }
        oam.push_back(static_cast<uint8_t>(s.y - 1));
        oam.push_back(static_cast<uint8_t>(tileDataToIndex[t] << 1));
        oam.push_back(static_cast<uint8_t>(s.p));
        oam.push_back(static_cast<uint8_t>(s.x));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void buildDataNES_palette(const std::vector<std::set<uint8_t> > &palettes, uint8_t backgroundColor, std::vector<uint8_t>& paletteOut)
{
    const size_t PaletteGroupSize = 4;
    paletteOut.clear();
    for(size_t i = 0; i < palettes.size(); i++)
    {
        paletteOut.push_back(backgroundColor);
        size_t j = 1;
        for(uint8_t c : palettes[i])
        {
            paletteOut.push_back(c);
            j++;
        }
        while(j < PaletteGroupSize)
        {
            paletteOut.push_back(0x3F);
            j++;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

ExportDataNES buildExportData(const OverlayOptimiser& optimiser, int paletteMask, int exportBankSize)
{
    ExportDataNES exportData;
    Image2D image = optimiser.outputImage();
    // Background nametable / CHR
    // if there is no bank size, then we can
    if (exportBankSize == 0) {
        exportData.bgCHR.clear();
        exportData.bgCHR.resize(1);
        exportData.bgCHR[0] = {};
        buildDataNES_BG(image,
                        paletteMask,
                        optimiser.debugPaletteIndicesBackground(),
                        exportData.nametable,
                        exportData.exram,
                        exportData.bgCHR[0]);
    } else {
        buildDataNES_BGBanked(image,
                        paletteMask,
                        exportBankSize,
                        optimiser.debugPaletteIndicesBackground(),
                        exportData.nametable,
                        exportData.exram,
                        exportData.bgCHR);
    }
    // Sprite OAM / CHR
    if(optimiser.spriteHeight() == 16)
        buildDataNES_OAM_8x16(image, paletteMask, optimiser.spritesOverlay(), exportData.oam, exportData.oamCHR);
    else
        buildDataNES_OAM_8x8(image, paletteMask, optimiser.spritesOverlay(), exportData.oam, exportData.oamCHR);
    // Palette
    buildDataNES_palette(optimiser.palettes(), optimiser.backgroundColor(), exportData.palette);
    return exportData;
}
