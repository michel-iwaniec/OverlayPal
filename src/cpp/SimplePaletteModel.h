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
#ifndef SIMPLE_PALETTE_MODEL_H
#define SIMPLE_PALETTE_MODEL_H

#include <QObject>
#include <QString>
#include <QAbstractTableModel>

#include <cstdint>
#include <cassert>
#include <vector>
#include <set>

//
// Model used for providing overlay optimisation's palettes
//
class SimplePaletteModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    SimplePaletteModel();

    ~SimplePaletteModel() override;

    void setPalette(const std::vector<std::set<uint8_t>>& palettes, uint8_t backgroundColor);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<std::vector<uint8_t>> mPalettes;
    const int NumPalettes = 8;
    const int PaletteGroupSize = 4;
};

#endif // SIMPLE_PALETTE_MODEL_H
