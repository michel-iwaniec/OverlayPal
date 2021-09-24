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

#include <QColor>

#include "SimplePaletteModel.h"

//---------------------------------------------------------------------------------------------------------------------

SimplePaletteModel::SimplePaletteModel()
{
    for(size_t i = 0; i < NumPalettes; i++)
    {
        std::vector<uint8_t> paletteGroup;
        paletteGroup.resize(PaletteGroupSize, 0x3F);
        mPalettes.push_back(paletteGroup);
    }
}

//---------------------------------------------------------------------------------------------------------------------

SimplePaletteModel::~SimplePaletteModel()
{
}

//---------------------------------------------------------------------------------------------------------------------

void SimplePaletteModel::setPalette(const std::vector<std::set<uint8_t> > &palettes, uint8_t backgroundColor)
{
    std::vector<std::vector<uint8_t>> newPalettes;
    for(size_t i = 0; i < palettes.size(); i++)
    {
        std::vector<uint8_t> paletteGroup;
        paletteGroup.push_back(backgroundColor);
        size_t j = 0;
        for(uint8_t c : palettes[i])
        {
            paletteGroup.push_back(c);
            j++;
        }
        while(j < PaletteGroupSize)
        {
            paletteGroup.push_back(0x3F);
            j++;
        }
        newPalettes.push_back(paletteGroup);
    }
    beginResetModel();
    mPalettes = newPalettes;
    endResetModel();
}

//---------------------------------------------------------------------------------------------------------------------

int SimplePaletteModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
    {
        return mPalettes.size();
    }
    else {
        return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------

int SimplePaletteModel::columnCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
    {
        return PaletteGroupSize;
    }
    else {
        return 0;
    }
}

//---------------------------------------------------------------------------------------------------------------------

QVariant SimplePaletteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
    {
        return QVariant();
    }
    if(orientation == Qt::Horizontal)
    {
        return QString("C%1").arg(section);
    }
    else if(orientation == Qt::Vertical)
    {
        if(section < 4)
        {
            return QString("BG%1").arg(section, 1, 16);
        }
        else
        {
            return QString("SPR%1").arg(section - 4, 1, 16);
        }
    }
    return QVariant();
}

//---------------------------------------------------------------------------------------------------------------------

QVariant SimplePaletteModel::data(const QModelIndex &index, int role) const
{
    int col = index.column();
    int row = index.row();
    // Format as hex
    int color = mPalettes[row][col];
    switch(role)
    {
    case Qt::DisplayRole:
        return QString("%1").arg(color, 2, 16, QLatin1Char('0')).toUpper();
    default:
        return QVariant();
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool SimplePaletteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
    {
        return false;
    }
    int col = index.column();
    int row = index.row();
    if(col >= NumPalettes)
        return false;
    if(col == 0)
    {
        return false;
    }
    else
    {
        if(role == Qt::DisplayRole || role == Qt::EditRole)
        {
            mPalettes[row][col] = value.toInt();
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

Qt::ItemFlags SimplePaletteModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
    {
        return Qt::ItemIsEnabled;
    }
    int col = index.column();
    if(col == 0)
    {
        return Qt::ItemIsEnabled;
    }
    else
    {
        return Qt::ItemIsEnabled;
    }
}

//---------------------------------------------------------------------------------------------------------------------

QHash<int, QByteArray> SimplePaletteModel::roleNames() const
{
    return { {Qt::DisplayRole, "display"} };
}
