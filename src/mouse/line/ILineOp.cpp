/**********************************************************************************************
    Copyright (C) 2014-2015 Oliver Eichler oliver.eichler@gmx.de

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

#include "canvas/CCanvas.h"
#include "gis/rte/router/CRouterSetup.h"
#include "mouse/line/ILineOp.h"
#include "mouse/line/IMouseEditLine.h"

#include <QtWidgets>

ILineOp::ILineOp(SGisLine& points, CGisDraw *gis, CCanvas *canvas, IMouseEditLine *parent)
    : QObject(parent)
    , parentHandler(parent)
    , points(points)
    , canvas(canvas)
    , gis(gis)
    , mapMove(false)
    , mapDidMove(false)
{
}

ILineOp::~ILineOp()
{
}

void ILineOp::mousePressEvent(QMouseEvent * e)
{
    const QPoint& pos = e->pos();

    if(e->button() == Qt::LeftButton)
    {
        lastPos     = pos;
        mapMove     = true;
        mapDidMove  = false;
    }
}

void ILineOp::mouseMoveEvent(QMouseEvent * e)
{
    const QPoint& pos = e->pos();

    if(mapMove)
    {
        if(pos != lastPos)
        {
            QPoint delta = pos - lastPos;
            canvas->moveMap(delta);
            mapDidMove  = true;
        }
    }

    lastPos = pos;
}

void ILineOp::mouseReleaseEvent(QMouseEvent *e)
{
    mapMove     = false;
    mapDidMove  = false;
}

void ILineOp::finalizeOperation(qint32 idx)
{
    if(idx == NOIDX)
    {
        return;
    }

    if(parentHandler->useAutoRouting())
    {
        if(idx > 0)
        {
            QPolygonF subs;
            IGisLine::point_t& pt1 = points[idx - 1];
            IGisLine::point_t& pt2 = points[idx];
            CRouterSetup::self().calcRoute(pt1.coord, pt2.coord, subs);

            pt1.subpts.clear();
            foreach(const QPointF &sub, subs)
            {
                pt1.subpts << IGisLine::subpt_t(sub);
            }
        }

        if(idx < (points.size() - 1))
        {
            QPolygonF subs;
            IGisLine::point_t& pt1 = points[idx];
            IGisLine::point_t& pt2 = points[idx + 1];
            CRouterSetup::self().calcRoute(pt1.coord, pt2.coord, subs);

            pt1.subpts.clear();
            foreach(const QPointF &sub, subs)
            {
                pt1.subpts << IGisLine::subpt_t(sub);
            }
        }
    }
}

qint32 ILineOp::isCloseTo(const QPoint& pos)
{
    qint32 min = NOINT;
    qint32 idx = NOIDX;
    const int N = points.size();
    for(int i = 0; i < N; i++)
    {
        const IGisLine::point_t& pt = points[i];

        qint32 d = (pos - pt.pixel).manhattanLength();
        if((d < 20) && (d < min))
        {
            min = d;
            idx = i;
        }
    }

    return idx;
}


inline qreal sqr(qreal a)
{
    return a*a;
}
inline qreal sqrlen(const QPointF &a)
{
    return sqr(a.x()) + sqr(a.y());
}

qreal sqr_distance(const QPolygonF &points, const QPointF &q)
{
    const size_t count = points.size();

    QPointF b = points[0];
    QPointF dbq = b - q;
    qreal dist = sqrlen(dbq);

    for (size_t i = 1; i<count; ++i)
    {
        const QPointF a = b;
        const QPointF daq = dbq;
        b = points[i];
        dbq = b - q;

        const QPointF dab = a - b;

        const qreal inv_sqrlen = 1./sqrlen(dab);
        const qreal t = (dab.x()*daq.x() + dab.y()*daq.y())*inv_sqrlen;
        if (t < 0.)
        {
            continue;
        }
        qreal current_dist;
        if (t<=1.)
        {
            current_dist = sqr(dab.x()*dbq.y() - dab.y()*dbq.x())*inv_sqrlen;
        }
        else//t>1.
        {
            current_dist = sqrlen(dbq);
        }
        if (current_dist<dist)
        {
            dist = current_dist;
        }
    }
    return dist;
}
qint32 ILineOp::isCloseToLine(const QPoint& pos)
{
    qint32 idx = NOIDX;
    qreal dist = 60;

    for(int i = 0; i < points.size() - 1; i++)
    {
        QPolygonF line;
        const IGisLine::point_t& pt1 = points[i];
        const IGisLine::point_t& pt2 = points[i + 1];

        if(pt1.subpts.isEmpty())
        {
            line << pt1.pixel << pt2.pixel;
        }
        else
        {
            foreach(const IGisLine::subpt_t& pt, pt1.subpts)
            {
                line << pt.pixel;
            }
            line << pt2.pixel;
        }

        qreal d = sqr_distance(line, pos);
        if(d < dist)
        {
            dist = d;
            idx  = i;
        }
    }

    return idx;
}

