#include "networkproxy.h"

#include <QDebug>
#include <QMap>
#include <QProcess>
#include <QSettings>

QStringList NetworkProxyHelper::NETWORK_INTERFACES{"Wi-Fi", "Enternet"};

NetworkProxy NetworkProxyHelper::getSystemProxy() {
  NetworkProxy proxy;
  proxy.type = NetworkProxyType::DISABLED;
#if defined(Q_OS_WIN)
  QSettings internetSettings(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet "
    "Settings",
    QSettings::NativeFormat);
  if (internetSettings.contains("AutoConfigURL")) {
    proxy.type = NetworkProxyType::PAC_PROXY;
    proxy.url  = internetSettings.value("AutoConfigURL").toString();
  } else if (internetSettings.value("ProxyEnable").toInt() == 1 &&
             internetSettings.contains("ProxyServer")) {
    QString proxyServer = internetSettings.value("ProxyServer").toString();
    int colonIndex      = proxyServer.indexOf(':');
    proxy.type          = NetworkProxyType::SOCKS_PROXY;
    proxy.host          = proxyServer.left(colonIndex);
    proxy.port          = proxyServer.mid(colonIndex + 1).toInt();
  }
#elif defined(Q_OS_LINUX)
  QProcess p;
  QString httpProxyHost, socksProxyHost;
  int httpProxyPort, socksProxyPort;
  p.start("gsettings", QStringList() << "list-recursively"
                                     << "org.gnome.system.proxy");
  p.waitForFinished();
  QList<QByteArray> settings = p.readAllStandardOutput().split('\n');
  for (QByteArray s : settings) {
    if (s.startsWith("org.gnome.system.proxy mode")) {
      int qIndex        = s.indexOf('\'');
      QString proxyMode = s.mid(qIndex + 1, s.lastIndexOf('\'') - qIndex - 1);
      if (proxyMode == "none") {
        proxy.type = NetworkProxyType::DISABLED;
      } else if (proxyMode == "auto") {
        proxy.type = NetworkProxyType::PAC_PROXY;
      } else if (proxyMode == "manual") {
        proxy.type =
          NetworkProxyType::SOCKS_PROXY;  // or NetworkProxyType::HTTP_PROXY;
      }
    } else if (s.startsWith("org.gnome.system.proxy autoconfig-url")) {
      int qIndex = s.indexOf('\'');
      proxy.url  = s.mid(qIndex + 1, s.lastIndexOf('\'') - qIndex - 1);
    } else if (s.startsWith("org.gnome.system.proxy.http port")) {
      httpProxyPort = s.mid(33).toInt();
    } else if (s.startsWith("org.gnome.system.proxy.http host")) {
      int qIndex    = s.indexOf('\'');
      httpProxyHost = s.mid(qIndex + 1, s.lastIndexOf('\'') - qIndex - 1);
    } else if (s.startsWith("org.gnome.system.proxy.socks port")) {
      socksProxyPort = s.mid(34).toInt();
    } else if (s.startsWith("org.gnome.system.proxy.socks host")) {
      int qIndex     = s.indexOf('\'');
      socksProxyHost = s.mid(qIndex + 1, s.lastIndexOf('\'') - qIndex - 1);
    }
  }
  // Determine it is an HTTP Proxy or SOCKS Proxy
  if (proxy.type == NetworkProxyType::SOCKS_PROXY) {
    if (socksProxyHost.size() && socksProxyPort > 0) {
      proxy.type = NetworkProxyType::SOCKS_PROXY;
      proxy.host = socksProxyHost;
      proxy.port = socksProxyPort;
    } else if (httpProxyHost.size() && httpProxyPort > 0) {
      proxy.type = NetworkProxyType::HTTP_PROXY;
      proxy.host = httpProxyHost;
      proxy.port = httpProxyPort;
    } else {
      proxy.type = NetworkProxyType::DISABLED;
    }
  }
#elif defined(Q_OS_MAC)
  QString stdOutput, httpProxyHost, socksProxyHost;
  int httpProxyPort, socksProxyPort;
  for (QString ni : NETWORK_INTERFACES) {
    QProcess p;
    p.start("networksetup", QStringList() << "-getautoproxyurl" << ni);
    p.waitForFinished();
    stdOutput = p.readAllStandardOutput();
    if (!stdOutput.startsWith("** Error:")) {
      QStringList sl = stdOutput.split('\n');
      for (QString s : sl) {
        if (s.startsWith("Enabled: Yes")) {
          proxy.type = NetworkProxyType::PAC_PROXY;
        } else if (s.startsWith("URL: ")) {
          proxy.url = s.mid(5);
        }
      }
    }
    p.start("networksetup", QStringList() << "-getwebproxy" << ni);
    p.waitForFinished();
    stdOutput = p.readAllStandardOutput();
    if (!stdOutput.startsWith("** Error:")) {
      QStringList sl = stdOutput.split('\n');
      for (QString s : sl) {
        if (s.startsWith("Enabled: Yes")) {
          proxy.type = NetworkProxyType::HTTP_PROXY;
        } else if (s.startsWith("Server: ")) {
          httpProxyHost = s.mid(8);
        } else if (s.startsWith("Port: ")) {
          httpProxyPort = s.mid(6).toInt();
        }
      }
    }
    p.start("networksetup", QStringList() << "-getsocksfirewallproxy" << ni);
    p.waitForFinished();
    stdOutput = p.readAllStandardOutput();
    if (!stdOutput.startsWith("** Error:")) {
      QStringList sl = stdOutput.split('\n');
      for (QString s : sl) {
        if (s.startsWith("Enabled: Yes")) {
          proxy.type = NetworkProxyType::SOCKS_PROXY;
        } else if (s.startsWith("Server: ")) {
          socksProxyHost = s.mid(8);
        } else if (s.startsWith("Port: ")) {
          socksProxyPort = s.mid(6).toInt();
        }
      }
    }
    if (proxy.type == NetworkProxyType::HTTP_PROXY) {
      proxy.host = httpProxyHost;
      proxy.port = httpProxyPort;
    } else if (proxy.type == NetworkProxyType::SOCKS_PROXY) {
      proxy.host = socksProxyHost;
      proxy.port = socksProxyPort;
    }
  }
#endif
  return proxy;
}

void NetworkProxyHelper::setSystemProxy(NetworkProxy proxy) {
#if defined(Q_OS_WIN)
  QSettings internetSettings(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet "
    "Settings",
    QSettings::NativeFormat);
  if (proxy.type == NetworkProxyType::PAC_PROXY) {
    internetSettings.setValue("AutoConfigURL", proxy.url);
  } else if (proxy.type == NetworkProxyType::HTTP_PROXY ||
             proxy.type == NetworkProxyType::SOCKS_PROXY) {
    internetSettings.setValue("ProxyEnable", 1);
    internetSettings.setValue(
      "ProxyServer",
      QString("%1:%2").arg(proxy.host, QString::number(proxy.port)));
  }
#elif defined(Q_OS_LINUX)
  // Support GNOME only
  QMap<NetworkProxyType, QList<QStringList> > settingCommands{
    {NetworkProxyType::PAC_PROXY,
     {{"set", "org.gnome.system.proxy", "mode", "auto"},
      {"set", "org.gnome.system.proxy", "autoconfig-url", proxy.url}}},
    {NetworkProxyType::SOCKS_PROXY,
     {{"set", "org.gnome.system.proxy", "mode", "manual"},
      {"set", "org.gnome.system.proxy.socks", "host", proxy.host},
      {"set", "org.gnome.system.proxy.socks", "port",
       QString::number(proxy.port)}}},
    {NetworkProxyType::HTTP_PROXY,
     {{"set", "org.gnome.system.proxy", "mode", "manual"},
      {"set", "org.gnome.system.proxy.http", "host", proxy.host},
      {"set", "org.gnome.system.proxy.http", "port",
       QString::number(proxy.port)}}},
  };
  QProcess p;
  QList<QStringList> commands = settingCommands[proxy.type];
  for (QStringList c : commands) {
    p.start("gsettings", c);
    p.waitForFinished();
  }
#elif defined(Q_OS_MAC)
  QMap<NetworkProxyType, QList<QStringList> > settingCommands{
    {NetworkProxyType::PAC_PROXY,
     {{"-setautoproxyurl", proxy.url}, {"-setautoproxystate", "on"}}},
    {NetworkProxyType::SOCKS_PROXY,
     {{"-setsocksfirewallproxy", proxy.host, QString::number(proxy.port)},
      {"-setsocksfirewallproxystate", "on"}}},
    {NetworkProxyType::HTTP_PROXY,
     {{"-setwebproxy", proxy.host, QString::number(proxy.port)},
      {"-setwebproxystate", "on"}}},
  };
  for (QString ni : NETWORK_INTERFACES) {
    QProcess p;
    QList<QStringList> commands = settingCommands[proxy.type];
    for (QStringList c : commands) {
      c.insert(1, ni);
      p.start("networksetup", c);
      p.waitForFinished();
    }
  }
#endif
}

void NetworkProxyHelper::resetSystemProxy() {
#if defined(Q_OS_WIN)
  QSettings internetSettings(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet "
    "Settings",
    QSettings::NativeFormat);
  internetSettings.setValue("ProxyEnable", 0);
  internetSettings.remove("AutoConfigURL");
  internetSettings.remove("ProxyServer");
#elif defined(Q_OS_LINUX)
  QProcess p;
  p.start("gsettings", QStringList() << "set"
                                     << "org.gnome.system.proxy"
                                     << "mode"
                                     << "none");
  p.waitForFinished();
  p.start("gsettings", QStringList()
                         << "set"
                         << "org.gnome.system.proxy"
                         << "ignore-hosts"
                         << "['localhost', '127.0.0.0/8', '::1', '10.0.0.0/8', "
                            "'172.16.0.0/12', '192.168.0.0/16']");
  p.waitForFinished();
  p.start("gsettings", QStringList() << "set"
                                     << "org.gnome.system.proxy.http"
                                     << "enabled"
                                     << "false");
  p.waitForFinished();
  p.start("gsettings", QStringList() << "set"
                                     << "org.gnome.system.proxy.http"
                                     << "host"
                                     << "");
  p.waitForFinished();
  p.start("gsettings", QStringList() << "set"
                                     << "org.gnome.system.proxy.http"
                                     << "port"
                                     << "0");
  p.waitForFinished();
  p.start("gsettings", QStringList() << "set"
                                     << "org.gnome.system.proxy.socks"
                                     << "host"
                                     << "");
  p.waitForFinished();
  p.start("gsettings", QStringList() << "set"
                                     << "org.gnome.system.proxy.socks"
                                     << "port"
                                     << "0");
  p.waitForFinished();
#elif defined(Q_OS_MAC)
  for (QString ni : NETWORK_INTERFACES) {
    QProcess p;
    p.start("networksetup", QStringList()
                              << "-setautoproxystate" << ni << "off");
    p.waitForFinished();
    p.start("networksetup", QStringList()
                              << "-setwebproxystate" << ni << "off");
    p.waitForFinished();
    p.start("networksetup", QStringList()
                              << "-setsocksfirewallproxystate" << ni << "off");
    p.waitForFinished();
  }
#endif
}
