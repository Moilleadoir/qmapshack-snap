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

#ifndef IMAP_H
#define IMAP_H


#include <QObject>
#include <QImage>
#include <QMutex>
#include <proj_api.h>

class CMap;
class QSettings;

class IMap : public QObject
{
    Q_OBJECT
    public:
        IMap(CMap * parent);
        virtual ~IMap();

        virtual void saveConfig(QSettings& cfg);
        virtual void loadConfig(QSettings& cfg);

        struct buffer_t
        {
            buffer_t() : zoomFactor(1.0,1.0), scale(1.0,1.0){}

            /// @note: all coordinate values are long/lat WGS84 [rad]

            /// the canvas buffer
            QImage  image;
            /// the used projection
            projPJ  pjsrc;
            /// the zoomfactor used to draw the canvas
            QPointF zoomFactor;
            /// the scale of the global viewport
            QPointF scale;
            /// top left corner
            QPointF ref1;
            /// top right corner
            QPointF ref2;
            /// bottom right corner
            QPointF ref3;
            /// bottom left corner
            QPointF ref4;
            /// point of focus
            QPointF focus;
        };

        virtual void draw(buffer_t& buf) = 0;

        /**
           @brief Test if map has been loaded successfully
           @return Return false if map is not loaded
         */
        bool activated(){return isActivated;}

        /**
           @brief getInfo
           @param px
           @param str
         */
        virtual void getInfo(const QPoint& px, QString& str){Q_UNUSED(px); Q_UNUSED(str);}
        /**
           @brief getToolTip
           @param px
           @param str
         */
        virtual void getToolTip(const QPoint& px, QString& str){Q_UNUSED(px); Q_UNUSED(str);}

        /**
           @brief Read opacity value
           @return Return the opacity in a range of 0..100(full opacity)
         */
        int getOpacity(){return qRound(opacity * 100);}

    public slots:
        /**
           @brief Write opacity value
           @param value must be in the range of 0..100(full opacity)
         */
        void slotSetOpacity(int value){opacity = value / 100.0;}

    protected:
        void convertRad2M(QPointF &p);
        void convertM2Rad(QPointF &p);

        void drawTile(QImage& img, QPolygonF& l, QPainter& p);

        CMap * map;

        /// source projection of the current map file
        /**
            Has to be set by subclass. Destruction has to be
            handeled by subclass.
        */
        projPJ  pjsrc;
        /// target projection
        /**
            Is set by IMap() to WGS84. Will be freed by ~IMap()
        */
        projPJ  pjtar;

        /**
           @brief True if map was loaded successfully
         */
        bool isActivated;

        qreal opacity;

};

extern QPointF operator*(const QPointF& p1, const QPointF& p2);

extern QPointF operator/(const QPointF& p1, const QPointF& p2);


#endif //IMAP_H

