#ifndef APPPROXY_H
#define APPPROXY_H

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QThread>
#include <QTranslator>

#include "appproxyworker.h"
#include "configurator.h"
#include "pacserver.h"
#include "v2raycore.h"

class AppProxy : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(AppProxy)

 public:
  AppProxy(QObject* parent = 0);
  ~AppProxy();

 signals:
  void getServerLatencyStarted(QJsonArray servers);
  void getGfwListStarted(QString gfwListUrl, QNetworkProxy proxy);
  void getNetworkStatusStarted(QMap<QString, bool> urls, QNetworkProxy proxy);
  void getSubscriptionServersStarted(QString url, QNetworkProxy proxy);

  void appVersionReady(QString appVersion);
  void v2RayCoreVersionReady(QString v2RayCoreVersion);
  void operatingSystemReady(QString operatingSystem);
  void v2RayCoreStatusReady(QString v2RayCoreStatus);
  void v2RayRunningStatusChanging(bool isChanged);
  void networkStatusReady(QString networkStatus);
  void proxySettingsReady(QString proxySettings);
  void appConfigReady(QString appConfig);
  void appConfigError(QString errorMessage);
  void appConfigChanged();
  void logsReady(QString logs);
  void proxyModeReady(QString proxyMode);
  void proxyModeChanged(QString proxyMode);
  void gfwListUpdated(QString gfwListUpdateTime);
  void serversReady(QString servers);
  void serverDInfoReady(QString server);
  void serverLatencyReady(QString latency);
  void addServerError(QString errorMessage);
  void serversChanged();

 public slots:
  void getAppVersion();
  void getV2RayCoreVersion();
  void getOperatingSystem();
  void getV2RayCoreStatus();
  void setV2RayCoreRunning(bool expectedRunning);
  void getNetworkStatus();
  void getAppConfig();
  void setAppConfig(QString configString);
  void setAutoStart(bool autoStart);
  void getLogs();
  void clearLogs();
  void getProxySettings();
  void setSystemProxyMode(QString proxyMode = "");
  void updateGfwList(QString gfwListUrl);
  void getServers();
  void getServer(QString serverName, bool forDuplicate = false);
  void getServerLatency(QString serverName = "");
  void setServerConnection(QString serverName, bool connected);
  void addV2RayServer(QString configString);
  void addShadowsocksServer(QString configString);
  void addSubscriptionUrl(QString subsriptionUrl);
  void addServerConfigFile(QString configFilePath);
  void editServer(QString serverName, QString protocol, QString configString);
  void removeServer(QString serverName);
  void scanQrCodeScreen();
  bool retranslate(QString language = "");

 private slots:
  void returnServerLatency(QMap<QString, QVariant> latency);
  void returnGfwList(QByteArray gfwList);
  void returnNetworkAccessiblity(QMap<QString, bool> accessible);
  void addSubsriptionServers(QString subsriptionServers,
                             QString subsriptionUrl = "");

 private:
  V2RayCore& v2ray;
  Configurator& configurator;
  QJsonObject serverLatency;
  PacServer pacServer;
  AppProxyWorker* worker = new AppProxyWorker();
  QThread workerThread;
  QTranslator translator;

  QJsonObject getPrettyV2RayConfig(const QJsonObject& serverConfig);
  QJsonObject getV2RayServerFromUrl(QString serverUrl,
                                    QString subscriptionUrl = "");
  QJsonObject getV2RayStreamSettingsConfig(const QJsonObject& serverConfig);
  QJsonArray getRandomUserAgents(int n);
  QJsonObject getPrettyShadowsocksConfig(const QJsonObject& serverConfig);
  QJsonObject getShadowsocksServerFromUrl(QString serverUrl,
                                          QString subscriptionUrl = "");
  QNetworkProxy getQProxy();
};

#endif  // APPPROXY_H
