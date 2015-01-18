/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#ifndef CDEVICEWATCHERLINUX_H
#define CDEVICEWATCHERLINUX_H

#include "device/IDeviceWatcher.h"

class QSocketNotifier;

class CDeviceWatcherLinux : public IDeviceWatcher
{
    Q_OBJECT
public:
    CDeviceWatcherLinux(QObject * parent);
    virtual ~CDeviceWatcherLinux();

private slots:
    void slotParseDeviceInfo();

private:
    void parseLine(const QByteArray &line);
    int netlinkSocket;
    QSocketNotifier * notifier;
};

#endif //CDEVICEWATCHERLINUX_H

