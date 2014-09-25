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


#include "gis/wpt/CScrOptWpt.h"
#include "gis/wpt/CGisItemWpt.h"
#include "gis/wpt/CProjWpt.h"
#include "gis/CGisWidget.h"
#include "mouse/IMouse.h"
#include "canvas/CCanvas.h"
#include "CMainWindow.h"

#include <QtWidgets>

CScrOptWpt::CScrOptWpt(CGisItemWpt *wpt, const QPoint& origin, IMouse *parent)
    : IScrOpt(parent->getCanvas())
    , key(wpt->getKey())
{
    setupUi(this);
    setOrigin(origin);
    label->setFont(CMainWindow::self().getMapFont());
    label->setText(IGisItem::removeHtml(wpt->getInfo()));

    toolMove->setEnabled(!wpt->isReadOnly());
    toolProj->setEnabled(!wpt->isGeocache());

    anchor = wpt->getPointCloseBy(origin);
    move(anchor.toPoint() + QPoint(SCR_OPT_OFFSET,SCR_OPT_OFFSET));
    adjustSize();
    show();

    connect(toolDelete, SIGNAL(clicked()), this, SLOT(slotDelete()));
    connect(toolEdit, SIGNAL(clicked()), this, SLOT(slotEdit()));
    connect(toolMove, SIGNAL(clicked()), this, SLOT(slotMove()));
    connect(toolProj, SIGNAL(clicked()), this, SLOT(slotProj()));
}

CScrOptWpt::~CScrOptWpt()
{

}

void CScrOptWpt::slotDelete()
{
    CGisWidget::self().delItemByKey(key);
    deleteLater();
}

void CScrOptWpt::slotEdit()
{
    CGisWidget::self().editItemByKey(key);
    deleteLater();
}

void CScrOptWpt::slotMove()
{
    CGisWidget::self().moveWptByKey(key);
    deleteLater();
}

void CScrOptWpt::slotProj()
{
    CGisWidget::self().projWptByKey(key);
    deleteLater();
}


void CScrOptWpt::draw(QPainter& p)
{
    IGisItem * item = CGisWidget::self().getItemByKey(key);
    if(item == 0)
    {
        deleteLater();
        return;
    }
    item->drawHighlight(p);

    drawBubble(anchor, p);
}
