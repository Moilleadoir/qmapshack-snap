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

#include "GeoMath.h"
#include "gis/trk/CDetailsTrk.h"
#include "gis/trk/filter/CFilterDelete.h"
#include "gis/trk/filter/CFilterDouglasPeuker.h"
#include "gis/trk/filter/CFilterMedian.h"
#include "gis/trk/filter/CFilterNewDate.h"
#include "gis/trk/filter/CFilterObscureDate.h"
#include "gis/trk/filter/CFilterOffsetElevation.h"
#include "gis/trk/filter/CFilterReplaceElevation.h"
#include "gis/trk/filter/CFilterReset.h"
#include "gis/trk/filter/CFilterSpeed.h"
#include "helpers/CLinksDialog.h"
#include "helpers/CSettings.h"
#include "helpers/CTextEditWidget.h"
#include "units/IUnit.h"

#include <QtWidgets>
#include <proj_api.h>

CDetailsTrk::CDetailsTrk(CGisItemTrk& trk, QWidget *parent)
    : QWidget(parent)
    , trk(trk)
    , originator(false)
{
    setupUi(this);

    QPixmap icon(16,8);
    for(int i=0; i < TRK_N_COLORS; ++i)
    {
        icon.fill(CGisItemTrk::lineColors[i]);
        comboColor->addItem(icon,"",CGisItemTrk::lineColors[i]);
    }

    int i = 0;
    while(!CActivityTrk::actDescriptor[i].name.isEmpty())
    {
        const CActivityTrk::desc_t& desc = CActivityTrk::actDescriptor[i];
        QCheckBox * check = new QCheckBox(this);
        check->setText(desc.name);
        check->setIcon(QIcon(desc.icon));
        check->setProperty("flag", desc.flag);
        check->setProperty("name", desc.name);
        check->setProperty("symbol", desc.icon);
        check->setObjectName("check" + desc.objName);
        //check->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        connect(check, SIGNAL(clicked(bool)), this, SLOT(slotActivitySelected(bool)));

        layoutActivities->addWidget(check);

        i++;
    }
    layoutActivities->addItem(new QSpacerItem(0,0,QSizePolicy::Maximum, QSizePolicy::MinimumExpanding));

    setupGui();

    plotElevation->setTrack(&trk);
    plotDistance->setTrack(&trk);
    plotSpeed->setTrack(&trk);


    if(trk.isOnDevice())
    {
        toolLock->setDisabled(true);
    }

    QTreeWidgetItem * item, * item0;
    item0 = new QTreeWidgetItem(treeFilter);
    item0->setIcon(0, QIcon("://icons/48x48/PointHide.png"));
    item0->setText(0, tr("Reduce visible track points"));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterDouglasPeuker(trk, treeFilter));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterReset(trk, treeFilter));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterDelete(trk, treeFilter));

    item0 = new QTreeWidgetItem(treeFilter);
    item0->setIcon(0, QIcon("://icons/48x48/SetEle.png"));
    item0->setText(0, tr("Change elevation of track points"));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterMedian(trk, treeFilter));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterReplaceElevation(trk, treeFilter));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterOffsetElevation(trk, treeFilter));

    item0 = new QTreeWidgetItem(treeFilter);
    item0->setIcon(0, QIcon("://icons/48x48/Time.png"));
    item0->setText(0, tr("Change timestamp of track points"));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterNewDate(trk, treeFilter));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterObscureDate(trk, treeFilter));

    item = new QTreeWidgetItem(item0);
    treeFilter->setItemWidget(item,0, new CFilterSpeed(trk, treeFilter));

    item0 = new QTreeWidgetItem(treeFilter);
    item0->setIcon(0, QIcon("://icons/48x48/TrkCut.png"));
    item0->setText(0, tr("Cut track into pieces"));

    SETTINGS;
    cfg.beginGroup("TrackDetails");
    checkProfile->setChecked(cfg.value("showProfile", true).toBool());
    checkSpeed->setChecked(cfg.value("showSpeed", true).toBool());
    checkProgress->setChecked(cfg.value("showProgress", true).toBool());
    splitter->restoreState(cfg.value("splitterSizes").toByteArray());
    treeWidget->header()->restoreState(cfg.value("trackPointListState").toByteArray());
    cfg.endGroup();

    connect(checkProfile, SIGNAL(clicked()), this, SLOT(slotShowPlots()));
    connect(checkSpeed, SIGNAL(clicked()), this, SLOT(slotShowPlots()));
    connect(checkProgress, SIGNAL(clicked()), this, SLOT(slotShowPlots()));
    connect(comboColor, SIGNAL(currentIndexChanged(int)), this, SLOT(slotColorChanged(int)));
    connect(toolLock, SIGNAL(toggled(bool)), this, SLOT(slotChangeReadOnlyMode(bool)));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));
    connect(textCmtDesc, SIGNAL(anchorClicked(QUrl)), this, SLOT(slotLinkActivated(QUrl)));
    connect(labelInfo, SIGNAL(linkActivated(QString)), this, SLOT(slotLinkActivated(QString)));

    connect(plotDistance, SIGNAL(sigMouseClickState(int)), this, SLOT(slotMouseClickState(int)));
    connect(plotElevation, SIGNAL(sigMouseClickState(int)), this, SLOT(slotMouseClickState(int)));
    connect(plotSpeed, SIGNAL(sigMouseClickState(int)), this, SLOT(slotMouseClickState(int)));

    connect(listHistory, SIGNAL(sigChanged()), this, SLOT(setupGui()));

    slotShowPlots();
}

CDetailsTrk::~CDetailsTrk()
{
    SETTINGS;
    cfg.beginGroup("TrackDetails");
    cfg.setValue("showProfile", checkProfile->isChecked());
    cfg.setValue("showSpeed", checkSpeed->isChecked());
    cfg.setValue("showProgress", checkProgress->isChecked());
    cfg.setValue("splitterSizes", splitter->saveState());
    cfg.setValue("trackPointListState", treeWidget->header()->saveState());
    cfg.endGroup();
}



void CDetailsTrk::setupGui()
{
    if(originator)
    {
        return;
    }
    CCanvas::setOverrideCursor(Qt::WaitCursor, "CDetailsTrk::setupGui");
    originator = true;

    QString str, val, unit;
    bool isReadOnly = trk.isReadOnly();

    tabWidget->widget(2)->setEnabled(!isReadOnly);

    if(trk.isTainted())
    {
        labelTainted->show();
    }
    else
    {
        labelTainted->hide();
    }

    labelInfo->setText(trk.getInfo(true));
    comboColor->setCurrentIndex(trk.getColorIdx());
    comboColor->setEnabled(!isReadOnly);
    toolLock->setChecked(isReadOnly);

    QList<QTreeWidgetItem*> items;
    const CGisItemTrk::trk_t& t = trk.getTrackData();
    foreach (const CGisItemTrk::trkseg_t& seg, t.segs)
    {
        foreach(const CGisItemTrk::trkpt_t& trkpt, seg.pts)
        {
            QTreeWidgetItem * item = new QTreeWidgetItem();
            item->setTextAlignment(eColNum,Qt::AlignLeft);
            item->setTextAlignment(eColEle,Qt::AlignRight);
            item->setTextAlignment(eColDelta,Qt::AlignRight);
            item->setTextAlignment(eColDist,Qt::AlignRight);
            item->setTextAlignment(eColAscend,Qt::AlignRight);
            item->setTextAlignment(eColDescend,Qt::AlignRight);
            item->setTextAlignment(eColSpeed,Qt::AlignRight);


            if(trkpt.flags & CGisItemTrk::trkpt_t::eHidden)
            {
                for(int i = 0; i < eColMax; i++)
                {
                    item->setForeground(i,QBrush(Qt::gray));
                }
            }
            else
            {
                for(int i = 0; i < eColMax; i++)
                {
                    item->setForeground(i,QBrush(Qt::black));
                }
            }

            item->setText(eColNum,QString::number(trkpt.idxTotal));
            if(trkpt.time.isValid())
            {
                item->setText(eColTime, IUnit::self().datetime2string(trkpt.time, true, QPointF(trkpt.lon, trkpt.lat)*DEG_TO_RAD));
            }
            else
            {
                item->setText(eColTime, "-");
            }

            if(trkpt.ele != NOINT)
            {
                IUnit::self().meter2elevation(trkpt.ele, val, unit);
                str = tr("%1 %2").arg(val).arg(unit);
            }
            else
            {
                str = "-";
            }
            item->setText(eColEle,str);

            IUnit::self().meter2distance(trkpt.deltaDistance, val, unit);
            item->setText(eColDelta, tr("%1 %2").arg(val).arg(unit));

            IUnit::self().meter2distance(trkpt.distance, val, unit);
            item->setText(eColDist, tr("%1 %2").arg(val).arg(unit));

            // speed
            if(trkpt.speed != NOFLOAT)
            {
                IUnit::self().meter2speed(trkpt.speed, val, unit);
                str = tr("%1 %2").arg(val).arg(unit);
            }
            else
            {
                str = "-";
            }
            item->setText(eColSpeed,str);

            if(trkpt.slope1 != NOFLOAT)
            {
                str = QString("%1°(%2%)").arg(trkpt.slope1,2,'f',0).arg(trkpt.slope2,2,'f',0);
            }
            else
            {
                str = "-";
            }
            item->setText(eColSlope,str);

            IUnit::self().meter2elevation(trkpt.ascend, val, unit);
            item->setText(eColAscend, tr("%1 %2").arg(val).arg(unit));
            IUnit::self().meter2elevation(trkpt.descend, val, unit);
            item->setText(eColDescend, tr("%1 %2").arg(val).arg(unit));

            // position
            GPS_Math_Deg_To_Str(trkpt.lon, trkpt.lat, str);
            item->setText(eColPosition,str);

            items << item;
        }
    }

    treeWidget->clear();
    treeWidget->addTopLevelItems(items);
    treeWidget->header()->resizeSections(QHeaderView::ResizeToContents);

    textCmtDesc->document()->clear();
    textCmtDesc->append(IGisItem::createText(isReadOnly, trk.getComment(), trk.getDescription(), trk.getLinks()));
    textCmtDesc->moveCursor (QTextCursor::Start);
    textCmtDesc->ensureCursorVisible();

    quint32 flags = trk.getActivities().getAllFlags();

    int i = 0;
    while(!CActivityTrk::actDescriptor[i].objName.isEmpty())
    {
        const CActivityTrk::desc_t& desc = CActivityTrk::actDescriptor[i];

        QCheckBox * check = findChild<QCheckBox*>("check" + desc.objName);
        if(check)
        {
            check->setChecked((flags & desc.flag) == desc.flag);
        }

        i++;
    }

    if((flags & CGisItemTrk::trkpt_t::eActMask) == 0)
    {
        labelActivityHelp->show();
    }
    else
    {
        labelActivityHelp->hide();
    }

    plotTrack->setTrack(&trk);
    listHistory->setupHistory(trk);

    originator = false;
    CCanvas::restoreOverrideCursor("CDetailsTrk::setupGui");
}

void CDetailsTrk::setMouseFocus(const CGisItemTrk::trkpt_t * pt)
{
    if(pt != 0)
    {
        plotTrack->setMouseFocus(pt->lon, pt->lat);
        labelInfoTrkPt->setText(trk.getInfoTrkPt(*pt));
        labelInfoProgress->setText(trk.getInfoProgress(*pt));
    }
    else
    {
        labelInfoTrkPt->setText("-\n-");
        labelInfoProgress->setText("-\n-");
    }
}

void CDetailsTrk::setMouseRangeFocus(const CGisItemTrk::trkpt_t * pt1, const CGisItemTrk::trkpt_t * pt2)
{
    if(pt1 && pt2)
    {
        labelInfoRange->setText(trk.getInfoRange(*pt1, *pt2));
    }
    else
    {
        labelInfoRange->setText("-\n-");
    }
}

void CDetailsTrk::setMouseClickFocus(const CGisItemTrk::trkpt_t * pt)
{
    if(pt != 0)
    {
        treeWidget->blockSignals(true);
        treeWidget->setCurrentItem(treeWidget->topLevelItem(pt->idxTotal));
        treeWidget->blockSignals(false);
    }
}

void CDetailsTrk::slotMouseClickState(int s)
{
    if(s == IPlot::eMouseClickIdle)
    {
        labelInfoRange->setText("-\n-");
        plotDistance->setMouseRangeFocus(0,0);
        plotElevation->setMouseRangeFocus(0,0);
        plotSpeed->setMouseRangeFocus(0,0);
    }
}

void CDetailsTrk::slotShowPlots()
{
    if(checkProfile->isChecked())
    {
        plotElevation->show();
    }
    else
    {
        plotElevation->hide();
    }

    if(checkSpeed->isChecked())
    {
        plotSpeed->show();
    }
    else
    {
        plotSpeed->hide();
    }

    if(checkProgress->isChecked())
    {
        plotDistance->show();
    }
    else
    {
        plotDistance->hide();
    }
}

void CDetailsTrk::slotColorChanged(int idx)
{
    if(trk.getColorIdx() != idx)
    {
        trk.setColor(idx);
    }
}

void CDetailsTrk::slotChangeReadOnlyMode(bool on)
{
    trk.setReadOnlyMode(on);
    setupGui();
}


void CDetailsTrk::slotItemSelectionChanged()
{
    QTreeWidgetItem * item = treeWidget->currentItem();
    if(item != 0)
    {
        quint32 idx = item->text(eColNum).toUInt();
        trk.setMouseFocusByTotalIndex(idx, CGisItemTrk::eFocusMouseMove, "CDetailsTrk");
    }
}

void CDetailsTrk::slotLinkActivated(const QString& url)
{
    if(url == "name")
    {
        QString name = QInputDialog::getText(this, tr("Edit name..."), tr("Enter new track name."), QLineEdit::Normal, trk.getName());
        if(name.isEmpty())
        {
            return;
        }
        trk.setName(name);
        setupGui();
    }
}

void CDetailsTrk::slotLinkActivated(const QUrl& url)
{
    if(url.toString() == "comment")
    {
        CTextEditWidget dlg(this);
        dlg.setHtml(trk.getComment());
        if(dlg.exec() == QDialog::Accepted)
        {
            trk.setComment(dlg.getHtml());
        }
        setupGui();
    }
    else if(url.toString() == "description")
    {
        CTextEditWidget dlg(this);
        dlg.setHtml(trk.getDescription());
        if(dlg.exec() == QDialog::Accepted)
        {
            trk.setDescription(dlg.getHtml());
        }
        setupGui();
    }
    else if(url.toString() == "links")
    {
        QList<IGisItem::link_t> links = trk.getLinks();
        CLinksDialog dlg(links, this);
        if(dlg.exec() == QDialog::Accepted)
        {
            trk.setLinks(links);
        }
        setupGui();
    }
    else
    {
        QDesktopServices::openUrl(url);
    }
}

void CDetailsTrk::slotActivitySelected(bool checked)
{
    if(!checked)
    {
        if(QMessageBox::warning(this, tr("Reset activities..."), tr("This will remove all activities from the track. Proceed?"), QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok) != QMessageBox::Ok)
        {
            setupGui();
            return;
        }

        trk.setActivity(CGisItemTrk::trkpt_t::eActNone, tr("None"), "://icons/48x48/ActNone.png");
        return;
    }

    QObject * s = sender();
    bool ok = false;
    quint32 flag = s->property("flag").toUInt(&ok);
    if(ok)
    {
        trk.setActivity(flag, s->property("name").toString(), s->property("symbol").toString());
    }
}
