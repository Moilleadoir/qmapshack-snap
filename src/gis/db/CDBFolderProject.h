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

#ifndef CDBFOLDERPROJECT_H
#define CDBFOLDERPROJECT_H

#include "gis/db/IDBFolder.h"

class CDBFolderProject : public IDBFolder
{
    public:
        CDBFolderProject(quint64 key, QTreeWidgetItem *parent);
        virtual ~CDBFolderProject();
};

#endif //CDBFOLDERPROJECT_H

