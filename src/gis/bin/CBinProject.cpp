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

#include "gis/bin/CBinProject.h"
#include "gis/gpx/CGpxProject.h"
#include "helpers/CSettings.h"


#include <QtWidgets>

CBinProject::CBinProject(const QString &filename, const QString &key, CGisListWks *parent)
    : IGisProject(key, filename, parent)
{
    setText(0, QFileInfo(filename).baseName());
    setIcon(0,QIcon("://icons/32x32/QmsProject.png"));

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(0, QObject::tr("Failed to open..."), QObject::tr("Failed to open %1").arg(filename), QMessageBox::Abort);
        return;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setVersion(QDataStream::Qt_5_2);

    *this << in;

    file.close();

    setToolTip(0, getInfo());
    valid = true;
}

CBinProject::~CBinProject()
{

}


void CBinProject::save()
{

    if(filename.isEmpty())
    {
        saveAs();
    }
    else
    {
        saveAs(filename, *this);
        markAsSaved();
    }
}

void CBinProject::saveAs()
{
    SETTINGS;
    QString path = cfg.value("Paths/lastGisPath", QDir::homePath()).toString();

    QString filter = "*.qms";
    QString fn = QFileDialog::getSaveFileName(0, QObject::tr("Save GIS data to..."), path, "*.gpx;; *.qms", &filter);

    if(fn.isEmpty())
    {
        return;
    }


    if(filter == "*.gpx")
    {
        CGpxProject::saveAs(fn, *this);
    }
    else if(filter == "*.qms")
    {
        saveAs(fn, *this);

        filename = fn;
        setText(0, QFileInfo(filename).baseName());
        markAsSaved();
    }
    else
    {
        return;
    }

    path = QFileInfo(fn).absolutePath();
    cfg.setValue("Paths/lastGisPath", path);

}

void CBinProject::saveAs(const QString& fn, IGisProject& project)
{
    QString _fn_ = fn;
    QFileInfo fi(_fn_);
    if(fi.suffix() != "qms")
    {
        _fn_ += ".qms";
    }

    // todo save qms
    QFile file(_fn_);

    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(0, QObject::tr("Failed to open..."), QObject::tr("Failed to open %1").arg(_fn_), QMessageBox::Abort);
        return;
    }
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setVersion(QDataStream::Qt_5_2);

    QString tmp = project.getFilename();
    project.setFilename(_fn_);

    project >> out;

    project.setFilename(tmp);

    file.close();
}

