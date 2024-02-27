#ifndef INISETTINGS_H
#define INISETTINGS_H

#include <QObject>

class QSettings;

const QString IniFsaFile = "IniFsaFile";
const QString IniArshinFile = "IniArshinFile";
const QString IniInputFile = "IniInputFile";

class IniSettings : public QObject
{
  Q_OBJECT
public:
  IniSettings(QObject *parent = nullptr);
  ~IniSettings();
  QVariant ReadValue(QString key);
  void SetValue(QString key, QVariant value);

private:
  QVariant defaultSettings(QString key);

  std::map<QString, QVariant> _mapset;
  QSettings* _settings;
};

#endif // INISETTINGS_H
