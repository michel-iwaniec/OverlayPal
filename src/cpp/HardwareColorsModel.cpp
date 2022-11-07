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

#include "HardwareColorsModel.h"

#include <QColor>
#include <QDebug>

//---------------------------------------------------------------------------------------------------------------------

HardwareColorsModel::HardwareColorsModel()
{
}

//---------------------------------------------------------------------------------------------------------------------

HardwareColorsModel::~HardwareColorsModel()
{
}

//---------------------------------------------------------------------------------------------------------------------

void HardwareColorsModel::setColors(const std::vector<uint8_t>& colors)
{
    beginResetModel();
    mColors = colors;
    endResetModel();
}

//---------------------------------------------------------------------------------------------------------------------

void HardwareColorsModel::setColors(const std::set<uint8_t>& colors)
{
    std::vector<uint8_t> sortedColors;
    std::copy(colors.begin(), colors.end(), std::back_inserter(sortedColors));
    std::sort(sortedColors.begin(), sortedColors.end());
    setColors(sortedColors);
}

//---------------------------------------------------------------------------------------------------------------------

void HardwareColorsModel::setHardwarePalette(const QVariantList& hardwarePalette)
{
    mHardwarePalette = hardwarePalette;
}

//---------------------------------------------------------------------------------------------------------------------

int HardwareColorsModel::rowCount(const QModelIndex &parent) const
{
    return mColors.size();
}

//---------------------------------------------------------------------------------------------------------------------

int HardwareColorsModel::columnCount(const QModelIndex &parent) const
{

    return 1;
}

//---------------------------------------------------------------------------------------------------------------------

QVariant HardwareColorsModel::data(const QModelIndex &index, int role) const
{
    int col = index.column();
    int row = index.row();

    if(row < 0 || row >= mColors.size())
    {
        // return dummy data for out-of-range
        switch(role)
        {
            case Qt::DisplayRole:
                return "3F";
            case Qt::BackgroundRole:
                return QColor(0, 0, 0);
            case Qt::ForegroundRole:
                return QColor(255, 255, 255);
            default:
                return QVariant();
        }
    }

    // Format as hex
    int color = mColors[row];
    QVariantList rgbColor = mHardwarePalette[color].toList();
    QColor backgroundColor = QColor(rgbColor[0].toInt(), rgbColor[1].toInt(), rgbColor[2].toInt());
    switch(role)
    {
        case Qt::DisplayRole:
            return QString("%1").arg(color, 2, 16, QLatin1Char('0')).toUpper();
        case Qt::BackgroundRole:
            return backgroundColor;
        case Qt::ForegroundRole:
            return foregroundTextColor(backgroundColor);
        default:
            return QVariant();
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool HardwareColorsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

Qt::ItemFlags HardwareColorsModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled;
}

//---------------------------------------------------------------------------------------------------------------------

QHash<int, QByteArray> HardwareColorsModel::roleNames() const
{
    return { {Qt::DisplayRole, "display"},
             {Qt::BackgroundRole, "backgroundColor"},
             {Qt::ForegroundRole, "foregroundColor"}};
}
