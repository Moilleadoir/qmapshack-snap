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

#ifndef CSCROPTWPT_H
#define CSCROPTWPT_H

#include "mouse/IScrOpt.h"

#include <QWidget>
#include "ui_IScrOptWpt.h"

class CGisItemWpt;
class IMouse;

class CScrOptWpt : public IScrOpt , public QWidget, private Ui::IScrOptWpt
{
    public:
        CScrOptWpt(CGisItemWpt * wpt, IMouse *parent);
        virtual ~CScrOptWpt();

        void draw(QPainter& p){}

    private:
        CGisItemWpt * wpt;
        QString info;
};

#endif //CSCROPTWPT_H

