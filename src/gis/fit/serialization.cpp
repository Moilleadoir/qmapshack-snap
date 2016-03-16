/**********************************************************************************************
    Copyright (C) 2015 Ivo Kronenberg

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

#include "gis/fit/defs/fit_enums.h"
#include "gis/fit/defs/fit_fields.h"
#include "gis/rte/CGisItemRte.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/wpt/CGisItemWpt.h"

static const qreal degrees = 180.0;
static const qreal twoPow31 = qPow(2, 31);
static const uint sec1970to1990 = QDateTime(QDate(1989, 12, 31), QTime(0, 0, 0),Qt::UTC).toTime_t();


namespace CFitDataConverter
{
    /**
     * converts the semicircle to the WGS-84 geoids (Degrees Decimal Minutes (DDD MM.MMM)).
     * North latitude +, South latitude -
     * East longitude +, West longitude -
     *
       return: the given semicircle value converted to degree.
     */
    static qreal toDegree(qint32 semicircles)
    {
        return semicircles * (degrees / twoPow31);
    }

    /**
       timestamp: seconds since UTC 00:00 Dec 31 1989
     */
    static QDateTime toDateTime(quint32 timestamp)
    {
        QDateTime dateTime;
        dateTime.setTime_t(sec1970to1990 + timestamp);
        dateTime.setTimeSpec(Qt::UTC);
        return dateTime;
    }
}

template<typename T>
static void readKnownExtensions(T &exts, const CFitMessage &mesg)
{
        // see gis/trk/CKnownExtension for the keys of the extensions
        if(mesg.isFieldValueValid(eRecordHeartRate))
        {
            exts["gpxtpx:TrackPointExtension|gpxtpx:hr"] = mesg.getFieldValue(eRecordHeartRate);
        }
        if(mesg.isFieldValueValid(eRecordTemperature))
        {
            exts["gpxtpx:TrackPointExtension|gpxtpx:atemp"] = mesg.getFieldValue(eRecordTemperature);
        }
        if(mesg.isFieldValueValid(eRecordCadence))
        {
            exts["gpxtpx:TrackPointExtension|gpxtpx:cad"] = mesg.getFieldValue(eRecordCadence);
        }
        if(mesg.isFieldValueValid(eRecordSpeed))
        {
            exts["speed"] = mesg.getFieldValue(eRecordSpeed);
        }
}

bool readFitRecord(const CFitMessage &mesg, IGisItem::wpt_t &pt)
{
    if(mesg.isFieldValueValid(eRecordPositionLong) && mesg.isFieldValueValid(eRecordPositionLat))
    {
        pt.lon = CFitDataConverter::toDegree(mesg.getFieldValue(eRecordPositionLong).toInt());
        pt.lat = CFitDataConverter::toDegree(mesg.getFieldValue(eRecordPositionLat).toInt());
        // QVariant.toInt() does not convert double to int but return 0.
        pt.ele = (int) mesg.getFieldValue(eRecordEnhancedAltitude).toDouble();
        pt.time = CFitDataConverter::toDateTime(mesg.getFieldValue(eRecordTimestamp).toUInt());

        readKnownExtensions(pt.extensions, mesg);

        return true;
    }
    return false;
}

bool readFitRecord(const CFitMessage &mesg, CGisItemTrk::trkpt_t &pt)
{
    if(readFitRecord(mesg, (IGisItem::wpt_t &)pt))
    {
        pt.speed = mesg.getFieldValue(eRecordSpeed).toDouble();

        readKnownExtensions(pt.extensions, mesg);

        pt.extensions.squeeze();
        return true;
    }
    return false;
}

void readFitCoursePoint(const CFitMessage &mesg, IGisItem::wpt_t &wpt)
{
    if(mesg.isFieldValueValid(eCoursePointName))
    {
        wpt.name = mesg.getFieldValue(eCoursePointName).toString();
    }
    if(mesg.isFieldValueValid(eCoursePointTimestamp))
    {
        wpt.time = CFitDataConverter::toDateTime(mesg.getFieldValue(eCoursePointTimestamp).toUInt());
    }

    if(mesg.isFieldValueValid(eCoursePointPositionLong) && mesg.isFieldValueValid(eCoursePointPositionLat))
    {
        wpt.lon = CFitDataConverter::toDegree(mesg.getFieldValue(eCoursePointPositionLong).toInt());
        wpt.lat = CFitDataConverter::toDegree(mesg.getFieldValue(eCoursePointPositionLat).toInt());
    }
    // TODO find appropriate icon for different CoursePointType (CoursePoint***)
    // see WptIcons.initWptIcons() for all values
    wpt.sym = "Waypoint";
}

void CGisItemTrk::readTrkFromFit(CFitStream &stream)
{
    const CFitMessage& mesg = stream.firstMesgOf(eMesgNumCourse);
    if(mesg.isValid() && mesg.isFieldValueValid(eCourseName))
    {
        trk.name = mesg.getFieldValue(eCourseName).toString();
    }
    else
    {
        // course files can have a name but activities don't, so use just the filename
        trk.name = QFileInfo(stream.getFileName()).baseName().replace("_", " ");
    }

    // note to the FIT specification: the specification allows different ordering of the messages.
    // Record messages can either be at the beginning or in chronological order within the record
    // messages. Garmin devices uses the chronological ordering. We only consider the chronological
    // order, otherwise timestamps (of records and events) must be compared to each other.
    trkseg_t seg;
    do
    {
        const CFitMessage& mesg = stream.nextMesg();
        if(mesg.getGlobalMesgNr() == eMesgNumRecord)
        {
            // for documentation: MesgNumActivity, MesgNumSession, MesgNumLap, MesgNumLength could also contain data
            CGisItemTrk::trkpt_t pt;
            if(readFitRecord(mesg, pt))
            {
                seg.pts.append(std::move(pt));
            }
        }
        else if(mesg.getGlobalMesgNr() ==  eMesgNumEvent)
        {
            if(mesg.getFieldValue(eEventEvent).toUInt() == eEventTimer)
            {
                uint event = mesg.getFieldValue(eEventEventType).toUInt();
                if(event == eEventTypeStop || event == eEventTypeStopAll || event == eEventTypeStopDisableAll)
                {
                    if(!seg.pts.isEmpty())
                    {
                        trk.segs.append(seg);
                        seg = trkseg_t();
                    }
                }
            }
        }
    }
    while (stream.hasMoreMesg());

    if(trk.segs.size() == 0 && !seg.pts.isEmpty())
    {
        // navigation course files do not have to have start / stop event, so add the segment now.
        trk.segs.append(seg);
    }
    if(trk.segs.isEmpty())
    {
        throw tr("FIT file %1 contains no GPS data.").arg(stream.getFileName());
    }
}


void CGisItemWpt::readWptFromFit(CFitStream &stream)
{
    const CFitMessage& mesg = stream.lastMesg();
    readFitCoursePoint(mesg, wpt);
}


void CGisItemRte::readRteFromFit(CFitStream &stream)
{
    // a course file could be considered as a route...
    const CFitMessage& mesg = stream.firstMesgOf(eMesgNumCourse);
    if(mesg.isFieldValueValid(eCourseName))
    {
        rte.name = mesg.getFieldValue(eCourseName).toString();
    }
    do
    {
        const CFitMessage& mesg = stream.nextMesg();
        if(mesg.getGlobalMesgNr() == eMesgNumRecord)
        {
            rtept_t pt;
            if(readFitRecord(mesg, pt))
            {
                rte.pts.append(pt);
            }
        }
    }
    while (stream.hasMoreMesg());
}


