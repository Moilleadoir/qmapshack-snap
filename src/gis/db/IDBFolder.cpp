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

#include "gis/db/macros.h"
#include "gis/db/IDBFolder.h"
#include "gis/db/CDBFolderGroup.h"
#include "gis/db/CDBFolderProject.h"
#include "gis/db/CDBFolderOther.h"
#include "gis/db/CDBItem.h"
#include "gis/IGisItem.h"

#include <QtSql>

IDBFolder::IDBFolder(QSqlDatabase& db, type_e type, quint64 id, QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent, type)
    , db(db)
    , id(id)
{
    setupFromDB();
}

IDBFolder::IDBFolder(QSqlDatabase& db, type_e type, quint64 id, QTreeWidget * parent)
    : QTreeWidgetItem(parent, type)
    , db(db)
    , id(id)
{
    setupFromDB();
}

IDBFolder::~IDBFolder()
{

}

IDBFolder * IDBFolder::createFolderByType(QSqlDatabase& db, int type, quint64 id, QTreeWidgetItem * parent)
{
    switch(type)
    {
    case eTypeGroup:
        return new CDBFolderGroup(db, id, parent);
    case eTypeProject:
        return new CDBFolderProject(db, id, parent);
    case eTypeOther:
        return new CDBFolderOther(db, id, parent);
    default:
        return 0;
    }
}

void IDBFolder::setupFromDB()
{
    if(id == 0)
    {
        return;
    }
    QSqlQuery query(db);

    query.prepare("SELECT name FROM folders WHERE id=:id");
    query.bindValue(":id", id);
    QUERY_EXEC(return);
    query.next();

    setText(eColumnName, query.value(0).toString());

    query.prepare("SELECT EXISTS(SELECT 1 FROM folder2folder WHERE parent=:id LIMIT 1)");
    query.bindValue(":id", id);
    QUERY_EXEC(return);
    query.next();

    if(query.value(0).toInt() == 1)
    {
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
    else
    {
        query.prepare("SELECT EXISTS(SELECT 1 FROM folder2item WHERE parent=:id LIMIT 1)");
        query.bindValue(":id", id);
        QUERY_EXEC(return);
        query.next();
        if(query.value(0).toInt() == 1)
        {
            setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }

}

void IDBFolder::expanding()
{
    qDeleteAll(takeChildren());

    QSqlQuery query(db);

    // folders 1st
    query.prepare("SELECT t1.child, t2.type FROM folder2folder AS t1, folders AS t2 WHERE t1.parent = :id AND t2.id = t1.child ORDER BY t2.id");
    query.bindValue(":id", id);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 idChild     = query.value(0).toULongLong();
        quint32 typeChild   = query.value(1).toInt();
        IDBFolder::createFolderByType(db, typeChild, idChild, this);
    }

    // tracks 2nd
    query.prepare("SELECT t1.child FROM folder2item AS t1, items AS t2 WHERE t1.parent = :id AND t2.id = t1.child AND t2.type=:type ORDER BY t2.id");
    query.bindValue(":id", id);
    query.bindValue(":type", IGisItem::eTypeTrk);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 idChild = query.value(0).toULongLong();
        new CDBItem(db, idChild, this);
    }

    // routes 3rd
    query.prepare("SELECT t1.child FROM folder2item AS t1, items AS t2 WHERE t1.parent = :id AND t2.id = t1.child AND t2.type=:type ORDER BY t2.id");
    query.bindValue(":id", id);
    query.bindValue(":type", IGisItem::eTypeRte);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 idChild = query.value(0).toULongLong();
        new CDBItem(db, idChild, this);
    }

    //waypoints 4th
    query.prepare("SELECT t1.child FROM folder2item AS t1, items AS t2 WHERE t1.parent = :id AND t2.id = t1.child AND t2.type=:type ORDER BY t2.id");
    query.bindValue(":id", id);
    query.bindValue(":type", IGisItem::eTypeWpt);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 idChild = query.value(0).toULongLong();
        new CDBItem(db, idChild, this);
    }

    // overlays 5th
    query.prepare("SELECT t1.child FROM folder2item AS t1, items AS t2 WHERE t1.parent = :id AND t2.id = t1.child AND t2.type=:type ORDER BY t2.id");
    query.bindValue(":id", id);
    query.bindValue(":type", IGisItem::eTypeOvl);
    QUERY_EXEC(return);
    while(query.next())
    {
        quint64 idChild = query.value(0).toULongLong();
        new CDBItem(db, idChild, this);
    }


}

void IDBFolder::close(quint64 idFolder)
{
    if(id == idFolder)
    {
        treeWidget()->blockSignals(true);
        setCheckState(eColumnCheckbox, Qt::Unchecked);
        treeWidget()->blockSignals(false);
    }
    else
    {
        for(int i = 0; i < childCount(); i++)
        {
            IDBFolder * folder = dynamic_cast<IDBFolder*>(child(i));
            if(folder)
            {
                folder->close(idFolder);
            }
        }
    }

}

void IDBFolder::update(quint64 idFolder)
{
    if(id == idFolder && isExpanded())
    {
        treeWidget()->blockSignals(true);
        expanding();
        treeWidget()->blockSignals(false);
    }
    else
    {
        for(int i = 0; i < childCount(); i++)
        {
            IDBFolder * folder = dynamic_cast<IDBFolder*>(child(i));
            if(folder)
            {
                folder->update(idFolder);
            }
        }
    }
}
