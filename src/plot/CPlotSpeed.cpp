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

#include "CPlotSpeed.h"

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
    CPlotData::axistype_e type = data->axisType;

    if(mode == eModeIcon)
    {
        setXLabel(trk->getName());
        setYLabel("");
    }
    else
    {
        if(type == CPlotData::eAxisLinear)
        {
            setXLabel(tr("distance [%1]").arg(IUnit::self().baseunit));
        }
        else
        {
            setXLabel(tr("time [h]"));
        }
        setYLabel(tr("speed. [%1]").arg(IUnit::self().speedunit));
    }

    QPolygonF lineSpeed;

    qreal speedfactor = IUnit::self().speedfactor;
    const CGisItemTrk::trk_t& t = trk->getTrackData();
    foreach (const CGisItemTrk::trkseg_t& seg, t.segs)
    {
        foreach(const CGisItemTrk::trkpt_t& trkpt, seg.pts)
        {
            if(trkpt.flags & CGisItemTrk::trkpt_t::eDeleted)
            {
                continue;
            }

            if(trkpt.speed != NOFLOAT)
            {
                lineSpeed << QPointF(type == CPlotData::eAxisLinear ? trkpt.distance : (qreal)trkpt.time.toTime_t(), trkpt.speed * speedfactor);
            }
        }
    }

    clear();
    newLine(lineSpeed, "GPS");
    setLimits();
    resetZoom();
}

void CPlotSpeed::setPointOfFocus(const CGisItemTrk::trkpt_t * pt)
{

}
