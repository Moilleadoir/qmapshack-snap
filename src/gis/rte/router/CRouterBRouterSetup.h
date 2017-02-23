/**********************************************************************************************
    Copyright (C) 2017 Norbert Truchsess norbert.truchsess@t-online.de

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

#ifndef CROUTERBROUTERSETUP_H
#define CROUTERBROUTERSETUP_H

#include <QtCore>
#include "setup/IAppSetup.h"
#include <QWebPage>

class CRouterBRouterSetup : public QObject
{
    Q_OBJECT
public:
    CRouterBRouterSetup();
    ~CRouterBRouterSetup();
    enum mode_e { ModeLocal, ModeOnline };
    struct tile_s { QPoint tile; QDateTime date; qreal size; };
    bool expertMode;
    mode_e installMode;
    QString onlineWebUrl;
    QString onlineServiceUrl;
    QString onlineProfilesUrl;
    QStringList onlineProfiles;
    QStringList onlineProfilesAvailable;
    QString localDir;
    QString localProfileDir;
    QString localSegmentsDir;
    QStringList localProfiles;
    QString localHost;
    QString localPort;
    QString binariesUrl;
    QString segmentsUrl;

    void load();
    void save();

    const QString getProfileContent(const int index);
    const QString getProfileContent(const QString profile);
    const QStringList getProfiles();
    const QDir getProfileDir();

    void readProfiles();
    void loadOnlineConfig();
    const QString getOnlineProfileContent(const QString profile);
    void installOnlineProfile(const QString profile);

    void initializeTiles();
    void installOnlineTile(const QPoint tile);
    void deleteTile(const QPoint tile);

    QVector<QPoint> getInvalidTiles();
    QVector<QPoint> getCurrentTiles();
    QVector<QPoint> getOutdatedTiles();
    QVector<QPoint> getOnlineTilesAvailable();
    QVector<QPoint> getOutstandingTiles();

public slots:
    void slotLoadOnlineTilesRequestFinished();
    void slotLoadOnlineTileDownloadFinished(QNetworkReply* reply);
    void slotLoadOnlineTileDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void slotLoadOnlineTileDownloadReadReady();

signals:
    void tilesLocalChanged();
    void tilesDownloadProgress(qint64 received, qint64 total);

private:
    const bool defaultExpertMode = false;
    const mode_e defaultInstallMode = ModeOnline;
    const QString defaultOnlineWebUrl = "http://brouter.de/brouter-web/";
    const QString defaultOnlineServiceUrl = "http://h2096617.stratoserver.net:443";
    const QString defaultOnlineProfilesUrl = "http://brouter.de/brouter/profiles2/";
    const QString defaultLocalDir = ".";
    const QString defaultLocalProfileDir = "profiles2";
    const QString defaultLocalSegmentsDir = "segments4";
    const QString defaultLocalHost = "127.0.0.1";
    const QString defaultLocalPort = "17777";
    const QString defaultBinariesUrl = "http://brouter.de/brouter_bin/";
    const QString defaultSegmentsUrl = "http://brouter.de/brouter/segments4/";

    const QString onlineCacheDir = "BRouter";

    void readProfiles(mode_e mode);
    const QString getProfileContent(mode_e mode, QString profile);
    const QDir getProfileDir(mode_e mode);
    const QByteArray loadOnlineProfile(const QString profile);

    const mode_e modeFromString(QString mode);
    const QString stringFromMode(mode_e mode);

    const QPoint tileFromFileName(QString fileName);
    const QString fileNameFromTile(QPoint tile);
    QFile * findFileForReply(QNetworkReply * reply);

    void readTiles();

    QVector<tile_s> onlineTiles;
    QVector<QPoint> invalidTiles;
    QVector<QPoint> oldTiles;
    QVector<QPoint> currentTiles;
    QVector<QPoint> outstandingTiles;

    QWebPage tilesWebPage;
    QNetworkAccessManager * tilesDownloadManager;
    QVector<QNetworkReply*> tilesDownloadManagerReplies;
    QVector<QFile*> tilesDownloadManagerFiles;

};

#endif
