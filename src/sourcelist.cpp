/**
 *  OSM
 *  Copyright (C) 2019  Pavel Smokotnin

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
#include "sourcelist.h"
#include "measurement.h"
#include "union.h"
#include "elc.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>

SourceList::SourceList(QObject *parent, bool appendMeasurement) :
    QObject(parent),
    m_currentFile(),
    colorIndex(3)
{
    if (appendMeasurement) {
        addMeasurement();
    }
}
SourceList* SourceList::clone(QObject *parent, bool filtered) const noexcept
{qDebug() << "clone";
    SourceList *list = new SourceList(parent, false);
    for (auto item : items()) {
        if (!filtered || item->objectName() == "Measurement" || item->objectName() == "Stored") {
            list->appendItem(item);
        }
    }

    connect(this, &SourceList::preItemRemoved, list, [=](int index) {
        if (!list) return;
        auto item = get(index);
        list->removeItem(item, false);
    });
    connect(this, &SourceList::postItemAppended, list, [=](auto item){
        list->appendItem(item, false);
    });
    connect(this, &SourceList::preItemMoved, list, [=](int from, int to){
        list->move(from, to);
    });

    return list;
}
const QVector<Fftchart::Source*>& SourceList::items() const
{
    return mItems;
}

SourceList::iterator SourceList::begin() noexcept
{
    return mItems.begin();
}
SourceList::iterator SourceList::end() noexcept
{
    return mItems.end();
}
int SourceList::count() const noexcept
{
    return mItems.size();
}
Fftchart::Source * SourceList::get(int i) const noexcept
{
    if (i < 0 || i >= mItems.size())
        return nullptr;

    return mItems.at(i);
}

void SourceList::clean() noexcept
{
    while (mItems.size() > 0) {
        emit preItemRemoved(0);
        auto item = get(0);
        mItems.removeAt(0);
        emit postItemRemoved();
        item->deleteLater();
    }
    colorIndex = 3;
}
void SourceList::reset() noexcept
{
    clean();
    addMeasurement();
}
bool SourceList::move(int from, int to) noexcept
{
    if (from == to)
        return false;

    if (from < to) {
        return move(to, from);
    }

    emit preItemMoved(from, to);
    if (mItems.size() > from) {
        Fftchart::Source* item = mItems.takeAt(from);
        mItems.insert((to > from ? to - 1 : to), item);
    } else {
        qWarning() << "move element from out the bounds";
    }
    emit postItemMoved();

    return true;
}

int SourceList::indexOf(Fftchart::Source *item) const noexcept
{
    return mItems.indexOf(item);
}
bool SourceList::save(const QUrl &fileName) const noexcept
{
    QFile saveFile(fileName.toLocalFile());
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open file");
        return false;
    }

    QJsonObject object;
    object["type"] = "sourcelsist";

    QJsonArray data;
    for (int i = 0; i < mItems.size(); ++i) {
        auto item = mItems.at(i);
        QJsonObject itemJson;
        itemJson["type"] = item->objectName();
        itemJson["data"] = item->toJSON();
        data.append(itemJson);
    }
    object["list"] = data;

    QJsonDocument document(object);
    if (saveFile.write(document.toJson(QJsonDocument::JsonFormat::Compact)) != -1) {
        return true;
    }

    return false;
}
bool SourceList::load(const QUrl &fileName) noexcept
{
    QFile loadFile(fileName.toLocalFile());
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open file");
        return false;
    }
    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadedDocument(QJsonDocument::fromJson(saveData));
    if (loadedDocument.isNull() || loadedDocument.isEmpty())
        return false;

    enum LoadType {List, Stored};
    static std::map<QString, LoadType> typeMap = {
        {"sourcelsist", List},
        {"stored",      Stored},
    };

    if (typeMap.find(loadedDocument["type"].toString()) != typeMap.end()) {
        switch(typeMap.at(loadedDocument["type"].toString())) {
            case List:
                m_currentFile = fileName;
                return loadList(loadedDocument);

            case Stored:
                return loadStored(loadedDocument["data"].toObject());
        }
    }

    return false;
}
bool SourceList::importTxt(const QUrl &fileName) noexcept
{
    QFile file(fileName.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open file");
        return false;
    }

    QString notes;
    char line[1024];
    bool fOk, mOk;
    float frequency, magnitude, maxMagnitude, coherence = 1.f;
    complex phase;

    std::vector<Fftchart::Source::FTData> d;
    d.reserve(480); //48 ppo
    auto s = new Stored();

    while (file.readLine(line, 1024) > 0) {
        QString qLine(line);
        notes += line;
        auto list = qLine.split("\t");
        fOk = mOk = false;

        if (list.size() > 1) {
            frequency = list[0].replace(",", ".").toFloat(&fOk);
            magnitude = list[1].replace(",", ".").toFloat(&mOk);
            phase.polar(M_PI * (list.size() > 2 ? list[2].replace(",", ".").toFloat() : 0) / 180.f);
            coherence = (list.size() > 3 ? list[3].replace(",", ".").toFloat() : 1);

            if (fOk && mOk && list.size() > 1) {
                if (magnitude > maxMagnitude) {
                    maxMagnitude = magnitude;
                }

                d.push_back({
                            frequency,
                            magnitude,
                            0,
                            phase,
                            coherence
                        });
            }
        }
    }

    for(auto& row : d) {
        auto m = row.module;
        row.module = std::pow(10.f, (m - maxMagnitude) / 20.f);
        row.magnitude = std::pow(10.f, (m - maxMagnitude + 15.f) / 20.f);
    }

    s->copyFrom(d.size(), 0, d.data(), nullptr);
    s->setName("Imported");
    s->setNotes(notes);
    s->setActive(true);
    appendItem(s, true);
    return true;

}
int SourceList::selectedIndex() const
{
    return m_selected;
}

Fftchart::Source *SourceList::selected() const noexcept
{
    return mItems.value(m_selected);
}

void SourceList::setSelected(int selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        emit selectedChanged();
    }
}

bool SourceList::loadList(const QJsonDocument &document) noexcept
{
    enum LoadType {Measurement, Stored};
    static std::map<QString, LoadType> typeMap = {
        {"Measurement", Measurement},
        {"Stored",      Stored},
    };

    clean();
    QJsonArray list = document["list"].toArray();

    for (const auto item : list) {
        auto object = item.toObject();

        if (typeMap.find(object["type"].toString()) == typeMap.end())
            continue;

        switch(typeMap.at(object["type"].toString())) {
            case Measurement:
                loadMeasurement(object["data"].toObject());
            break;

            case Stored:
                loadStored(object["data"].toObject());
            break;
        }
    }

    return true;
}
bool SourceList::loadMeasurement(const QJsonObject &data) noexcept
{
    if (data.isEmpty())
        return false;

    auto *m = new Measurement();
    m->fromJSON(data);
    appendItem(m, false);
    nextColor();

    return true;
}
bool SourceList::loadStored(const QJsonObject &data) noexcept
{
    if (data.isEmpty())
        return false;

    auto s = new Stored();
    s->fromJSON(data);
    appendItem(s, false);
    nextColor();
    return true;
}
Union *SourceList::addUnion()
{
    auto *s = new Union();
    appendItem(s, true);
    return s;
}

ELC *SourceList::addElc()
{
    auto *s = new ELC();
    appendItem(s, true);
    return s;
}
Measurement *SourceList::addMeasurement()
{
    auto *m = new Measurement();
    appendItem(m, true);
    return m;
}
void SourceList::appendNone()
{
    mItems.prepend(nullptr);
}
void SourceList::appendItem(Fftchart::Source *item, bool autocolor)
{
    emit preItemAppended();

    if (autocolor) {
        item->setColor(nextColor());
    }
    mItems.append(item);

    emit postItemAppended(item);
}
void SourceList::removeItem(Fftchart::Source *item, bool deleteItem)
{
    for (int i = 0; i < mItems.size(); ++i) {
        if (mItems.at(i) == item) {
            auto item = get(i);
            emit preItemRemoved(i);
            mItems.removeAt(i);
            emit postItemRemoved();
            if (deleteItem)
                item->deleteLater();
            break;
        }
    }
}
QColor SourceList::nextColor()
{
    colorIndex += 3;
    if (colorIndex >= colors.length()) {
        colorIndex -= colors.length();
    }

    return colors.at(colorIndex);
}
