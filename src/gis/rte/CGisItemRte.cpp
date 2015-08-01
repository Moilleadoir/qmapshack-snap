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


#include "CMainWindow.h"
#include "GeoMath.h"
#include "canvas/CCanvas.h"
#include "gis/CGisDraw.h"
#include "gis/CGisListWks.h"
#include "gis/WptIcons.h"
#include "gis/prj/IGisProject.h"
#include "gis/rte/CDetailsRte.h"
#include "gis/rte/CGisItemRte.h"
#include "gis/rte/CScrOptRte.h"

#include <QtWidgets>
#include <QtXml>
#include <proj_api.h>

const QPen CGisItemRte::penBackground(Qt::white, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
IGisItem::key_t CGisItemRte::keyUserFocus;

#define MIN_DIST_FOCUS 200

/// used to create a copy of route with new parent
CGisItemRte::CGisItemRte(const CGisItemRte& parentRte, IGisProject * project, int idx, bool clone)
    : IGisItem(project, eTypeRte, idx)
    , penForeground(Qt::darkBlue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , penForegroundFocus(Qt::magenta, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , totalDistance(NOFLOAT)
    , totalDays(NOINT)
    , mouseMoveFocus(0)
{
    *this = parentRte;
    key.project = project->getKey();
    key.device  = project->getDeviceKey();

    if(clone)
    {
        rte.name += QObject::tr("_Clone");
        key.clear();
        history.events.clear();
    }

    if(parentRte.isOnDevice())
    {
        flags |= eFlagWriteAllowed;
    }
    else if(!parentRte.isReadOnly())
    {
        flags |= eFlagWriteAllowed;
    }
    else
    {
        flags &= ~eFlagWriteAllowed;
    }



    setupHistory();
    deriveSecondaryData();
    updateDecoration(eMarkChanged, eMarkNone);
}

/// used to create route from GPX file
CGisItemRte::CGisItemRte(const QDomNode& xml, IGisProject *parent)
    : IGisItem(parent, eTypeRte, parent->childCount())
    , penForeground(Qt::darkBlue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , penForegroundFocus(Qt::magenta, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , totalDistance(NOFLOAT)
    , totalDays(NOINT)
    , mouseMoveFocus(0)
{
    // --- start read and process data ----
    readRte(xml, rte);
    // --- stop read and process data ----

    setupHistory();
    deriveSecondaryData();
    updateDecoration(eMarkNone, eMarkNone);
}

CGisItemRte::CGisItemRte(const history_t& hist, IGisProject * project)
    : IGisItem(project, eTypeRte, project->childCount())
    , penForeground(Qt::darkBlue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , penForegroundFocus(Qt::magenta, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , totalDistance(NOFLOAT)
    , totalDays(NOINT)
    , mouseMoveFocus(0)
{
    history = hist;
    loadHistory(hist.histIdxCurrent);
    deriveSecondaryData();
}

CGisItemRte::CGisItemRte(quint64 id, QSqlDatabase& db, IGisProject * project)
    : IGisItem(project, eTypeRte, NOIDX)
    , penForeground(Qt::darkBlue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , penForegroundFocus(Qt::magenta, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , totalDistance(NOFLOAT)
    , totalDays(NOINT)
    , mouseMoveFocus(0)
{
    loadFromDb(id, db);
}

CGisItemRte::CGisItemRte(const SGisLine &l, const QString &name, IGisProject *project, int idx)
    : IGisItem(project, eTypeRte, idx)
    , penForeground(Qt::darkBlue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , penForegroundFocus(Qt::magenta, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
    , totalDistance(NOFLOAT)
    , totalDays(NOINT)
    , mouseMoveFocus(0)
{
    rte.name = name;
    readRouteDataFromGisLine(l);

    flags |=  eFlagCreatedInQms|eFlagWriteAllowed;

    setupHistory();
    updateDecoration(eMarkChanged, eMarkNone);
}

CGisItemRte::~CGisItemRte()
{
    // reset user focus if focused on this track
    if(key == keyUserFocus)
    {
        keyUserFocus.clear();
    }
}

bool CGisItemRte::isCalculated()
{
    bool yes = true;
    foreach(const rtept_t &pt, rte.pts)
    {
        if((pt.fakeSubpt.lat == NOFLOAT) || (pt.fakeSubpt.lon == NOFLOAT))
        {
            yes = false;
        }
    }

    return yes;
}

void CGisItemRte::deriveSecondaryData()
{
    qreal north = -90;
    qreal east  = -180;
    qreal south =  90;
    qreal west  =  180;

    foreach(const rtept_t &rtept, rte.pts)
    {
        if(rtept.lon < west)
        {
            west    = rtept.lon;
        }
        if(rtept.lon > east)
        {
            east    = rtept.lon;
        }
        if(rtept.lat < south)
        {
            south   = rtept.lat;
        }
        if(rtept.lat > north)
        {
            north   = rtept.lat;
        }

        foreach(const subpt_t &subpt, rtept.subpts)
        {
            if(subpt.lon < west)
            {
                west    = subpt.lon;
            }
            if(subpt.lon > east)
            {
                east    = subpt.lon;
            }
            if(subpt.lat < south)
            {
                south   = subpt.lat;
            }
            if(subpt.lat > north)
            {
                north   = subpt.lat;
            }
        }
    }

    boundingRect = QRectF(QPointF(west * DEG_TO_RAD, north * DEG_TO_RAD), QPointF(east * DEG_TO_RAD,south * DEG_TO_RAD));
}

void CGisItemRte::edit()
{
    CDetailsRte dlg(*this, &CMainWindow::self());
    dlg.exec();
}

void CGisItemRte::setSymbol()
{
    icon = QPixmap("://icons/32x32/Route.png").scaled(22,22, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setIcon(CGisListWks::eColumnIcon, icon);
}

void CGisItemRte::setName(const QString& str)
{
    setText(CGisListWks::eColumnName, str);
    rte.name = str;
    changed(QObject::tr("Changed name."), "://icons/48x48/EditText.png");
}

void CGisItemRte::setComment(const QString& str)
{
    rte.cmt = str;
    changed(QObject::tr("Changed comment"), "://icons/48x48/EditText.png");
}

void CGisItemRte::setDescription(const QString& str)
{
    rte.desc = str;
    changed(QObject::tr("Changed description"), "://icons/48x48/EditText.png");
}

void CGisItemRte::setLinks(const QList<link_t>& links)
{
    rte.links = links;
    changed(QObject::tr("Changed links"), "://icons/48x48/Link.png");
}



QString CGisItemRte::getInfo(bool allowEdit) const
{
    QString val1, unit1, val2, unit2;
    QString str = "<div>";

    if(allowEdit)
    {
        str += "<b>" + toLink(isReadOnly(), "name", getName(), "") + "</b>";
    }
    else
    {
        str += "<div style='font-weight: bold;'>" + getName() + "</div>";
    }

    str += "<br/>\n";
    if(totalDistance != NOFLOAT)
    {
        IUnit::self().meter2distance(totalDistance, val1, unit1);
        str += QObject::tr("Length: %1 %2").arg(val1).arg(unit1);
    }
    else
    {
        str += QObject::tr("Length: -");
    }

    str += "<br/>\n";
    if(totalTime.isValid())
    {
        if(totalDays != 0 && totalDays != NOINT)
        {
            str += QObject::tr("Time: %2 days %1").arg(totalTime.toString()).arg(totalDays);
        }
        else
        {
            str += QObject::tr("Time: %1").arg(totalTime.toString());
        }
    }
    else
    {
        str += QObject::tr("Time: -");
    }

    if(!lastRoutedWith.isEmpty())
    {
        str += "<br/>\n";
        str += QObject::tr("Last time routed:<br/>%1").arg(IUnit::datetime2string(lastRoutedTime, false, boundingRect.center()));
        str += "<br/>\n";
        str += QObject::tr("with %1").arg(lastRoutedWith);
    }
    return str;
}

IScrOpt * CGisItemRte::getScreenOptions(const QPoint& origin, IMouse * mouse)
{
    return new CScrOptRte(this, origin, mouse);
}

QPointF CGisItemRte::getPointCloseBy(const QPoint& screenPos)
{
    QMutexLocker lock(&mutexItems);

    qint32 d    = NOINT;
    QPointF pt  = NOPOINTF;
    foreach(const QPointF &point, line)
    {
        int tmp = (screenPos - point).manhattanLength();
        if(tmp < d)
        {
            pt  = point;
            d   = tmp;
        }
    }

    return pt;
}



bool CGisItemRte::isCloseTo(const QPointF& pos)
{
    QMutexLocker lock(&mutexItems);

    qreal dist = GPS_Math_DistPointPolyline(line, pos);
    return dist < 20;
}

void CGisItemRte::gainUserFocus(bool yes)
{
    keyUserFocus = yes ? key : key_t();
}


void CGisItemRte::looseUserFocus()
{
    if(keyUserFocus == key)
    {
        keyUserFocus.clear();
    }
}



void CGisItemRte::drawItem(QPainter& p, const QPolygonF& viewport, QList<QRectF> &blockedAreas, CGisDraw *gis)
{
    QMutexLocker lock(&mutexItems);

    line.clear();
    if(!isVisible(boundingRect, viewport, gis))
    {
        return;
    }

    QPointF p1 = viewport[0];
    QPointF p2 = viewport[2];
    gis->convertRad2Px(p1);
    gis->convertRad2Px(p2);
    QRectF extViewport(p1,p2);

    QVector<qint32> points;

    foreach(const rtept_t &rtept, rte.pts)
    {
        QPointF pt(rtept.lon * DEG_TO_RAD, rtept.lat * DEG_TO_RAD);

        gis->convertRad2Px(pt);

        line << pt;
        points << 1;

        blockedAreas << QRectF(pt - rtept.focus, rtept.icon.size());
        foreach(const subpt_t &subpt, rtept.subpts)
        {
            QPointF pt(subpt.lon * DEG_TO_RAD, subpt.lat * DEG_TO_RAD);
            gis->convertRad2Px(pt);
            line << pt;
            if(subpt.type != subpt_t::eTypeNone)
            {
                points << 2;
            }
            else
            {
                points << 0;
            }
        }
    }

    p.setPen(penBackground);
    p.drawPolyline(line);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    for(int i = 0; i < line.size(); i++)
    {
        switch(points[i])
        {
        case 1:
            p.drawEllipse(line[i],7,7);
            break;

        case 2:
            p.drawEllipse(line[i],5,5);
            break;
        }
    }

    p.setPen(hasUserFocus() ? penForegroundFocus : penForeground);
    p.setBrush(hasUserFocus() ? penForegroundFocus.color() : penForeground.color());
    drawArrows(line, extViewport, p);
    p.drawPolyline(line);

    p.setPen(Qt::NoPen);
    for(int i = 0; i < line.size(); i++)
    {
        switch(points[i])
        {
        case 1:
            p.setBrush(Qt::red);
            p.drawEllipse(line[i],5,5);
            break;

        case 2:
            p.setBrush(Qt::cyan);
            p.drawEllipse(line[i],3,3);
            break;
        }
    }
}

void CGisItemRte::drawItem(QPainter& p, const QRectF& viewport, CGisDraw * gis)
{
    QMutexLocker lock(&mutexItems);

    if(rte.pts.isEmpty())
    {
        return;
    }

    if(hasUserFocus() && mouseMoveFocus && mouseMoveFocus->lon != NOFLOAT && mouseMoveFocus->lat != NOFLOAT)
    {
        QPointF anchor(mouseMoveFocus->lon, mouseMoveFocus->lat);
        anchor *= DEG_TO_RAD;
        gis->convertRad2Px(anchor);
        p.drawEllipse(anchor, 5, 5);


        QString str, val, unit;
        str += QObject::tr("Time: %1 ").arg(mouseMoveFocus->time.toString());
        IUnit::self().meter2distance(mouseMoveFocus->distance, val, unit);
        str += QObject::tr("Distance: %1 %2").arg(val).arg(unit);
        str += "\n" + mouseMoveFocus->instruction;

        // calculate bounding box of text
        QFont f = CMainWindow::self().getMapFont();
        QFontMetrics fm(f);
        QRect rectText = fm.boundingRect(QRect(0,0,500,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);
        rectText.adjust(-5, -5, 5, 5);
        rectText.moveBottomLeft(anchor.toPoint() + QPoint(-50,-50));


        // create bubble path
        QPainterPath path1;
        path1.addRoundedRect(rectText,5,5);

        QPolygonF poly2;
        poly2 << anchor << (rectText.bottomLeft() + QPointF(10,-5)) << (rectText.bottomLeft() + QPointF(30,-5)) << anchor;
        QPainterPath path2;
        path2.addPolygon(poly2);

        path1 = path1.united(path2);


        p.setFont(f);
        p.setPen(CCanvas::penBorderGray);
        p.setBrush(CCanvas::brushBackWhite);
        p.drawPolygon(path1.toFillPolygon());

        p.save();
        p.translate(5,5);
        p.setPen(Qt::darkBlue);
        p.drawText(rectText, str);
        p.restore();
    }
}

void CGisItemRte::drawLabel(QPainter& p, const QPolygonF& viewport, QList<QRectF> &blockedAreas, const QFontMetricsF &fm, CGisDraw *gis)
{
    QMutexLocker lock(&mutexItems);

    if(!isVisible(boundingRect, viewport, gis))
    {
        return;
    }


    foreach(const rtept_t &rtept, rte.pts)
    {
        QPointF pt(rtept.lon * DEG_TO_RAD, rtept.lat * DEG_TO_RAD);

        gis->convertRad2Px(pt);
        //pt = pt - rtept.focus;
        //p.drawPixmap(pt, rtept.icon);

        QRectF rect = fm.boundingRect(rtept.name);
        rect.adjust(-2,-2,2,2);

        // place label on top
        rect.moveCenter(pt + QPointF(rtept.icon.width()/2, -fm.height()));
        if(isBlocked(rect, blockedAreas))
        {
            // place label on bottom
            rect.moveCenter(pt + QPointF( rtept.icon.width()/2, +fm.height() + rtept.icon.height()));
            if(isBlocked(rect, blockedAreas))
            {
                // place label on right
                rect.moveCenter(pt + QPointF( rtept.icon.width() + rect.width()/2, +fm.height()));
                if(isBlocked(rect, blockedAreas))
                {
                    // place label on left
                    rect.moveCenter(pt + QPointF( -rect.width()/2, +fm.height()));
                    if(isBlocked(rect, blockedAreas))
                    {
                        // failed to place label anywhere
                        return;
                    }
                }
            }
        }

        CCanvas::drawText(rtept.name, p, rect.toRect(), Qt::darkBlue);
        blockedAreas << rect;
    }
}

void CGisItemRte::drawHighlight(QPainter& p)
{
    QMutexLocker lock(&mutexItems);

    if(line.isEmpty() || hasUserFocus())
    {
        return;
    }

    p.setPen(QPen(QColor(255,0,0,100),11,Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(line);
}

void CGisItemRte::readRouteDataFromGisLine(const SGisLine &l)
{
    bool doAutoRouting = !l.first().subpts.isEmpty();
    rte.pts.clear();

    for(int i = 0; i < l.size(); i++)
    {
        rte.pts << rtept_t();

        rtept_t& rtept      = rte.pts.last();
        const point_t& pt   = l[i];

        rtept.lon = pt.coord.x() * RAD_TO_DEG;
        rtept.lat = pt.coord.y() * RAD_TO_DEG;
        rtept.ele = pt.ele;
    }

    if(doAutoRouting)
    {
        calc();
    }
    deriveSecondaryData();
}

void CGisItemRte::setDataFromPolyline(const SGisLine &l)
{
    QMutexLocker lock(&mutexItems);

    mouseMoveFocus = 0;

    readRouteDataFromGisLine(l);

    flags |= eFlagTainted;
    changed(QObject::tr("Changed route points."), "://icons/48x48/LineMove.png");
}

void CGisItemRte::getPolylineFromData(SGisLine& l)
{
    QMutexLocker lock(&mutexItems);

    l.clear();
    foreach(const rtept_t &rtept, rte.pts)
    {
        l << point_t(QPointF(rtept.lon * DEG_TO_RAD, rtept.lat * DEG_TO_RAD));

        point_t& pt = l.last();

        pt.subpts.clear();
        foreach(const subpt_t &subpt, rtept.subpts)
        {
            pt.subpts << IGisLine::subpt_t(QPointF(subpt.lon * DEG_TO_RAD, subpt.lat * DEG_TO_RAD));
        }
    }
}

void CGisItemRte::calc()
{
    QMutexLocker lock(&mutexItems);

    mouseMoveFocus = 0;
    for(int i = 0; i < rte.pts.size(); i++)
    {
        rte.pts[i].subpts.clear();
    }
    CRouterSetup::self().calcRoute(getKey());
}

void CGisItemRte::reset()
{
    QMutexLocker lock(&mutexItems);

    for(int i = 0; i < rte.pts.size(); i++)
    {
        rtept_t& pt = rte.pts[i];
        pt.subpts.clear();
        pt.fakeSubpt = subpt_t();
    }

    mouseMoveFocus  = 0;
    totalDays       = NOINT;
    totalDistance   = NOFLOAT;
    totalTime       = QTime();
    lastRoutedTime  = QDateTime();
    lastRoutedWith  = "";

    deriveSecondaryData();
    updateHistory();

    if(key == keyUserFocus)
    {
        gainUserFocus(false);
    }
}


QPointF CGisItemRte::setMouseFocusByPoint(const QPoint& pt, focusmode_e fmode, const QString &owner)
{
    QMutexLocker lock(&mutexItems);

    const subpt_t * newPointOfFocus = 0;
    quint32 idx = 0;

    if(pt != NOPOINT && GPS_Math_DistPointPolyline(line, pt) < MIN_DIST_FOCUS)
    {
        quint32 i = 0;
        qint32 d1 = NOINT;

        foreach(const QPointF &point, line)
        {
            int tmp = (pt - point).manhattanLength();
            if(tmp <= d1)
            {
                idx = i;
                d1  = tmp;
            }
            i++;
        }

        newPointOfFocus = getSubPtByIndex(idx);
    }

    if(newPointOfFocus && (newPointOfFocus->type == subpt_t::eTypeNone))
    {
        newPointOfFocus = 0;
    }


    mouseMoveFocus = newPointOfFocus;

    return newPointOfFocus ? ((int)idx < line.size() ? line[idx] : NOPOINTF) : NOPOINTF;
}

const CGisItemRte::subpt_t * CGisItemRte::getSubPtByIndex(quint32 idx)
{
    quint32 cnt = 0;
    foreach(const rtept_t &rtept, rte.pts)
    {
        if(cnt == idx)
        {
            return &rtept.fakeSubpt;
        }

        foreach(const subpt_t &subpt, rtept.subpts)
        {
            cnt++;
            if(cnt == idx)
            {
                return &subpt;
            }
        }
        cnt++;
    }

    return 0;
}

void CGisItemRte::setResult(Routino_Output * route, const QString& options)
{
    QMutexLocker lock(&mutexItems);

    reset();

    qint32 idxRtept = -1;
    rtept_t * rtept = 0;

    Routino_Output * next = route;
    while(next)
    {
        if(next->type == ROUTINO_POINT_WAYPOINT)
        {
            idxRtept++;
            rtept = &rte.pts[idxRtept];
            rtept->subpts.clear();
            rtept->fakeSubpt.lon       = next->lon * RAD_TO_DEG;
            rtept->fakeSubpt.lat       = next->lat * RAD_TO_DEG;

            rtept->fakeSubpt.turn      = next->turn;
            rtept->fakeSubpt.bearing   = next->bearing;
            rtept->fakeSubpt.distance  = next->dist;
            rtept->fakeSubpt.time      = QTime(0,0).addSecs(next->time/10);
            rtept->fakeSubpt.type      = subpt_t::eTypeWpt;
        }
        else if(rtept != 0)
        {
            rtept->subpts << subpt_t();
            subpt_t& subpt  = rtept->subpts.last();
            subpt.lon       = next->lon * RAD_TO_DEG;
            subpt.lat       = next->lat * RAD_TO_DEG;

            subpt.turn      = next->turn;
            subpt.bearing   = next->bearing;
            subpt.distance  = next->dist;
            subpt.time      = subpt.time.addSecs(next->time/10);

            if(next->string != 0)
            {
                subpt.streets << next->string;
            }

            if(next->type > ROUTINO_POINT_CHANGE)
            {
                subpt.type = subpt_t::eTypeJunct;
            }
            else
            {
                subpt.type = subpt_t::eTypeNone;
            }

            totalDistance = subpt.distance;
            totalTime     = subpt.time;
            totalDays     = qFloor(next->time/864000);
        }

        next = next->next;
    }

    lastRoutedTime = QDateTime::currentDateTimeUtc();
    lastRoutedWith = "Routino, " + options;

    deriveSecondaryData();
    updateHistory();
}

struct maneuver_t
{
    QStringList streets;
    QString instruction;
    quint32 time;
    qreal dist;
    qint32 bearing;
    qint32 turn;
};

static const qint32 idx2bearing[] =
{
    NOINT
    , 0
    , -45
    , 45
    , 180
    , 135
    , -135
    , -90
    , 90
};


void CGisItemRte::setResult(const QDomDocument& xml, const QString &options)
{
    QMutexLocker lock(&mutexItems);

    reset();

    QDomElement response    = xml.firstChildElement("response");
    QDomElement route       = response.firstChildElement("route");

    // get time of travel
    QDomElement xmlTime     = route.firstChildElement("time");
    totalTime = QTime(0,0).addSecs(xmlTime.text().toUInt());


    // build list of maneuvers
    QDomNodeList xmlLegs       = route.firstChildElement("legs").elementsByTagName("leg");
    const qint32 L = xmlLegs.size();

    QList<maneuver_t> maneuvers;

    for(int l = 0; l < L; l++)
    {
        QDomNode xmlLeg = xmlLegs.item(l);
        QDomNodeList xmlManeuvers = xmlLeg.firstChildElement("maneuvers").elementsByTagName("maneuver");
        const qint32 M = xmlManeuvers.size();
        for(int m = 0; m < M; m++)
        {
            maneuvers << maneuver_t();
            maneuver_t& maneuver    = maneuvers.last();
            QDomNode xmlManeuver    = xmlManeuvers.item(m);
            maneuver.instruction    = xmlManeuver.firstChildElement("narrative").text();
            maneuver.time           = xmlManeuver.firstChildElement("time").text().toUInt();
            maneuver.dist           = xmlManeuver.firstChildElement("distance").text().toFloat();

            maneuver.bearing        = idx2bearing[xmlManeuver.firstChildElement("direction").text().toUInt()];
            maneuver.turn           = xmlManeuver.firstChildElement("turnType").text().toInt();

            QDomNodeList xmlStreets = xmlManeuver.toElement().elementsByTagName("streets");
            const int S = xmlStreets.size();
            for(int s = 0; s < S; s++)
            {
                QDomNode xmlStreet = xmlStreets.item(s);
                maneuver.streets << xmlStreet.toElement().text();
            }
        }
    }

    QVector<subpt_t> shape;

    // read the shape
    QDomElement xmlShape        = route.firstChildElement("shape");
    QDomElement xmlShapePoints  = xmlShape.firstChildElement("shapePoints");
    QDomNodeList xmlLatLng      = xmlShapePoints.elementsByTagName("latLng");
    const qint32 N = xmlLatLng.size();
    for(int n = 0; n < N; n++)
    {
        QDomNode elem   = xmlLatLng.item(n);
        QDomElement lat = elem.firstChildElement("lat");
        QDomElement lng = elem.firstChildElement("lng");

        shape << subpt_t();
        subpt_t& subpt = shape.last();
        subpt.lon = lng.text().toFloat();
        subpt.lat = lat.text().toFloat();
    }


    QVector<quint32> idxLegs;
    QDomElement xmlLegIndexes = xmlShape.firstChildElement("legIndexes");
    QDomNodeList xmlIndex     = xmlLegIndexes.elementsByTagName("index");
    const qint32 I = xmlIndex.size();
    for(int i = 0; i < I; i++)
    {
        QDomNode elem = xmlIndex.item(i);
        idxLegs << elem.toElement().text().toUInt();
    }

    quint32 time = 0;
    quint32 dist = 0;
    QDomElement xmlManeuverIndexes = xmlShape.firstChildElement("maneuverIndexes");
    xmlIndex                       = xmlManeuverIndexes.elementsByTagName("index");
    qint32 M = xmlIndex.size();
    for(int m = 0; m < M; m++)
    {
        QDomNode elem           = xmlIndex.item(m);
        quint32 idx             = elem.toElement().text().toUInt();
        subpt_t& subpt          = shape[idx];
        maneuver_t& maneuver    = maneuvers[m];
        subpt.type              = subpt_t::eTypeJunct;
        subpt.instruction       = maneuver.instruction;

        subpt.time              = QTime(0,0).addSecs(time);
        time += maneuver.time;

        subpt.distance          = dist;
        dist += maneuver.dist * 1000;

        subpt.bearing           = maneuver.bearing;
        subpt.turn              = maneuver.turn;

        subpt.streets           = maneuver.streets;
    }

    for(int i = 0; i < rte.pts.size() - 1; i++ )
    {
        quint32 idx1 = idxLegs[i];
        quint32 idx2 = idxLegs[i+1];

        rtept_t& rtept      = rte.pts[i];
        rtept.subpts        = shape.mid(idx1, idx2 - idx1 + 1);
        rtept.fakeSubpt.lon = rtept.lon;
        rtept.fakeSubpt.lat = rtept.lat;
    }

    rtept_t& rtept = rte.pts.last();
    rtept.fakeSubpt.lon = rtept.lon;
    rtept.fakeSubpt.lat = rtept.lat;

    totalDistance  = dist;
    lastRoutedTime = QDateTime::currentDateTimeUtc();
    lastRoutedWith = "MapQuest" + options;

    deriveSecondaryData();
    updateHistory();
}


