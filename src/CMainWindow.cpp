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

#include "CMainWindow.h"
#include "CSettings.h"
#include "CCanvas.h"
#include "map/CMap.h"
#include "version.h"

#include <QtGui>
#include <QtWidgets>

CMainWindow * CMainWindow::pSelf = 0;

CMainWindow::CMainWindow()
{
    pSelf = this;
    qDebug() << WHAT_STR;
    setupUi(this);
    setWindowTitle(WHAT_STR);


    SETTINGS;
    // start ---- restore window geometry -----
    if ( cfg.contains("MainWindow/geometry"))
    {
        restoreGeometry(cfg.value("MainWindow/geometry").toByteArray());
    }
    else
    {
        setGeometry(0,0,800,600);
    }

    if ( cfg.contains("MainWindow/state"))
    {
        restoreState(cfg.value("MainWindow/state").toByteArray());
    }
    // end ---- restore window geometry -----


    connect(actionAddCanvas, SIGNAL(triggered()), this, SLOT(slotAddCanvas()));

    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(slotTabCloseRequest(int)));

}

CMainWindow::~CMainWindow()
{
    SETTINGS;
    cfg.setValue("MainWindow/state", saveState());
    cfg.setValue("MainWindow/geometry", saveGeometry());

}


void CMainWindow::addMapList(QListWidget * list, const QString &name)
{
    tabMaps->addTab(list,name);
}

void CMainWindow::delMapList(QListWidget * list)
{
    for(int i = 0; i < tabMaps->count(); i++)
    {
        QWidget * w = tabMaps->widget(i);
        if(w == list)
        {
            tabMaps->removeTab(i);
            delete w;
            return;
        }
    }
}


void CMainWindow::slotAddCanvas()
{
    CCanvas * canvas = new CCanvas(tabWidget);
    tabWidget->addTab(canvas, canvas->objectName());
    new CMap(canvas);
}

void CMainWindow::slotTabCloseRequest(int i)
{
    QWidget * w = tabWidget->widget(i);
    tabWidget->removeTab(i);

    delete w;
}


