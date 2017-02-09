/**********************************************************************************************
    Copyright (C) 2016 Michel Durand zero@cms123.fr

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
#include "gis/CGisListWks.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/wpt/CGisItemWpt.h"
#include "gis/tcx/CTcxProject.h"

#include <QtWidgets>




CTcxProject::CTcxProject(const QString &filename, CGisListWks * parent)
    : IGisProject(eTypeTcx, filename, parent)
{
    setIcon(CGisListWks::eColumnIcon, QIcon("://icons/32x32/TcxProject.png"));
    blockUpdateItems(true);
    loadTcx(filename);
    blockUpdateItems(false);
    setupName(QFileInfo(filename).completeBaseName().replace("_", " "));
}


void CTcxProject::loadTcx(const QString& filename)
{
    try
    {
        loadTcx(filename, this);
    }
    catch(QString &errormsg)
    {
        QMessageBox::critical(CMainWindow::getBestWidgetForParent(),
                              tr("Failed to load file %1...").arg(filename), errormsg, QMessageBox::Abort);
        valid = false;
    }
}


void CTcxProject::loadTcx(const QString &filename, CTcxProject *project)
{
       QFile file(filename);
    
    // if the file does not exist, the file name is assumed to be a name for a new project
    if (!file.exists() || QFileInfo(filename).suffix().toLower() != "tcx")
    {
        project->filename.clear();
        project->setupName(filename);
        project->setToolTip(CGisListWks::eColumnName, project->getInfo());
        project->valid = true;
        return;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        throw tr("Failed to open %1").arg(filename);
    }


    QDomDocument xml;
    QString msg;
    int line;
    int column;
    if (!xml.setContent(&file, false, &msg, &line, &column))
    {
        file.close();
        throw tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg);
    }
    file.close();

    QDomElement xmlTcx = xml.documentElement();
    if (xmlTcx.tagName() != "TrainingCenterDatabase")
    {
        throw tr("Not a TCX file: %1").arg(filename);
    }

    const QDomNodeList& tcxActivitys = xmlTcx.elementsByTagName("Activity");
    const QDomNodeList& tcxCourses = xmlTcx.elementsByTagName("Course");
        
    if (!tcxActivitys.item(0).isElement() && !tcxCourses.item(0).isElement())
    {
        if ( xmlTcx.elementsByTagName("Workout").item(0).isElement() ) 
        {
            throw tr("This TCX file contains at least 1 workout, but neither an activity nor a course. "
                "As workouts do not contain position data, they can not be imported to QMapShack.");
        }
        else
        {
            throw tr("This TCX file does not contain any activity or course: %1").arg(filename); 
        }
    }


    for (int i = 0; i < tcxActivitys.count(); i++)
    {
        project->loadActivity(tcxActivitys.item(i));

    }

    for (int i = 0; i < tcxCourses.count(); i++)
    {
        project->loadCourse(tcxCourses.item(i));

    }
  
  
    project->sortItems();
    project->setupName(QFileInfo(filename).completeBaseName().replace("_", " "));
    project->setToolTip(CGisListWks::eColumnName, project->getInfo());
    project->valid = true;
}


void CTcxProject::loadActivity(QDomNode& activityRootNode)
{
    if (activityRootNode.isElement())
    {
        CTrackData trk;

        trk.name = activityRootNode.toElement().elementsByTagName("Id").item(0).firstChild().nodeValue(); // activities do not have a "Name" but an "Id" instead (containing start date-time)

        const QDomNodeList& tcxLaps = activityRootNode.toElement().elementsByTagName("Lap");

        trk.segs.resize(tcxLaps.count());
        for (int i = 0; i < tcxLaps.count(); i++)    // browse laps
        {
            CTrackData::trkseg_t *seg = &(trk.segs[i]);

            const QDomNodeList& tcxLapTrackpts = tcxLaps.item(i).toElement().elementsByTagName("Trackpoint");

            for (int j = 0; j < tcxLapTrackpts.count(); j++) // browse trackpoints
            {
                const QDomElement positionElement = tcxLapTrackpts.item(j).toElement().elementsByTagName("Position").item(0).toElement();

                if (positionElement.isElement()) // if this trackpoint contains position, i.e. GPSr was able to capture position
                {
                    CTrackData::trkpt_t trkpt;

                    QString timeString = tcxLapTrackpts.item(j).toElement().elementsByTagName("Time").item(0).firstChild().nodeValue();
                    QDateTime trkPtTimestamp;
                    IUnit::parseTimestamp(timeString, trkPtTimestamp);
                    trkpt.time = trkPtTimestamp;

                    trkpt.lat = positionElement.elementsByTagName("LatitudeDegrees").item(0).firstChild().nodeValue().toDouble();
                    trkpt.lon = positionElement.elementsByTagName("LongitudeDegrees").item(0).firstChild().nodeValue().toDouble();
                    trkpt.ele = tcxLapTrackpts.item(j).toElement().elementsByTagName("AltitudeMeters").item(0).firstChild().nodeValue().toDouble();

                    const QDomElement HRElement = tcxLapTrackpts.item(j).toElement().elementsByTagName("HeartRateBpm").item(0).toElement();
                    if (HRElement.isElement()) // if this trackpoint contains heartrate data, i.e. heartrate sensor data has been captured
                    {
                        trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:hr"] = HRElement.elementsByTagName("Value").item(0).firstChild().nodeValue().toDouble();
                    }

                    const QDomElement CADElement = tcxLapTrackpts.item(j).toElement().elementsByTagName("Cadence").item(0).toElement();
                    if (CADElement.isElement()) // if this trackpoint contains cadence data, i.e. cadence sensor data has been captured
                    {
                        trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:cad"] = CADElement.firstChild().nodeValue().toDouble();
                    }

                    seg->pts.append(trkpt); // 1 TCX lap gives 1 GPX track segment 
                }
            }
        }

        new CGisItemTrk(trk, this);
    }
}


void CTcxProject::loadCourse(QDomNode& courseRootNode)
{
    if (courseRootNode.isElement())
    {
        CTrackData trk;

        trk.name = courseRootNode.toElement().elementsByTagName("Name").item(0).firstChild().nodeValue();
        QString timeString;
        QDateTime trkPtTimestamp;
        trk.segs.resize(1);
        CTrackData::trkseg_t *seg = &(trk.segs[0]);

        const QDomNodeList& tcxTrackpts = courseRootNode.toElement().elementsByTagName("Trackpoint");

        for (int i = 0; i < tcxTrackpts.count(); i++) // browse trackpoints
        {
            const QDomElement positionElement = tcxTrackpts.item(i).toElement().elementsByTagName("Position").item(0).toElement();

            if (positionElement.isElement()) // if this trackpoint contains position, i.e. GPSr was able to capture position
            {
                CTrackData::trkpt_t trkpt;

                timeString = tcxTrackpts.item(i).toElement().elementsByTagName("Time").item(0).firstChild().nodeValue();
                IUnit::parseTimestamp(timeString, trkPtTimestamp);
                trkpt.time = trkPtTimestamp;

                trkpt.lat = positionElement.elementsByTagName("LatitudeDegrees").item(0).firstChild().nodeValue().toDouble();
                trkpt.lon = positionElement.elementsByTagName("LongitudeDegrees").item(0).firstChild().nodeValue().toDouble();
                trkpt.ele = tcxTrackpts.item(i).toElement().elementsByTagName("AltitudeMeters").item(0).firstChild().nodeValue().toDouble();

                const QDomElement HRElement = tcxTrackpts.item(i).toElement().elementsByTagName("HeartRateBpm").item(0).toElement();
                if (HRElement.isElement()) // if this trackpoint contains heartrate data, i.e. heartrate sensor data has been captured
                {
                    trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:hr"] = HRElement.elementsByTagName("Value").item(0).firstChild().nodeValue().toDouble();
                }

                const QDomElement CADElement = tcxTrackpts.item(i).toElement().elementsByTagName("Cadence").item(0).toElement();
                if (CADElement.isElement()) // if this trackpoint contains cadence data, i.e. cadence sensor data has been captured
                {
                    trkpt.extensions["gpxtpx:TrackPointExtension|gpxtpx:cad"] = CADElement.firstChild().nodeValue().toDouble();
                }

                seg->pts.append(trkpt);
            }
        }

        new CGisItemTrk(trk, this);

        const QDomNodeList& tcxCoursePts = courseRootNode.toElement().elementsByTagName("CoursePoint");
        for (int i = 0; i < tcxCoursePts.count(); i++) // browse course points
        {
            QString name = tcxCoursePts.item(i).toElement().elementsByTagName("Name").item(0).firstChild().nodeValue();
            qreal lat = tcxCoursePts.item(i).toElement().elementsByTagName("Position").item(0).toElement().elementsByTagName("LatitudeDegrees").item(0).firstChild().nodeValue().toDouble();
            qreal lon = tcxCoursePts.item(i).toElement().elementsByTagName("Position").item(0).toElement().elementsByTagName("LongitudeDegrees").item(0).firstChild().nodeValue().toDouble();
            qreal ele = tcxCoursePts.item(i).toElement().elementsByTagName("AltitudeMeters").item(0).firstChild().nodeValue().toDouble();
            QString icon = tcxCoursePts.item(i).toElement().elementsByTagName("PointType").item(0).firstChild().nodeValue(); // there is no "icon" in course points ;  "PointType" is used instead (can be "turn left", "turn right", etc... See list in http://www8.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd)

            new CGisItemWpt(QPointF(lon, lat), ele, QDateTime::currentDateTime().toUTC(), name, icon, this); // 1 TCX course point gives 1 GPX waypoint
        }
    }
}


bool CTcxProject::saveAs(const QString& fn, IGisProject& project)
{
    QString _fn_ = fn;
    QFileInfo fi(_fn_);
    if (fi.suffix().toLower() != "tcx")
    {
        _fn_ += ".tcx";
    }

    project.mount();

    //  ---- start content of tcx
    QDomDocument doc;
    QDomElement tcx = doc.createElement("TrainingCenterDatabase");
    doc.appendChild(tcx);

    tcx.setAttribute("xmlns", "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2");
    tcx.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    tcx.setAttribute("xsi:schemaLocation", "http://www.garmin.com/xmlschemas/ProfileExtension/v1 http://www.garmin.com/xmlschemas/UserProfilePowerExtensionv1.xsd http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd http://www.garmin.com/xmlschemas/UserProfile/v2 http://www.garmin.com/xmlschemas/UserProfileExtensionv2.xsd");

    
    CGisItemTrk *trkItem = nullptr;
    for (int i = 0; i < project.childCount(); i++) // find 1st project track
    {
        trkItem = dynamic_cast<CGisItemTrk*>(project.child(i));
        if (nullptr == trkItem)
        {
            continue;
        }
        else
        {
            break;
        }
    }

    if (nullptr == trkItem)
    {
        int res = QMessageBox::warning(CMainWindow::getBestWidgetForParent(), tr("No track in selected project...")
            , tr("The project you have selected does not contain any track ! "
            "A course with no track makes no sense. "
            "<b>Please add a track to this project and try again.</b>")
            , QMessageBox::Ok, QMessageBox::Ok);
             return false;
     }


    if (!trkItem->isTrkTimeValid())
    {
        int res = QMessageBox::warning(CMainWindow::getBestWidgetForParent(), tr("Track with invalid timestamps...")
            , tr("The track you have selected contains trackpoints with "
            "invalid timestamps. "
            "Device might not accept the generated TCX course file if left as is. "
            "<b>Do you want to apply a filter with constant speed (10 m/s) and continue?</b>")
            , QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (res == QMessageBox::No)
        {
            return false;
        }

        if (res == QMessageBox::Yes)
        {
            trkItem->filterSpeed(10);
        }
    }

    QList<QDateTime> trkPtToOverwriteDateTimes;
    QList<qint32> trkPtToOverwriteElevations;
 
    for (int i = 0; i < project.childCount(); i++) // browse waypoints 
    {
        CGisItemWpt *wptItem = dynamic_cast<CGisItemWpt*>(project.child(i));
        if (nullptr == wptItem)
        {
            continue;
        }
        else
        {
            trkPtToOverwriteDateTimes << trkItem->getCloserPtDateTime(wptItem->getPosition());
            trkPtToOverwriteElevations << wptItem->getElevation();
        }
    }
    

    tcx.appendChild(doc.createElement("Courses"));
    tcx.lastChild().appendChild(doc.createElement("Course"));

    tcx.lastChild().lastChild().appendChild(doc.createElement("Name"));
    QString str = trkItem->getName();
    str.truncate(15);
    tcx.lastChild().lastChild().lastChild().appendChild(doc.createTextNode(str));

            
    QDomElement lapElmt = doc.createElement("Lap");
    tcx.lastChild().lastChild().appendChild(lapElmt);

    lapElmt.appendChild(doc.createElement("TotalTimeSeconds"));
    lapElmt.lastChild().appendChild(doc.createTextNode(QString::number(trkItem->getTotalElapsedSeconds())));

    lapElmt.appendChild(doc.createElement("DistanceMeters"));
    lapElmt.lastChild().appendChild(doc.createTextNode(QString::number(trkItem->getTotalDistance())));
            
    lapElmt.appendChild(doc.createElement("Intensity"));
    lapElmt.lastChild().appendChild(doc.createTextNode("Active"));

    trkItem->saveTCX(tcx, trkPtToOverwriteDateTimes, trkPtToOverwriteElevations);

    

    int j = 0;
    for (int i = 0; i < project.childCount(); i++)
    {
        CGisItemWpt *item = dynamic_cast<CGisItemWpt*>(project.child(i));
        if (nullptr == item)
        {
            continue;
        }

        item->saveTCX(tcx, trkPtToOverwriteDateTimes[j]);
        j++;
    }



    bool res = true;
    try
    {
        QFile file(_fn_);
        if (!file.open(QIODevice::WriteOnly))
        {
            throw tr("Failed to create file '%1'").arg(_fn_);
        }
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>" << endl;

        out << doc.toString();
        file.close();
        if (file.error() != QFile::NoError)
        {
            throw tr("Failed to write file '%1'").arg(_fn_);
        }
    }
    catch (const QString& msg)
    {
        // as saveAs() can be called from the thread that exports a database showing the
        // message box will crash the app. Therefore we test if the current thread is the
        // application's main thread. If not we forward the exception.
        //
        // Not sure if that is a good concept.
        if (QThread::currentThread() == qApp->thread())
        {
            QMessageBox::warning(CMainWindow::getBestWidgetForParent(), tr("Saving GIS data failed..."), msg, QMessageBox::Abort);
        }
        else
        {
            throw msg;
        }
        res = false;
    }
    project.umount();
    return res;
}





