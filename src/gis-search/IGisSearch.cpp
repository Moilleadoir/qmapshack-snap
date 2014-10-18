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

#include "gis-search/IGisSearch.h"
#include "gis-search/CGisSearchWidget.h"
#include "gis/WptIcons.h"
#include "helpers/CWptIconDialog.h"

#include <QtWidgets>

IGisSearch::IGisSearch(QTreeWidget *parent)
    : QTreeWidgetItem(parent)
    , symName("Default")
{
    QPointF focus;
    edit = new CGisSearchWidget(parent);
    edit->toolIcon->setIcon(getWptIconByName(symName, focus));
    edit->toolIcon->setObjectName(symName);

    edit->lineEdit->addAction(QIcon("://icons/32x32/SearchGoogle.png"), QLineEdit::LeadingPosition);

    parent->setItemWidget(this, 0, edit);

    connect(edit->toolIcon, SIGNAL(clicked()), this, SLOT(slotChangeIcon()));
}

IGisSearch::~IGisSearch()
{

}

void IGisSearch::slotChangeIcon()
{
    CWptIconDialog dlg(edit->toolIcon);
    if(dlg.exec() == QDialog::Accepted)
    {
        symName = edit->toolIcon->objectName();
    }

}
