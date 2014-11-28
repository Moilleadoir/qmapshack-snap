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

#ifndef CDBPROJECT_H
#define CDBPROJECT_H

#include "gis/prj/IGisProject.h"
#include <QSqlDatabase>

class CDBProject : public IGisProject
{
    public:
        CDBProject(const QString &dbName, quint64 id, CGisListWks * parent);
        virtual ~CDBProject();

        void save();
        void saveAs();

        quint64 getId(){return id;}

        /**
           @brief Serialize object out of a QDataStream

           See CGisSerialization.cpp for implementation

           @param stream the binary data stream
           @return The stream object.
        */
        QDataStream& operator<<(QDataStream& stream);

        /**
           @brief Serialize object into a QDataStream

           See CGisSerialization.cpp for implementation

           @param stream the binary data stream
           @return The stream object.
        */
        QDataStream& operator>>(QDataStream& stream);



    private:
        QSqlDatabase db;
        quint64 id;
};

#endif //CDBPROJECT_H

