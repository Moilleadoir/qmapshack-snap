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

#ifndef CDBITEM_H
#define CDBITEM_H

#include <QTreeWidgetItem>
#include <QCoreApplication>

class IDBFolder;
class QSqlDatabase;

class CDBItem : public QTreeWidgetItem
{
    Q_DECLARE_TR_FUNCTIONS(CDBItem)
public:
    CDBItem(QSqlDatabase& db, quint64 id, IDBFolder * parent);
    virtual ~CDBItem();

    quint64 getId()
    {
        return id;
    }
    const QString& getKey()
    {
        return key;
    }

    /**
       @brief Send show/hide events to the workspace
     */
    void toggle();

    /**
       @brief Delete the folder/item relation in the database
     */
    void remove();

    void updateAge();

private:
    QSqlDatabase& db;
    quint64 id;

    int type = 0;
    QString key;
};

#endif //CDBITEM_H

