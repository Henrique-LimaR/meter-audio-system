/**
 *  OSM
 *  Copyright (C) 2021  Pavel Smokotnin

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
#ifndef FILESYSTEM_IOSDIALOGPLUGIN_H
#define FILESYSTEM_IOSDIALOGPLUGIN_H

#include "../dialogPlugin.h"

namespace filesystem {

class IosDialogPlugin : public DialogPlugin
{
    Q_OBJECT

public:
    IosDialogPlugin(QObject *parent = nullptr);
    void open(const QQuickWindow *window) override;
};

} // namespace filesystem

#endif // FILESYSTEM_IOSDIALOGPLUGIN_H
