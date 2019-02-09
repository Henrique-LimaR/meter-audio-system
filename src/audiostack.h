/**
 *  OSM
 *  Copyright (C) 2018  Pavel Smokotnin

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef AUDIOSTACK_H
#define AUDIOSTACK_H
#include <QDebug>
#include <math.h>
#include <qglobal.h>
#include <mutex>

class AudioStack
{

public:
    struct Cell {
        float value;
        Cell * next = nullptr;
        Cell * pre = nullptr;
    };

private:
    std::mutex memoryMutex;

protected:
    unsigned long _size = 0, sizeLimit = 0;
    Cell * firstdata = nullptr, * lastdata = nullptr;
    Cell * pointer = nullptr;
    virtual void dropFirst();

public:
    AudioStack(unsigned long size);
    AudioStack(AudioStack *original);
    virtual ~AudioStack() = default;

    void setSize(unsigned long size);
    unsigned long size();
    virtual void add(const float data);

    void reset();
    bool next();
    bool isNext();
    float first();

    float current();
    float shift();
    void fill(float value);

    void rewind(long delta);
};

#endif // AUDIOSTACK_H
