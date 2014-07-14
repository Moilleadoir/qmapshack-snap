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

#include "gis/IGisItem.h"
#include "units/IUnit.h"

#include <QtXml>

IGisItem::IGisItem(QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent)
{

}

IGisItem::~IGisItem()
{

}

inline void readXml(const QDomNode& xml, const QString& tag, qint32& value)
{
    if(xml.namedItem(tag).isElement())
    {
        value = xml.namedItem(tag).toElement().text().toInt();
    }
}

inline void readXml(const QDomNode& xml, const QString& tag, qreal& value)
{
    if(xml.namedItem(tag).isElement())
    {
        value = xml.namedItem(tag).toElement().text().toDouble();
    }
}

inline void readXml(const QDomNode& xml, const QString& tag, QString& value)
{
    if(xml.namedItem(tag).isElement())
    {
        value = xml.namedItem(tag).toElement().text();
    }
}

inline void readXml(const QDomNode& xml, const QString& tag, QDateTime& value)
{
    if(xml.namedItem(tag).isElement())
    {
        QString time = xml.namedItem(tag).toElement().text();
        IUnit::parseTimestamp(time, value);

    }
}

void IGisItem::readWpt(const QDomNode& xml, wpt_t& wpt)
{
    const QDomNamedNodeMap& attr = xml.attributes();
    wpt.lon = attr.namedItem("lon").nodeValue().toDouble();
    wpt.lat = attr.namedItem("lat").nodeValue().toDouble();

    readXml(xml, "ele", wpt.ele);
    readXml(xml, "time", wpt.time);
    readXml(xml, "magvar", wpt.magvar);
    readXml(xml, "geoidheight", wpt.geoidheight);
    readXml(xml, "name", wpt.name);
    readXml(xml, "cmt", wpt.cmt);
    readXml(xml, "desc", wpt.desc);
    readXml(xml, "src", wpt.src);

    if(xml.namedItem("link").isElement())
    {
        const QDomNodeList& links = xml.toElement().elementsByTagName("link");
        int N = links.count();
        for(int n = 0; n < N; ++n)
        {
            const QDomNode& link = links.item(n);

            link_t tmp;
            tmp.uri.setUrl(link.attributes().namedItem("href").nodeValue());
            readXml(link, "text", tmp.text);
            readXml(link, "type", tmp.type);

            wpt.links << tmp;
        }
    }

    readXml(xml, "sym", wpt.sym);
    readXml(xml, "type", wpt.type);
    readXml(xml, "fix", wpt.fix);
    readXml(xml, "sat", wpt.sat);
    readXml(xml, "hdop", wpt.hdop);
    readXml(xml, "vdop", wpt.vdop);
    readXml(xml, "pdop", wpt.pdop);
    readXml(xml, "ageofdgpsdata", wpt.ageofdgpsdata);
    readXml(xml, "dgpsid", wpt.dgpsid);

}

void IGisItem::genKey()
{
    if(key.isEmpty())
    {
        QCryptographicHash md5(QCryptographicHash::Md5);

        QByteArray tmp((const char *)this, sizeof(*this));
        md5.addData(tmp);
        key = md5.result().toHex();
    }
}

const QString& IGisItem::getKey()
{
    if(key.isEmpty())
    {
        genKey();
    }
    return key;


}
