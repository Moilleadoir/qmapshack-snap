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

#include "gis/db/CLostFoundProject.h"
#include "gis/CGisListWks.h"
#include "gis/db/macros.h"
#include "gis/wpt/CGisItemWpt.h"
#include "gis/trk/CGisItemTrk.h"
#include "gis/rte/CGisItemRte.h"
#include "gis/ovl/CGisItemOvlArea.h"

#include <QtWidgets>
#include <QtSql>


CLostFoundProject::CLostFoundProject(const QString &dbName, CGisListWks * parent)
    : CDBProject(parent)
{
    type    = eTypeLostFound;
    db      = QSqlDatabase::database(dbName);
    setIcon(CGisListWks::eColumnName,QIcon("://icons/32x32/DeleteMultiple.png"));

    filename        = dbName;
    metadata.name   = QObject::tr("Lost & Found");

    setupName(dbName);

    valid = true;
}

CLostFoundProject::~CLostFoundProject()
{

}

void CLostFoundProject::updateFromDb()
{
    qDeleteAll(takeChildren());

    QSqlQuery query(db);
    query.prepare("SELECT id, type FROM items AS t1 WHERE NOT EXISTS(SELECT * FROM folder2item WHERE child=t1.id) ORDER BY t1.type, t1.name");
    QUERY_EXEC(return);

    while(query.next())
    {
        quint64 id      = query.value(0).toULongLong();
        quint32 type    = query.value(1).toUInt();

        switch(type)
        {
            case IGisItem::eTypeWpt:
                new CGisItemWpt(id, db, this);
                break;
            case IGisItem::eTypeTrk:
                new CGisItemTrk(id, db, this);
                break;
            case IGisItem::eTypeRte:
                new CGisItemRte(id, db, this);
                break;
            case IGisItem::eTypeOvl:
                new CGisItemOvlArea(id, db, this);
                break;
            default:;
        }
    }
}
