#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QTextStream>
#include <QTranslator>
#include <QtGlobal>

#include "appproxy.h"
#include "constants.h"
#include "runguard.h"

void messageHandler(QtMsgType msgType,
                    const QMessageLogContext &context,
                    const QString &msg) {
  Q_UNUSED(context);

  QString dt = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
  QString logMessage("%1 %2 v2ray-desktop: %3");
  QString msgTypeStr;
  switch (msgType) {
    case QtDebugMsg: msgTypeStr = "[Debug]"; break;
    case QtInfoMsg: msgTypeStr = "[Info]"; break;
    case QtWarningMsg: msgTypeStr = "[Warning]"; break;
    case QtCriticalMsg: msgTypeStr = "[Critical]"; break;
    case QtFatalMsg: msgTypeStr = "[Fatal]"; break;
    default: break;
  }
  if (msgType != QtDebugMsg) {
    QFile logFile(Configurator::getAppLogFilePath());
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream logTextStream(&logFile);
    logTextStream << logMessage.arg(dt, msgTypeStr, msg) << endl;
  } else {
    QTextStream(stdout) << logMessage.arg(dt, msgTypeStr, msg) << endl;
  }
}

int main(int argc, char *argv[]) {
  QCoreApplication::setApplicationName(APP_NAME);
  QCoreApplication::setApplicationVersion(
    QString("v%1.%2.%3")
      .arg(QString::number(APP_VERSION_MAJOR),
           QString::number(APP_VERSION_MINOR),
           QString::number(APP_VERSION_PATCH)));
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication app(argc, argv);
  // Make sure there is no other instance running
  RunGuard runGuard(APP_NAME);
  if (!runGuard.tryToRun()) {
    QTextStream(stderr)
      << QString("There is another %1 instance running!\n").arg(APP_NAME);
    return 127;
  }

  // Set up QML and AppProxy
#if defined(Q_OS_WIN)
  QFont font("Microsoft YaHei", 10);
  app.setFont(font);
#endif
  qmlRegisterSingletonType<AppProxy>(
    "com.v2ray.desktop.AppProxy", APP_VERSION_MAJOR, APP_VERSION_MINOR,
    "AppProxy", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
      Q_UNUSED(engine)
      Q_UNUSED(scriptEngine)
      AppProxy *appProxy = new AppProxy();
      return appProxy;
    });

  // Set up logging
  qInstallMessageHandler(messageHandler);

  // Set up the application
  QQmlApplicationEngine engine;
  const QUrl url(QStringLiteral("qrc:/ui/main.qml"));
  QObject::connect(
    &engine, &QQmlApplicationEngine::objectCreated, &app,
    [url](QObject *obj, const QUrl &objUrl) {
      if (!obj && url == objUrl) {
        QCoreApplication::exit(-1);
      }
    },
    Qt::QueuedConnection);
  engine.load(url);

  return app.exec();
}
