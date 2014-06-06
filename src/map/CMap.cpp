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

#include "map/CMap.h"
#include "map/CMapItem.h"
#include "map/CMapList.h"
#include "CCanvas.h"
#include "CMainWindow.h"

#include <QtGui>
#include <QtWidgets>

//#define N_ZOOM_LEVELS 20
//const qreal CMap::scales[N_ZOOM_LEVELS] =
//{
//     0.10
//    ,0.20
//    ,0.40
//    ,0.80
//    ,1.60
//    ,3.20
//    ,6.40
//    ,12.8
//    ,25.6
//    ,51.2
//    ,102.4
//    ,204.8
//    ,409.6
//    ,819.2
//    ,1638.4
//    ,3276.8
//    ,6553.6
//    ,13107.2
//    ,26214.4
//    ,52428.8
//};

#define N_ZOOM_LEVELS 36
const qreal CMap::scales[N_ZOOM_LEVELS] =
{
      0.10
    , 0.15
    , 0.20
    , 0.30
    , 0.50
    , 0.70
    , 1.0
    , 1.5
    , 2.0
    , 3.0
    , 5.0
    , 7.0
    , 10.0
    , 15.0
    , 20.0
    , 30.0
    , 50.0
    , 70.0
    , 100.0
    , 150.0
    , 200.0
    , 300.0
    , 500.0
    , 700.0
    , 1000.0
    , 1500.0
    , 2000.0
    , 3000.0
    , 5000.0
    , 7000.0
    , 10000.0
    , 15000.0
    , 20000.0
    , 30000.0
    , 50000.0
    , 70000.0
};



CMap::CMap(CCanvas *parent)
    : QThread(parent)
    , canvas(parent)
    , bufIndex(false)
    , bufWidth(100)
    , bufHeight(100)
    , viewWidth(100)
    , viewHeight(100)
    , scale(1.0,-1.0)
    , zoomIndex(0)
{
    // setup map parameters and connect to canvas
    pjsrc = pj_init_plus("+proj=merc +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs");
    pjtar = pj_init_plus("+proj=longlat +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs");

    zoom(5);

    mapList = new CMapList(canvas);
    CMainWindow::self().addMapList(mapList, canvas->objectName());

    resize(canvas->size());
    connect(this, SIGNAL(finished()), canvas, SLOT(update()));

    buildMapList();

}

CMap::~CMap()
{
    pj_free(pjtar);
    pj_free(pjsrc);

    CMainWindow::self().delMapList(mapList);
}

void CMap::emitSigCanvasUpdate()
{
    emit sigCanvasUpdate();
}

QString CMap::getProjection()
{
    if(pjsrc == 0)
    {
        return QString::Null();
    }
    return pj_get_def(pjsrc,0);
}

void CMap::saveConfig(QSettings& cfg)
{
    saveActiveMapsList(cfg);
    cfg.setValue("map/zoomIndex", zoomIndex);

}

void CMap::loadConfig(QSettings& cfg)
{
    restoreActiveMapsList(cfg);
    int idx = cfg.value("map/zoomIndex",zoomIndex).toInt();
    zoom(idx);
}

void CMap::buildMapList()
{
    QDir pathMaps("./");

    QCryptographicHash md5(QCryptographicHash::Md5);
    QStringList filters;
    filters << "*rmap" << "*jnx" << "*img" << "*vrt" << "*map";

    QMutexLocker lock(&CMapItem::mutexActiveMaps);
    mapList->clear();
    // find available maps
    foreach(const QString& filename, pathMaps.entryList(filters, QDir::Files|QDir::Readable, QDir::Name))
    {
        QFileInfo fi(filename);

        CMapItem * item = new CMapItem(*mapList, this);

        item->setText(fi.baseName());
        item->filenames  << pathMaps.absoluteFilePath(filename);
        item->setSizeHint(QSize(0,32));
        item->updateIcon();

        QFile f(pathMaps.absoluteFilePath(filename));
        f.open(QIODevice::ReadOnly);
        md5.reset();
        md5.addData(f.read(1024));
        item->key = md5.result().toHex();
        f.close();
    }
}

void CMap::saveActiveMapsList(QSettings &cfg)
{
    QStringList keys;
    QMutexLocker lock(&CMapItem::mutexActiveMaps);

    for(int i = 0; i < mapList->count(); i++)
    {
        CMapItem * item = mapList->item(i);
        if(item && !item->files.isEmpty())
        {
            keys << item->key;
        }
    }

    cfg.setValue("map/active", keys);

}

void CMap::restoreActiveMapsList(QSettings &cfg)
{
    QStringList keys = cfg.value("map/active", "").toStringList();
    QMutexLocker lock(&CMapItem::mutexActiveMaps);

    foreach(const QString& key, keys)
    {
        for(int i = 0; i < mapList->count(); i++)
        {
            CMapItem * item = mapList->item(i);

            if(item && item->key == key)
            {
                item->activate();
                break;
            }
        }
    }
}

void CMap::resize(const QSize& size)
{
    if(isRunning())
    {
        wait();
    }
    mutex.lock(); // --------- start serialize with thread
    viewWidth   = size.width();
    viewHeight  = size.height();
    center      = QPointF(viewWidth/2.0, viewHeight/2.0);
    int a       = sqrt(viewWidth*viewWidth + viewHeight*viewHeight);
//    bufWidth    = a + 100;
//    bufHeight   = a + 100;
    bufWidth    = viewWidth  + 100;
    bufHeight   = viewHeight + 100;

    buffer[0].image = QImage(bufWidth, bufHeight, QImage::Format_ARGB32);
    buffer[1].image = QImage(bufWidth, bufHeight, QImage::Format_ARGB32);
    mutex.unlock(); // --------- stop serialize with thread
}

bool CMap::needsRedraw()
{
    bool res = false;
    mutex.lock();
    res = intNeedsRedraw;
    mutex.unlock();
    return res;
}

void CMap::zoom(bool in, bool &needsRedraw)
{
    if(pjsrc == 0) return;
    zoom(zoomIndex + (in ? -1 : 1));
    needsRedraw = true;
}

void CMap::zoom(int idx)
{
    if(idx < 0)
    {
        idx = 0;
    }

    if(idx == N_ZOOM_LEVELS)
    {
        idx = N_ZOOM_LEVELS -1;
    }

    mutex.lock(); // --------- start serialize with thread
    if(zoomIndex != idx)
    {
        zoomIndex       = idx;
        zoomFactor.rx() = scales[idx];
        zoomFactor.ry() = scales[idx];
        intNeedsRedraw  = true;
    }
    mutex.unlock(); // --------- stop serialize with thread
}

void CMap::convertRad2M(QPointF &p)
{
    if(pjsrc == 0)
    {
        return;
    }
    pj_transform(pjtar,pjsrc,1,0,&p.rx(),&p.ry(),0);
}

void CMap::convertM2Rad(QPointF &p)
{
    if(pjsrc == 0)
    {
        return;
    }
    pj_transform(pjsrc,pjtar,1,0,&p.rx(),&p.ry(),0);
}

void CMap::convertPx2Rad(QPointF &p)
{
    mutex.lock(); // --------- start serialize with thread

    QPointF f = focus;
    convertRad2M(f);

    p = f + (p - center) * scale * zoomFactor;

    convertM2Rad(p);

    mutex.unlock(); // --------- stop serialize with thread
}

void CMap::convertRad2Px(QPointF &p)
{
    mutex.lock(); // --------- start serialize with thread

    QPointF f = focus;
    convertRad2M(f);
    convertRad2M(p);

    p = (p - f) / (scale * zoomFactor) + center;

    mutex.unlock(); // --------- stop serialize with thread
}

void CMap::draw(QPainter& p, bool needsRedraw, const QPointF& f, const QRectF &r)
{
    if(pjsrc == 0) return;

    // convert global coordinate of focus into point of map
    focus = f;

    QPointF f1 = focus;
    convertRad2M(f1);

    QPointF bufferScale = scale * zoomFactor;

    mutex.lock(); // --------- start serialize with thread
    // derive top left reference coordinate of map buffer
    ref1 = f1 + QPointF(-bufWidth/2, -bufHeight/2) * bufferScale;
    convertM2Rad(ref1);
    // derive top right reference coordinate of map buffer
    ref2 = f1 + QPointF( bufWidth/2, -bufHeight/2) * bufferScale;
    convertM2Rad(ref2);
    // derive bottom right reference coordinate of map buffer
    ref3 = f1 + QPointF( bufWidth/2,  bufHeight/2) * bufferScale;
    convertM2Rad(ref3);
    // derive bottom left reference coordinate of map buffer
    ref4 = f1 + QPointF(-bufWidth/2,  bufHeight/2) * bufferScale;
    convertM2Rad(ref4);

    // get current active buffer
    IMap::buffer_t& currentBuffer = buffer[bufIndex];

    // convert buffers top left reference point to local coordinate system
    QPointF ref = currentBuffer.ref1;
    convertRad2M(ref);

    // derive offset to show coordinate of focus right in the middle of the draw
    // context. NOTE: the draw context's coordinate system has been moved into the
    // middle of the view port.
    QPointF off = (ref - f1) / (currentBuffer.scale * currentBuffer.zoomFactor);

    p.save();
    // scale image if current zoomfactor does not match buffer's zoomfactor
    p.scale(currentBuffer.zoomFactor.x()/zoomFactor.x(), currentBuffer.zoomFactor.y()/zoomFactor.y());
    // add offset
    p.translate(off);
    // draw buffer to painter
    p.drawImage(0,0, currentBuffer.image);
    p.restore();

    // intNeedsRedraw is reset by the thread
    if(needsRedraw) intNeedsRedraw = true;
    mutex.unlock(); // --------- stop serialize with thread

    if(needsRedraw && !isRunning())
    {
        start();
    }

}
void CMap::run()
{
    mutex.lock();
    QTime t;
    t.start();
    qDebug() << "start thread";

    IMap::buffer_t& currentBuffer = buffer[!bufIndex];
    while(intNeedsRedraw)
    {
        // copy all projection information need by the
        // map render objects to buffer structure
        currentBuffer.pjsrc         = pjsrc;
        currentBuffer.zoomFactor    = zoomFactor;
        currentBuffer.scale         = scale;
        currentBuffer.ref1          = ref1;
        currentBuffer.ref2          = ref2;
        currentBuffer.ref3          = ref3;
        currentBuffer.ref4          = ref4;
        currentBuffer.focus         = focus;
        intNeedsRedraw              = false;
        mutex.unlock();

        // ----- reset buffer -----
        currentBuffer.image.fill(Qt::transparent);

        // iterate over all active maps and call the draw method
        CMapItem::mutexActiveMaps.lock();
        if(mapList)
        {
            for(int i = 0; i < mapList->count(); i++)
            {
                CMapItem * item = mapList->item(i);

                if(!item || item->files.isEmpty())
                {
                    // as all active maps have to be at the top of the list
                    // it is ok to break ass soon as the first map with no
                    // active files is hit.
                    break;
                }

                foreach(IMap * m, item->files)
                {
                    m->draw(currentBuffer);
                }
            }
        }
        CMapItem::mutexActiveMaps.unlock();
        mutex.lock();
    }
    // ----- switch buffer ------
    bufIndex = !bufIndex;
    qDebug() << "stop thread" << t.elapsed();
    mutex.unlock();

}

