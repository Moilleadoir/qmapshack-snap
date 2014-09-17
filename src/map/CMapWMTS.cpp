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

#include "map/CMapWMTS.h"
#include "map/CMapDraw.h"
#include "map/wmts/CDiskCache.h"
#include "units/IUnit.h"


#include <QtWidgets>
#include <QtXml>
#include <QtNetwork>

#include <ogr_spatialref.h>
#include <proj_api.h>

CMapWMTS::CMapWMTS(const QString &filename, CMapDraw *parent)
    : IMap(eFeatVisibility|eFeatTileCache, parent)
    , diskCache(0)
    , lastRequest(false)

{
    qDebug() << "------------------------------";
    qDebug() << "WTMS: try to open" << filename;

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(0, tr("Error..."), tr("Failed to open %1").arg(filename), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    QString msg;
    int line, column;
    QDomDocument dom;
    if(!dom.setContent(&file, true, &msg, &line, &column))
    {
        file.close();
        QMessageBox::critical(0, tr("Error..."), tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    file.close();

    const QDomElement& xmlCapabilities = dom.documentElement();
    if(xmlCapabilities.tagName() != "Capabilities")
    {
        QMessageBox::critical(0, tr("Error..."), tr("Failed to read: %1\nUnknown structure.").arg(filename), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    const QDomNode& xmlServiceIdentification = xmlCapabilities.namedItem("ServiceIdentification");
    QString ServiceType         = xmlServiceIdentification.firstChildElement("ServiceType").text();
    QString ServiceTypeVersion  = xmlServiceIdentification.firstChildElement("ServiceTypeVersion").text();

    if(!ServiceType.contains("WMTS", Qt::CaseInsensitive) || ServiceTypeVersion != "1.0.0")
    {
        QMessageBox::critical(0, tr("Error..."), tr("Unexpexted service. '* WMTS 1.0.0' is expected. '%1 %2' is read.").arg(ServiceType).arg(ServiceTypeVersion), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    const QDomNode& xmlContents = xmlCapabilities.namedItem("Contents");
    const QDomNodeList& xmlLayers = xmlContents.toElement().elementsByTagName("Layer");

    const int N = xmlLayers.count();
    for(int n = 0; n < N; n++)
    {
        QString str;
        QStringList values;
        const QDomNode& xmlLayer = xmlLayers.at(n);
        layer_t layer;

        layer.title = xmlLayer.firstChildElement("Title").text();

        // read bounding box
        const QDomNode& xmlBoundingBox = xmlLayer.firstChildElement("WGS84BoundingBox");
        str = xmlBoundingBox.namedItem("LowerCorner").toElement().text();
        values = str.split(" ");
        QPointF bottomLeft(values[0].toDouble(), values[1].toDouble());

        str = xmlBoundingBox.namedItem("UpperCorner").toElement().text();
        values = str.split(" ");
        QPointF topRight(values[0].toDouble(), values[1].toDouble());

        layer.boundingBox.setBottomLeft(bottomLeft);
        layer.boundingBox.setTopRight(topRight);

        const QDomNode& xmlStyle = xmlLayer.firstChildElement("Style");
        layer.styles << xmlStyle.namedItem("Identifier").toElement().text();

        const QDomNode& xmlTileMatrixSetLink = xmlLayer.firstChildElement("TileMatrixSetLink");
        layer.tileMatrixSet = xmlTileMatrixSetLink.namedItem("TileMatrixSet").toElement().text();

        const QDomNode& xmlTileMatrixSetLimits = xmlTileMatrixSetLink.firstChildElement("TileMatrixSetLimits");
        if(xmlTileMatrixSetLimits.isElement())
        {
            const QDomNodeList& xmlTileMatrixLimits = xmlTileMatrixSetLimits.toElement().elementsByTagName("TileMatrixLimits");
            const int L = xmlTileMatrixLimits.count();
            for(int l = 0; l < L; l++)
            {
                const QDomNode& xmlTileMatrixLimit = xmlTileMatrixLimits.at(l);
                QString Identifier          = xmlTileMatrixLimit.namedItem("TileMatrix").toElement().text();
                layer.limits[Identifier]    = limit_t();
                limit_t& limit              = layer.limits[Identifier];

                limit.minTileRow = xmlTileMatrixLimit.namedItem("MinTileRow").toElement().text().toInt();
                limit.maxTileRow = xmlTileMatrixLimit.namedItem("MaxTileRow").toElement().text().toInt();
                limit.minTileCol = xmlTileMatrixLimit.namedItem("MinTileCol").toElement().text().toInt();
                limit.maxTileCol = xmlTileMatrixLimit.namedItem("MaxTileCol").toElement().text().toInt();
            }
        }

        const QDomNode& xmlResourceURL = xmlLayer.firstChildElement("ResourceURL");
        const QDomNamedNodeMap& attr = xmlResourceURL.attributes();

        layer.resourceURL = attr.namedItem("template").nodeValue();
        layer.resourceURL = layer.resourceURL.replace("{style}",layer.styles[0], Qt::CaseInsensitive);
        layer.resourceURL = layer.resourceURL.replace("{TileMatrixSet}",layer.tileMatrixSet, Qt::CaseInsensitive);

        qDebug() << layer.resourceURL;
        layers << layer;
    }

    const QDomNodeList& xmlTileMatrixSets = xmlContents.childNodes();
    const int M = xmlTileMatrixSets.count();

    for(int m = 0; m < M; m++)
    {
        const QDomNode& xmlTileMatrixSet = xmlTileMatrixSets.at(m);
        if(xmlTileMatrixSet.nodeName() != "TileMatrixSet")
        {
            continue;
        }


        QString Identifier      = xmlTileMatrixSet.namedItem("Identifier").toElement().text();
        tilesets[Identifier]    = tileset_t();
        tileset_t& tileset      = tilesets[Identifier];

        QString str = xmlTileMatrixSet.namedItem("SupportedCRS").toElement().text();

        char * ptr = str.toLatin1().data();
        OGRSpatialReference oSRS;
        oSRS.importFromURN(ptr);
        oSRS.exportToProj4(&ptr);

        qDebug() << ptr;
        tileset.pjsrc = pj_init_plus(ptr);
        if(tileset.pjsrc == 0)
        {
            QMessageBox::warning(0, tr("Error..."), tr("No georeference information found."));
            return;
        }

        const QDomNodeList& xmlTileMatrixN = xmlTileMatrixSet.toElement().elementsByTagName("TileMatrix");
        const int N = xmlTileMatrixN.count();
        for(int n = 0; n < N; n++)
        {
            QString str;
            QStringList values;
            const QDomNode& xmlTileMatrix = xmlTileMatrixN.at(n);
            QString Identifier =  xmlTileMatrix.namedItem("Identifier").toElement().text();
            tileset.tilematrix[Identifier] = tilematrix_t();
            tilematrix_t& matrix = tileset.tilematrix[Identifier];

            str = xmlTileMatrix.namedItem("TopLeftCorner").toElement().text();
            values = str.split(" ");
            matrix.topLeft      = QPointF(values[0].toDouble(), values[1].toDouble());
            matrix.scale        = xmlTileMatrix.namedItem("ScaleDenominator").toElement().text().toDouble();
            matrix.tileWidth    = xmlTileMatrix.namedItem("TileWidth").toElement().text().toInt();
            matrix.tileHeight   = xmlTileMatrix.namedItem("TileHeight").toElement().text().toInt();
            matrix.matrixWidth  = xmlTileMatrix.namedItem("MatrixWidth").toElement().text().toInt();
            matrix.matrixHeight = xmlTileMatrix.namedItem("MatrixHeight").toElement().text().toInt();                        
        }

    }

    QFileInfo fi(filename);
    slotSetCachePath(QDir::home().absoluteFilePath(".QMapShack/" + fi.baseName()));

    accessManager   = new QNetworkAccessManager(parent->thread());

    connect(this, SIGNAL(sigQueueChanged()), this, SLOT(slotQueueChanged()));
    connect(accessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));

    isActivated = true;
}

CMapWMTS::~CMapWMTS()
{

}

void CMapWMTS::configureCache()
{
    delete diskCache;
    diskCache = new CDiskCache(getCachePath(), getCacheSize(), getCacheExpiration(), this);
}

void CMapWMTS::slotQueueChanged()
{
    if(!urlQueue.isEmpty() && urlPending.size() < 6)
    {        
        QMutexLocker lock(&mutex);

        // request up to 6 pending request
        for(int i = 0; i < (6 - urlPending.size()); i++)
        {
            QString url = urlQueue.dequeue();
            lastRequest = urlQueue.isEmpty();

            QNetworkRequest request;
            request.setUrl(url);
            accessManager->get(request);
            urlPending << url;

            if(lastRequest)
            {
                break;
            }
        }
    }
    else if(lastRequest && urlPending.isEmpty())
    {
        // if all tiles are received the map layer can be redrawn with all tiles from cache
        map->emitSigCanvasUpdate();
        lastRequest = false;
    }
}

void CMapWMTS::slotRequestFinished(QNetworkReply* reply)
{
    QString url = reply->url().toString();

    if(urlPending.contains(url))
    {
        QImage img;
        // only take good responses
        if(!reply->error())
        {
            // read image data
            img.loadFromData(reply->readAll());

        }
        // always store image to cache, the cache will take care of NULL images
        diskCache->store(url, img);

        urlPending.removeAll(url);
    }

    // debug output any error
    if(reply->error())
    {
        qDebug() << reply->errorString();
    }

    // delete reply object
    reply->deleteLater();

    // check for more items to be queued
    slotQueueChanged();
}

void CMapWMTS::draw(IDrawContext::buffer_t& buf)
{
    QMutexLocker lock(&mutex);

    urlQueue.clear();

    if(map->needsRedraw())
    {
        return;
    }

    // get pixel offset of top left buffer corner
    QPointF pp = buf.ref1;
    map->convertRad2Px(pp);

    // start to draw the map
    QPainter p(&buf.image);
    USE_ANTI_ALIASING(p,true);
    p.setOpacity(getOpacity()/100.0);
    p.translate(-pp);


    // calculate maximum viewport
    qreal x1 = buf.ref1.x() < buf.ref4.x() ? buf.ref1.x() : buf.ref4.x();
    qreal y1 = buf.ref1.y() > buf.ref2.y() ? buf.ref1.y() : buf.ref2.y();

    qreal x2 = buf.ref2.x() > buf.ref3.x() ? buf.ref2.x() : buf.ref3.x();
    qreal y2 = buf.ref3.y() < buf.ref4.y() ? buf.ref3.y() : buf.ref4.y();

    QRectF viewport(QPointF(x1,y1) * RAD_TO_DEG, QPointF(x2,y2) * RAD_TO_DEG);

    // draw layers
    foreach(const layer_t& layer, layers)
    {
        if(!layer.boundingBox.intersects(viewport))
        {
            continue;
        }

        const tileset_t& tileset            = tilesets[layer.tileMatrixSet];
        const QMap<QString,limit_t>& limits = layer.limits;

        // convert viewport to layer's coordinate system
        QPointF pt1(x1,y1);
        QPointF pt2(x2,y2);

        pj_transform(pjtar, tileset.pjsrc, 1, 0, &pt1.rx(), &pt1.ry(), 0);
        pj_transform(pjtar, tileset.pjsrc, 1, 0, &pt2.rx(), &pt2.ry(), 0);

        // search matrix ID of tile level with best matching scale
        QString tileMatrixId;
        QPointF s1 = (pt2 - pt1)/QPointF(buf.image.width(), buf.image.height());        
        qreal d = NOFLOAT;
        foreach(const QString& key, tileset.tilematrix.keys())
        {
            const tilematrix_t& tilematrix = tileset.tilematrix[key];
            qreal s2 = tilematrix.scale * 0.28e-3;


            if(qAbs(s2 - s1.x()) < d)
            {
                tileMatrixId = key;
                d = qAbs(s2 - s1.x());
            }
        }


        // get min/max col/row values for that level
        qint32 minRow, maxRow, minCol, maxCol;
        const tilematrix_t& tilematrix = tileset.tilematrix[tileMatrixId];
        if(!limits.isEmpty())
        {
            if(limits.contains(tileMatrixId))
            {
                const limit_t& limit = limits[tileMatrixId];
                minCol = limit.minTileCol;
                maxCol = limit.maxTileCol;
                minRow = limit.minTileRow;
                maxRow = limit.maxTileRow;
            }
            else
            {
                // layer has limits but not for the selected tileMatrixId -> skip layer
                continue;
            }
        }
        else
        {
            minCol = 0;
            maxCol = tilematrix.matrixWidth;
            minRow = 0;
            maxRow = tilematrix.matrixHeight;
        }


        // derive range of col/row to request tiles
        qreal xscale =  tilematrix.scale * 0.28e-3;
        qreal yscale = -tilematrix.scale * 0.28e-3;

        qint32 col1 = qFloor((pt1.x() - tilematrix.topLeft.x()) / ( xscale * tilematrix.tileWidth));
        qint32 row1 = qFloor((pt1.y() - tilematrix.topLeft.y()) / ( yscale * tilematrix.tileHeight));
        qint32 col2 = qCeil((pt2.x()  - tilematrix.topLeft.x()) / ( xscale * tilematrix.tileWidth));
        qint32 row2 = qCeil((pt2.y()  - tilematrix.topLeft.y()) / ( yscale * tilematrix.tileHeight));


        if(col1 < minCol) col1 = minCol;
        if(col1 > maxCol) col1 = maxCol;
        if(row1 < minRow) row1 = minRow;
        if(row1 > maxRow) row1 = maxRow;

        if(col2 < minCol) col2 = minCol;
        if(col2 > maxCol) col2 = maxCol;
        if(row2 < minRow) row2 = minRow;
        if(row2 > maxRow) row2 = maxRow;


        // start to request tiles. draw tiles in cache, queue urls of tile yet to be requested
        for(qint32 row = row1; row <= row2; row++)
        {
            for(qint32 col = col1; col <= col2; col++)
            {

                qreal xx1 =  col      * (xscale * tilematrix.tileWidth)  + tilematrix.topLeft.x();
                qreal yy1 =  row      * (yscale * tilematrix.tileHeight) + tilematrix.topLeft.y();
                qreal xx2 = (col + 1) * (xscale * tilematrix.tileWidth)  + tilematrix.topLeft.x();
                qreal yy2 = (row + 1) * (yscale * tilematrix.tileHeight) + tilematrix.topLeft.y();

                QString url = layer.resourceURL;
                url = url.replace("{TileMatrix}",tileMatrixId, Qt::CaseInsensitive);
                url = url.replace("{TileRow}",QString::number(row), Qt::CaseInsensitive);
                url = url.replace("{TileCol}",QString::number(col), Qt::CaseInsensitive);

                if(diskCache->contains(url))
                {
                    QImage img;
                    diskCache->restore(url, img);
                    QPolygonF l;
                    l << QPointF(xx1, yy1) << QPointF(xx2, yy1) << QPointF(xx2, yy2) << QPointF(xx1, yy2);
                    pj_transform(tileset.pjsrc,pjtar, 1, 0, &l[0].rx(), &l[0].ry(), 0);
                    pj_transform(tileset.pjsrc,pjtar, 1, 0, &l[1].rx(), &l[1].ry(), 0);
                    pj_transform(tileset.pjsrc,pjtar, 1, 0, &l[2].rx(), &l[2].ry(), 0);
                    pj_transform(tileset.pjsrc,pjtar, 1, 0, &l[3].rx(), &l[3].ry(), 0);

                    drawTile(img, l, p);
                }
                else
                {
                    urlQueue << url;
                }
            }
        }

        emit sigQueueChanged();
    }
}
