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
#ifndef ARRAY_2D_H
#define ARRAY_2D_H

#include <cstdint>
#include <cassert>
#include <cstring>

//
// Simple class for representing a 2d array
//
template<typename T>
class Array2D
{
public:
    Array2D():
        mWidth(0),
        mHeight(0),
        mData(nullptr)
    {
    }

    Array2D(int width, int height):
        mWidth(width),
        mHeight(height),
        mData(nullptr)
    {
        if(mWidth > 0 && mHeight > 0)
        {
            mData = new T[mWidth * mHeight];
            for(size_t y = 0; y < mHeight; y++)
            {
                for(size_t x = 0; x < mWidth; x++)
                {
                    (*this)(x, y) = T();
                }
            }
        }
    }

    Array2D(const Array2D& other):
        mWidth(other.width()),
        mHeight(other.height()),
        mData(nullptr)
    {
        if(mWidth > 0 && mHeight > 0)
        {
            mData = new T[mWidth * mHeight];
        }
        initialiseFromOther(other);
    }

    Array2D& operator=(const Array2D& other)
    {
        if(mData != nullptr)
        {
            delete[] mData;
            mData = nullptr;
            mWidth = 0;
            mHeight = 0;
        }
        mWidth = other.width();
        mHeight = other.height();
        if(mWidth > 0 && mHeight > 0)
        {
            mData = new T[mWidth * mHeight];
        }
        initialiseFromOther(other);
        return *this;
    }

    virtual ~Array2D()
    {
        if(mData != nullptr)
        {
            delete[] mData;
            mData = nullptr;
            mWidth = 0;
            mHeight = 0;
        }
    }

    size_t width() const
    {
        return mWidth;
    }

    size_t height() const
    {
        return mHeight;
    }

    T& operator[](int index)
    {
        assert(index >= 0 && index < mWidth * mHeight);
        return mData[index];
    }

    const T& operator[](size_t index) const
    {
        assert(index >= 0 && index < mWidth * mHeight);
        return mData[index];
    }

    T& operator()(int x, int y)
    {
        assert(x >= 0 && x < mWidth);
        assert(y >= 0 && y < mHeight);
        return mData[mWidth * y + x];
    }

    const T& operator()(int x, int y) const
    {
        assert(x >= 0 && x < mWidth);
        assert(y >= 0 && y < mHeight);
        return mData[mWidth * y + x];
    }

    bool empty(uint8_t emptyColor = 0) const
    {
        if(mWidth > 0 && mHeight > 0)
        {
            for(size_t y = 0; y < mHeight; y++)
            {
                for(size_t x = 0; x < mWidth; x++)
                {
                    if((*this)(x, y) != emptyColor)
                        return false;
                }
            }
            return true;
        }
        else
        {
            return true;
        }
    }

protected:

    void initialiseFromOther(const Array2D& other)
    {
        assert(mWidth == other.width());
        assert(mHeight == other.height());
        assert(other.mData != nullptr);
        for(int y = 0; y < mHeight; y++)
        {
            for(int x = 0; x < mWidth; x++)
            {
                mData[mWidth * y + x] = other.mData[mWidth * y + x];
            }
        }
    }

private:
    size_t mWidth;
    size_t mHeight;
    T* mData;
};

// Specialization for indexed color images
using Image2D = Array2D<uint8_t>;

#endif // ARRAY_2D_H
