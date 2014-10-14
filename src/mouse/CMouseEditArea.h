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

#ifndef CMOUSEEDITAREA_H
#define CMOUSEEDITAREA_H

#include "mouse/IMouseEditLine.h"

class CGisItemOvlArea;

class CMouseEditArea : public IMouseEditLine
{
    Q_OBJECT
    public:
        CMouseEditArea(const QPointF& point, CGisDraw * gis, CCanvas * parent);
        CMouseEditArea(CGisItemOvlArea &area, CGisDraw * gis, CCanvas * parent);
        virtual ~CMouseEditArea();

        void mousePressEvent(QMouseEvent * e);

    protected slots:
        void slotCopyToNew();

    protected:
        virtual void drawLine(const QPolygonF& l, QPainter& p);
        IGisLine * getGisLine();

    private:
        QString     key;

};

#endif //CMOUSEEDITAREA_H

