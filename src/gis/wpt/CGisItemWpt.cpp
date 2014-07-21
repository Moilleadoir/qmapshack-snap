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

#include "gis/wpt/CGisItemWpt.h"
#include "gis/CGisProject.h"
#include "gis/CGisDraw.h"
#include "gis/WptIcons.h"
#include "canvas/CCanvas.h"
#include "GeoMath.h"


#include <QtWidgets>
#include <QtXml>



CGisItemWpt::CGisItemWpt(const QDomNode &xml, CGisProject *parent)
    : IGisItem(parent)
    , proximity(NOFLOAT)
{
    // --- start read and process data ----
    readWpt(xml, wpt);
    // decode some well known extensions
    if(xml.namedItem("extensions").isElement())
    {        
        const QDomNode& ext = xml.namedItem("extensions");
        readXml(ext, "ql:key", key);

        const QDomNode& wptx1 = ext.namedItem("wptx1:WaypointExtension");
        readXml(wptx1, "wptx1:Proximity", proximity);

        const QDomNode& xmlCache = ext.namedItem("cache");
        if(!xmlCache.isNull())
        {
            // read OC cache extensions
        }
    }

    const QDomNode& xmlCache = xml.namedItem("groundspeak:cache");
    if(!xmlCache.isNull() && !geocache.hasData)
    {
        readGcExt(xmlCache);
    }
    // --- stop read and process data ----

    setText(0, wpt.name);
    setIcon();
    genKey();
}

CGisItemWpt::~CGisItemWpt()
{

}

void CGisItemWpt::genKey()
{
    if(key.isEmpty())
    {
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData((const char*)&wpt, sizeof(wpt));
        key = md5.result().toHex();
    }
}

void CGisItemWpt::setIcon()
{
    if(geocache.hasData)
    {
        icon = getWptIconByName(geocache.type, focus);
    }
    else
    {
        icon = getWptIconByName(wpt.sym, focus);
    }

    QTreeWidgetItem::setIcon(0,icon);
}

void CGisItemWpt::save(QDomNode& gpx)
{
    QDomDocument doc = gpx.ownerDocument();

    QDomElement xmlWpt = doc.createElement("wpt");
    gpx.appendChild(xmlWpt);
    writeWpt(xmlWpt, wpt);

    // write the key as extension tag
    QDomElement xmlExt  = doc.createElement("extensions");
    xmlWpt.appendChild(xmlExt);
    writeXml(xmlExt, "ql:key", key);

    // write other well known extensions
    QDomElement wptx1  = doc.createElement("wptx1:WaypointExtension");
    xmlExt.appendChild(wptx1);
    writeXml(wptx1, "wptx1:Proximity", proximity);

    if(geocache.hasData && geocache.service == eGC)
    {
        QDomElement xmlCache = doc.createElement("groundspeak:cache");
        writeGcExt(xmlCache);
        xmlWpt.appendChild(xmlCache);
    }
}

void CGisItemWpt::readGcExt(const QDomNode& xmlCache)
{
    geocache.service = eGC;
    const QDomNamedNodeMap& attr = xmlCache.attributes();
    geocache.id = attr.namedItem("id").nodeValue().toInt();

    geocache.archived   = attr.namedItem("archived").nodeValue().toLocal8Bit() == "True";
    geocache.available  = attr.namedItem("available").nodeValue().toLocal8Bit() == "True";
    if(geocache.archived)
    {
        geocache.status = QObject::tr("Archived");
    }
    else if(geocache.available)
    {
        geocache.status = QObject::tr("Available");
    }
    else
    {
        geocache.status = QObject::tr("Not Available");
    }

    readXml(xmlCache, "groundspeak:name", geocache.name);
    readXml(xmlCache, "groundspeak:placed_by", geocache.owner);
    readXml(xmlCache, "groundspeak:type", geocache.type);
    readXml(xmlCache, "groundspeak:container", geocache.container);
    readXml(xmlCache, "groundspeak:difficulty", geocache.difficulty);
    readXml(xmlCache, "groundspeak:terrain", geocache.terrain);
    readXml(xmlCache, "groundspeak:short_description", geocache.shortDesc);
    readXml(xmlCache, "groundspeak:long_description", geocache.longDesc);
    readXml(xmlCache, "groundspeak:encoded_hints", geocache.hint);
    readXml(xmlCache, "groundspeak:country", geocache.country);
    readXml(xmlCache, "groundspeak:state", geocache.state);

    const QDomNodeList& logs = xmlCache.toElement().elementsByTagName("groundspeak:log");
    uint N = logs.count();

    for(uint n = 0; n < N; ++n)
    {
        const QDomNode& xmlLog = logs.item(n);
        const QDomNamedNodeMap& attr = xmlLog.attributes();

        geocachelog_t log;
        log.id = attr.namedItem("id").nodeValue().toUInt();
        readXml(xmlLog, "groundspeak:date", log.date);
        readXml(xmlLog, "groundspeak:type", log.type);
        if(xmlLog.namedItem("groundspeak:finder").isElement())
        {
            const QDomNamedNodeMap& attr = xmlLog.namedItem("groundspeak:finder").attributes();
            log.finderId = attr.namedItem("id").nodeValue();
        }

        readXml(xmlLog, "groundspeak:finder", log.finder);
        readXml(xmlLog, "groundspeak:text", log.text);

        geocache.logs << log;

    }
    geocache.hasData = true;
}

void CGisItemWpt::writeGcExt(QDomNode& xmlCache)
{
    QString str;
    xmlCache.toElement().setAttribute("xmlns:groundspeak", "http://www.groundspeak.com/cache/1/0");
    xmlCache.toElement().setAttribute("id", geocache.id);
    xmlCache.toElement().setAttribute("archived", geocache.archived ? "True" : "False");
    xmlCache.toElement().setAttribute("available", geocache.available ? "True" : "False");

    writeXml(xmlCache, "groundspeak:name", geocache.name);
    writeXml(xmlCache, "groundspeak:placed_by", geocache.owner);
    writeXml(xmlCache, "groundspeak:type", geocache.type);
    writeXml(xmlCache, "groundspeak:container", geocache.container);

    if(geocache.difficulty == int(geocache.difficulty))
    {
        str.sprintf("%1.0f", geocache.difficulty);
    }
    else
    {
        str.sprintf("%1.1f", geocache.difficulty);
    }
    writeXml(xmlCache, "groundspeak:difficulty", str);

    if(geocache.terrain == int(geocache.terrain))
    {
        str.sprintf("%1.0f", geocache.terrain);
    }
    else
    {
        str.sprintf("%1.1f", geocache.terrain);
    }
    writeXml(xmlCache, "groundspeak:terrain", str);
    writeXmlHtml(xmlCache, "groundspeak:short_description", geocache.shortDesc);
    writeXmlHtml(xmlCache, "groundspeak:long_description", geocache.longDesc);
    writeXml(xmlCache, "groundspeak:encoded_hints", geocache.hint);

    if(!geocache.logs.isEmpty())
    {
        QDomElement xmlLogs = xmlCache.ownerDocument().createElement("groundspeak:logs");
        xmlCache.appendChild(xmlLogs);

        foreach(const geocachelog_t& log, geocache.logs)
        {
            QDomElement xmlLog = xmlCache.ownerDocument().createElement("groundspeak:log");
            xmlLogs.appendChild(xmlLog);

            xmlLog.setAttribute("id", log.id);
            writeXml(xmlLog, "groundspeak:date", log.date);
            writeXml(xmlLog, "groundspeak:type", log.type);

            QDomElement xmlFinder = xmlCache.ownerDocument().createElement("groundspeak:finder");
            xmlLog.appendChild(xmlFinder);

            QDomText _finder_ = xmlCache.ownerDocument().createCDATASection(log.finder);
            xmlFinder.appendChild(_finder_);
            xmlFinder.setAttribute("id", log.finderId);

            writeXmlHtml(xmlLog, "groundspeak:text", log.text);
        }
    }
}

void CGisItemWpt::drawItem(QPainter& p, const QRectF& viewport, QList<QRectF> &blockedAreas, CGisDraw *gis)
{
    pos = QPointF(wpt.lon * DEG_TO_RAD, wpt.lat * DEG_TO_RAD);
    if(!viewport.contains(pos))
    {
        pos = QPoint();
        return;
    }
    gis->convertRad2Px(pos);
    p.drawPixmap(pos - focus, icon);

    if(proximity != NOFLOAT)
    {
        QPointF pt1(wpt.lon * DEG_TO_RAD, wpt.lat * DEG_TO_RAD);
        pt1 = GPS_Math_Wpt_Projection(pt1, proximity, 90 * DEG_TO_RAD);
        gis->convertRad2Px(pt1);

        double r = pt1.x() - pos.x();

        p.save();
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::white,3));
        p.drawEllipse(QRect(pos.x() - r - 1, pos.y() - r - 1, 2*r + 1, 2*r + 1));
        p.setPen(QPen(Qt::red,1));
        p.drawEllipse(QRect(pos.x() - r - 1, pos.y() - r - 1, 2*r + 1, 2*r + 1));
        p.restore();
    }

    blockedAreas << QRectF(pos - focus, icon.size());
}

void CGisItemWpt::drawLabel(QPainter& p, const QRectF& viewport, QList<QRectF> &blockedAreas, const QFontMetricsF &fm, CGisDraw *gis)
{
    if(pos.isNull())
    {
        return;
    }

    QPointF pt = pos - focus;

    QRectF rect = fm.boundingRect(wpt.name);
    rect.adjust(-2,-2,2,2);

    // place label on top
    rect.moveCenter(pt + QPointF(icon.width()/2, - fm.height()));
    if(isBlocked(rect, blockedAreas))
    {
        // place label on bottom
        rect.moveCenter(pt + QPointF( icon.width()/2, + fm.height() + icon.height()));
        if(isBlocked(rect, blockedAreas))
        {
            // place label on right
            rect.moveCenter(pt + QPointF( icon.width() + rect.width()/2, +fm.height()));
            if(isBlocked(rect, blockedAreas))
            {
                // place label on left
                rect.moveCenter(pt + QPointF( - rect.width()/2, +fm.height()));
                if(isBlocked(rect, blockedAreas))
                {
                    // failed to place label anywhere
                    return;
                }
            }
        }
    }

    CCanvas::drawText(wpt.name,p,rect.toRect(), Qt::darkBlue);
    blockedAreas << rect;
}

