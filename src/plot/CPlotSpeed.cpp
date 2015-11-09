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

#include "plot/CPlotAxis.h"
#include "plot/CPlotSpeed.h"

CPlotSpeed::CPlotSpeed(QWidget *parent)
    : IPlot(0, CPlotData::eAxisLinear, eModeNormal, parent)
{
}

CPlotSpeed::~CPlotSpeed()
{
}

void CPlotSpeed::setTrack(CGisItemTrk * track)
{
    trk = track;
    trk->registerPlot(this);

    updateData();
}

void CPlotSpeed::updateData()
{
    if(isHidden())
    {
        return;
    }

    clear();
    if(trk->getTotalElapsedSeconds() == 0)
    {
        resetZoom();
        update();
        return;
    }

    if(mode == eModeIcon)
    {
        setXLabel(trk->getName());
        setYLabel("");
    }
    else
    {
        setXLabel(tr("distance [%1]").arg(IUnit::self().baseunit));
        setYLabel(tr("speed. [%1]").arg(IUnit::self().speedunit));
    }


    QPolygonF lineSpeed;

    qreal speedfactor = IUnit::self().speedfactor;
    const CGisItemTrk::trk_t& t = trk->getTrackData();
    foreach (const CGisItemTrk::trkseg_t& seg, t.segs)
    {
        foreach(const CGisItemTrk::trkpt_t& trkpt, seg.pts)
        {
            if(trkpt.flags & CGisItemTrk::trkpt_t::eHidden)
            {
                continue;
            }

            if(trkpt.speed != NOFLOAT)
            {
                lineSpeed << QPointF(trkpt.distance, trkpt.speed * speedfactor);
            }
        }
    }

    newLine(lineSpeed, "GPS");
    setLimits();
    data->ymin = 0;
    data->y().setLimits(0,data->ymax);
    resetZoom();
}

void CPlotSpeed::setMouseFocus(const CGisItemTrk::trkpt_t * ptMouseMove)
{
    if(ptMouseMove == 0)
    {
        if(posMouse != NOPOINT)
        {
            posMouse = NOPOINT;
            needsRedraw = true;
        }
    }
    else
    {
        if(posMouse == NOPOINT)
        {
            needsRedraw = true;
        }

        posMouse.rx() = left  + data->x().val2pt(ptMouseMove->distance);
        posMouse.ry() = top  +  data->y().val2pt(ptMouseMove->speed);
    }
    update();
}

