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

#include "mouse/CMouseEditLine.h"
#include "mouse/CScrOptPoint.h"
#include "mouse/CScrOptEditLine.h"
#include "mouse/CScrOptRange.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/CGisDraw.h"
#include "gis/CGisWidget.h"
#include "canvas/CCanvas.h"
#include "GeoMath.h"


#include <QtWidgets>

CMouseEditLine::CMouseEditLine(const QPointF& point, CGisDraw * gis, CCanvas * parent)
    : IMouse(gis, parent)
    , state(eStateAddPointFwd)
    , idxFocus(0)
    , idxStart(-1)
    , idxStop(-1)
{
    cursor  = QCursor(QPixmap(":/cursors/cursorMoveLine.png"),0,0);

    // create a single point line
    coords1 << point;
    line    << point;
    gis->convertRad2Px(line);

    // start with adding trackpints imediately
    slotAddPoint2();

    // create permanent line edit on screen options
    scrOptEditLine = new CScrOptEditLine(canvas);
    scrOptEditLine->pushSaveOrig->hide(); // hide as there is no original
    connect(scrOptEditLine->pushSaveNew, SIGNAL(clicked()), this, SLOT(slotCopyToNew()));
    connect(scrOptEditLine->pushAbort, SIGNAL(clicked()), this, SLOT(slotAbort()));
}

CMouseEditLine::CMouseEditLine(CGisItemTrk &trk, CGisDraw *gis, CCanvas *parent)
    : IMouse(gis, parent)
    , state(eStateIdle)
    , idxFocus(-1)
    , idxStart(-1)
    , idxStop(-1)
{
    cursor  = QCursor(QPixmap(":/cursors/cursorMoveLine.png"),0,0);
    key     = trk.getKey();

    // reset any focus the track might have.
    trk.setMouseFocusByPoint(NOPOINT, CGisItemTrk::eFocusMouseMove);
    trk.setMouseFocusByPoint(NOPOINT, CGisItemTrk::eFocusMouseClick);
    if(trk.hasUserFocus())
    {
        trk.gainUserFocus(false);
    }

    // get a local copy of the track data
    const CGisItemTrk::trk_t& data = trk.getTrackData();
    foreach (const CGisItemTrk::trkseg_t& seg, data.segs)
    {
        foreach(const CGisItemTrk::trkpt_t& pt, seg.pts)
        {
            if(pt.flags & CGisItemTrk::trkpt_t::eDeleted)
            {
                continue;
            }
            coords1 << QPointF(pt.lon * DEG_TO_RAD, pt.lat * DEG_TO_RAD);
        }
    }

    // calculate a pixel polyline from track coordinates
    line = coords1;
    gis->convertRad2Px(line);

    // create permanent line edit on screen options
    scrOptEditLine = new CScrOptEditLine(canvas);
    connect(scrOptEditLine->pushSaveOrig, SIGNAL(clicked()), this, SLOT(slotCopyToOrig()));
    connect(scrOptEditLine->pushSaveNew, SIGNAL(clicked()), this, SLOT(slotCopyToNew()));
    connect(scrOptEditLine->pushAbort, SIGNAL(clicked()), this, SLOT(slotAbort()));

    /*
        trigger complete update of GIS components to make sure all changes to
        the originating object are reflected on the canvas->
    */
    canvas->slotTriggerCompleteUpdate(CCanvas::eRedrawGis);
}

CMouseEditLine::~CMouseEditLine()
{
    delete scrOptPoint;
    delete scrOptEditLine;
    delete scrOptRange;
}

void CMouseEditLine::drawLine(const QPolygonF &l, QPainter& p)
{
    p.setPen(QPen(Qt::magenta, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(l);
}

void CMouseEditLine::drawPointOfFocus(QPainter& p)
{
    if(idxFocus < 0)
    {
        return;
    }

    const QPointF& pt = line[idxFocus];

    p.setPen(Qt::darkBlue);
    p.setBrush(QColor(255,255,255,200));
    p.drawEllipse(pt, 6, 6);

    QPixmap bullet("://icons/8x8/bullet_magenta.png");
    p.drawPixmap(pt.x() - 3, pt.y() - 3, bullet);
}

void CMouseEditLine::drawBullets(const QPolygonF &l, QPainter& p)
{
    QPixmap bullet("://icons/8x8/bullet_magenta.png");
    foreach(const QPointF& pt, l)
    {
        p.drawPixmap(pt.x() - 3, pt.y() - 3, bullet);
    }
}

void CMouseEditLine::drawHighlight1(QPainter& p)
{
    if(idxStart < 0 || idxFocus < 0)
    {
        return;
    }

    int pos = idxFocus < idxStart ? idxFocus : idxStart;
    int len = qAbs(idxFocus - idxStart) + 1;

    QPolygonF highlight = line.mid(pos,len);

    p.setPen(QPen(Qt::red, 12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(highlight);
}

void CMouseEditLine::drawHighlight2(QPainter& p)
{
    if(idxStart < 0 || idxStop < 0)
    {
        return;
    }

    int pos = idxStop < idxStart ? idxStop : idxStart;
    int len = qAbs(idxStop - idxStart) + 1;

    QPolygonF highlight = line.mid(pos,len);

    p.setPen(QPen(Qt::red, 12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(highlight);
}

void CMouseEditLine::draw(QPainter& p, bool needsRedraw, const QRect &rect)
{
    if(needsRedraw)
    {
        line = coords1;
        gis->convertRad2Px(line);
        newLine = newCoords;
        gis->convertRad2Px(newLine);
    }

    switch(state)
    {
        case eStateIdle:
        case eStatePointSelected:
            drawLine(line, p);
            drawBullets(line, p);
            drawPointOfFocus(p);
            break;

        case eStateSelectRange:
            drawLine(line, p);
            drawHighlight1(p);
            drawBullets(line, p);
            drawPointOfFocus(p);
            break;

        case eStateRangeSelected:
            drawLine(line, p);
            drawHighlight2(p);
            drawBullets(line, p);
            drawPointOfFocus(p);
            break;

        case eStateMovePoint:
            drawLine(line, p);
            drawBullets(line, p);
            drawPointOfFocus(p);
            break;

        case eStateAddPointFwd:
        case eStateAddPointBwd:
            if(!(idxStart < 0))
            {
                drawLine(line.mid(0,idxStart + 1), p);
                p.drawLine(line[idxStart], newLine.first());
            }

            drawLine(newLine, p);

            if(idxStop < line.size())
            {
                drawLine(line.mid(idxStop,-1), p);
                p.drawLine(newLine.last(), line[idxStop]);
            }

            drawBullets(line, p);
            drawBullets(newLine, p);

            break;
        default:;
    }


    if(!scrOptPoint.isNull())
    {
        scrOptPoint->draw(p);
    }
    if(!scrOptRange.isNull())
    {
        scrOptRange->draw(p);
    }
}

void CMouseEditLine::mousePressEvent(QMouseEvent * e)
{
    point  = e->pos();
    if(e->button() == Qt::RightButton)
    {
        switch(state)
        {

            case eStateSelectRange:                
                cursor  = QCursor(QPixmap(":/cursors/cursorMoveLine.png"),0,0);
                QApplication::restoreOverrideCursor();
                QApplication::setOverrideCursor(cursor);
                //break; no break fall thru

            case eStateRangeSelected:
                delete scrOptRange;

                state       = eStateIdle;
                idxFocus    = -1;
                idxStart    = -1;
                idxStop     = -1;

                canvas->update();
                break;

            case eStateMovePoint:
                state       = eStateIdle;
                idxFocus    = -1;
                coords1     = save;
                line        = coords1;
                gis->convertRad2Px(line);

                canvas->update();
                break;

            case eStateAddPointBwd:
            case eStateAddPointFwd:
                if(QMessageBox::question(canvas, tr("Add points?"), tr("Add points to temporary line?"), QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
                {
                    if(state == eStateAddPointBwd)
                    {
                        newCoords.pop_front();
                    }

                    if(state == eStateAddPointFwd)
                    {
                        newCoords.pop_back();
                    }

                    save        = coords1.mid(0, idxStart + 1) + newCoords + coords1.mid(idxStop, -1);
                    coords1     = save;
                    line        = coords1;
                    gis->convertRad2Px(line);
                }

                state       = eStateIdle;
                idxFocus    = -1;
                idxStart    = -1;
                idxStop     = -1;

                cursor  = QCursor(QPixmap(":/cursors/cursorMoveLine.png"),0,0);
                QApplication::restoreOverrideCursor();
                QApplication::setOverrideCursor(cursor);

                canvas->update();
                break;

            default:
                delete scrOptPoint;
                delete scrOptRange;

                state       = eStateIdle;
                idxFocus    = -1;
                idxStart    = -1;
                idxStop     = -1;
                canvas->update();
        }
    }
    else if(e->button() == Qt::LeftButton)
    {
        switch(state)
        {
            case eStateIdle:
            {
                if(idxFocus >= 0)
                {
                    scrOptPoint = new CScrOptPoint(line[idxFocus], canvas);

                    // set icon for add points towards start (backward/eStateAddPointBwd)
                    if(idxFocus == 0)
                    {
                        // point is first of line
                        qreal a1, a2;
                        if(coords1.size() > 1)
                        {
                            const QPointF& pt1 = coords1[idxFocus];
                            const QPointF& pt2 = coords1[idxFocus + 1];
                            GPS_Math_Distance(pt1.x(), pt1.y(), pt2.x(), pt2.y(), a1, a2);
                        }
                        else
                        {
                            // only one point in the line
                            a1 = 90;
                        }

                        QPixmap pix("://icons/32x32/ToTop.png");
                        QTransform trans;
                        trans.rotate(a1 + 180);

                        pix = pix.transformed(trans, Qt::SmoothTransformation);

                        scrOptPoint->toolAdd1->setIcon(pix);
                    }
                    else
                    {
                        // point in the middle of the line
                        qreal a1, a2;
                        const QPointF& pt1 = coords1[idxFocus];
                        const QPointF& pt2 = coords1[idxFocus - 1];
                        GPS_Math_Distance(pt1.x(), pt1.y(), pt2.x(), pt2.y(), a1, a2);

                        QPixmap pix("://icons/32x32/Up.png");
                        QTransform trans;
                        trans.rotate(a1);

                        pix = pix.transformed(trans, Qt::SmoothTransformation);

                        scrOptPoint->toolAdd1->setIcon(pix);
                    }

                    // set icon for add points towards end (forward/eStateAddPointFwd)
                    if(idxFocus == (line.size() - 1))
                    {
                        // point is last of line
                        qreal a1, a2;
                        if(coords1.size() > 1)
                        {
                            const QPointF& pt1 = coords1[idxFocus];
                            const QPointF& pt2 = coords1[idxFocus - 1];
                            GPS_Math_Distance(pt1.x(), pt1.y(), pt2.x(), pt2.y(), a1, a2);
                        }
                        else
                        {
                            // only one point in the line
                            a1 = -90;
                        }

                        QPixmap pix("://icons/32x32/ToTop.png");
                        QTransform trans;
                        trans.rotate(a1 + 180);

                        pix = pix.transformed(trans, Qt::SmoothTransformation);

                        scrOptPoint->toolAdd2->setIcon(pix);


                    }
                    else
                    {
                        // point in the middle of the line
                        qreal a1, a2;
                        const QPointF& pt1 = coords1[idxFocus];
                        const QPointF& pt2 = coords1[idxFocus + 1];
                        GPS_Math_Distance(pt1.x(), pt1.y(), pt2.x(), pt2.y(), a1, a2);

                        QPixmap pix("://icons/32x32/Up.png");
                        QTransform trans;
                        trans.rotate(a1);

                        pix = pix.transformed(trans, Qt::SmoothTransformation);

                        scrOptPoint->toolAdd2->setIcon(pix);

                    }

                    connect(scrOptPoint->toolDelete, SIGNAL(clicked()), this, SLOT(slotDeletePoint()));
                    connect(scrOptPoint->toolSelectRange, SIGNAL(clicked()), this, SLOT(slotSelectRange()));
                    connect(scrOptPoint->toolMove, SIGNAL(clicked()), this, SLOT(slotMovePoint()));
                    connect(scrOptPoint->toolAdd1, SIGNAL(clicked()), this, SLOT(slotAddPoint1()));
                    connect(scrOptPoint->toolAdd2, SIGNAL(clicked()), this, SLOT(slotAddPoint2()));
                    canvas->update();

                    state = eStatePointSelected;
                }
                break;
            }
            case eStateSelectRange:
            {
                state   = eStateRangeSelected;
                idxStop = idxFocus;

                scrOptRange = new CScrOptRange(line[idxStop], canvas);
                connect(scrOptRange->toolDelete, SIGNAL(clicked()), this, SLOT(slotDeleteRange()));

                cursor  = QCursor(QPixmap(":/cursors/cursorMoveLine.png"),0,0);
                QApplication::restoreOverrideCursor();
                QApplication::setOverrideCursor(cursor);

                canvas->update();
                break;
            }
            case eStateAddPointFwd:
            {
                newLine.append(newLine.last());
                newCoords.append(newCoords.last());
                idxFocus++;

                canvas->update();
                break;
            }
            case eStateAddPointBwd:
            {
                newLine.prepend(newLine.first());
                newCoords.prepend(newCoords.first());
                idxFocus = 0;

                canvas->update();
                break;
            }
            default:
            {
                delete scrOptPoint;
                delete scrOptRange;

                state       = eStateIdle;
                idxFocus    = -1;
                idxStart    = -1;
                idxStop     = -1;
                canvas->update();
            }
        }
    }
}

void CMouseEditLine::mouseMoveEvent(QMouseEvent * e)
{
    point  = e->pos();    

    switch(state)
    {
        case eStateIdle:
        case eStateSelectRange:
        {
            if(!scrOptEditLine->rect().contains(point))
            {
                panCanvas(point);
            }

            int old = idxFocus;
            idxFocus = getPointCloseBy(point);
            if(old != idxFocus)
            {
                canvas->update();
            }
            break;
        }
        case eStateMovePoint:
        {
            panCanvas(point);

            QPointF pt      = point;
            line[idxFocus]  = pt;
            gis->convertPx2Rad(pt);
            coords1[idxFocus]  = pt;

            canvas->update();
            break;
        }
        case eStateAddPointBwd:
        case eStateAddPointFwd:
        {
            panCanvas(point);

            QPointF pt          = point;
            newLine[idxFocus]   = pt;
            gis->convertPx2Rad(pt);
            newCoords[idxFocus] = pt;

            canvas->update();

        }
        default:;
    }
}

void CMouseEditLine::mouseReleaseEvent(QMouseEvent *e)
{

}

void CMouseEditLine::wheelEvent(QWheelEvent * e)
{
    canvas->update();
}


int CMouseEditLine::getPointCloseBy(const QPoint& screenPos)
{
    qint32 i    = 0;
    qint32 idx  = -1;
    qint32  d   = NOINT;
    foreach(const QPointF& point, line)
    {
        int tmp = (screenPos - point).manhattanLength();
        if(tmp < d)
        {
            idx = i;
            d   = tmp;
        }
        i++;
    }

    if(d > 40) idx = -1;

    return idx;
}

void CMouseEditLine::slotDeletePoint()
{
    if(idxFocus < 0)
    {
        return;
    }
    scrOptPoint->deleteLater();

    coords1.remove(idxFocus);
    line.remove(idxFocus);

    idxFocus  = -1;
    state       = eStateIdle;

    canvas->update();
}

void CMouseEditLine::slotSelectRange()
{
    if(idxFocus < 0)
    {
        return;
    }
    scrOptPoint->deleteLater();

    idxStart    = idxFocus;
    state       = eStateSelectRange;

    cursor  = QCursor(QPixmap(":/cursors/cursorSelectRange.png"),0,0);
    QApplication::restoreOverrideCursor();
    QApplication::setOverrideCursor(cursor);

}

void CMouseEditLine::slotDeleteRange()
{
    if(idxStart < 0 || idxStop < 0)
    {
        return;
    }
    scrOptRange->deleteLater();

    int len = qAbs(idxStop - idxStart) + 1;
    int idx = idxStart < idxStop ? idxStart : idxStop;
    coords1.remove(idx,len);
    line = coords1;
    gis->convertRad2Px(line);

    state = eStateIdle;
    idxFocus  = -1;
    idxStart    = -1;
    idxStop     = -1;

    canvas->update();
}

void CMouseEditLine::slotMovePoint()
{
    if(idxFocus < 0)
    {
        return;
    }
    scrOptPoint->deleteLater();

    save    = coords1;
    state   = eStateMovePoint;

    canvas->update();
}

void CMouseEditLine::slotAddPoint1()
{
    if(idxFocus < 0)
    {
        return;
    }
    scrOptPoint->deleteLater();

    // set point with focus as temporary initial point
    newCoords.clear();    
    newCoords << coords1[idxFocus];
    newLine.clear();
    newLine   << line[idxFocus];

    // mark gap to insert points
    idxStart = idxFocus - 1;
    idxStop  = idxFocus;

    // set focus to first point in newLine
    idxFocus = 0;

    state   = eStateAddPointBwd;
    cursor  = QCursor(QPixmap(":/cursors/cursorAdd.png"),0,0);
    QApplication::restoreOverrideCursor();
    QApplication::setOverrideCursor(cursor);

    canvas->update();
}

void CMouseEditLine::slotAddPoint2()
{
    if(idxFocus < 0)
    {
        return;
    }
    scrOptPoint->deleteLater();

    // set point with focus as temporary initial point
    newCoords.clear();
    newCoords << coords1[idxFocus];    
    newLine.clear();
    newLine   << line[idxFocus];

    // mark gap to insert points
    idxStart = idxFocus;
    idxStop  = idxFocus + 1;

    // set focus to first point in newLine
    idxFocus = 0;

    state   = eStateAddPointFwd;
    cursor  = QCursor(QPixmap(":/cursors/cursorAdd.png"),0,0);
    QApplication::restoreOverrideCursor();
    QApplication::setOverrideCursor(cursor);

    canvas->update();
}


void CMouseEditLine::slotAbort()
{
    canvas->resetMouse();
    canvas->update();
}

void CMouseEditLine::slotCopyToOrig()
{    
    if(coords1.size() < 2)
    {
        return;
    }

    int res = QMessageBox::warning(canvas, tr("Warning!"), tr("This will replace all data of the orignal by a simple line of coordinates. All other data will be lost permanently."), QMessageBox::Ok|QMessageBox::Abort, QMessageBox::Ok);

    if(res != QMessageBox::Ok)
    {
        return;
    }

    IGisLine * l = dynamic_cast<IGisLine*>(CGisWidget::self().getItemByKey(key));
    if(l != 0)
    {
        l->replaceData(coords1);
    }
    canvas->resetMouse();
    canvas->slotTriggerCompleteUpdate(CCanvas::eRedrawGis);
}

void CMouseEditLine::slotCopyToNew()
{
    if(coords1.size() < 2)
    {
        return;
    }

    CGisProject * project = CGisWidget::self().selectProject();
    if(project == 0)
    {
        return;
    }

    /// @todo make this independent from track
    QString name;
    CGisItemTrk * trk = dynamic_cast<CGisItemTrk*>(CGisWidget::self().getItemByKey(key));
    if(trk != 0)
    {
        name = trk->getName();
    }

    name = QInputDialog::getText(0, QObject::tr("Edit name..."), QObject::tr("Enter new track name."), QLineEdit::Normal, name);

    if(name.isEmpty())
    {
        return;
    }

    new CGisItemTrk(coords1,name, project, -1);

    canvas->resetMouse();
    canvas->slotTriggerCompleteUpdate(CCanvas::eRedrawGis);
}
