#include <QSettings>

#include "inisettings.h"

IniSettings::IniSettings(QObject *parent) : QObject(parent)
  , _settings(new QSettings("interfif.ini", QSettings::IniFormat, this))
{
  QStringList keys = _settings->allKeys();
  for (auto& key : keys)
  {
    QVariant value = _settings->value(key);
    if (value != QVariant())
      _mapset.insert(std::pair<QString, QVariant>(key, value));
  }
}


IniSettings::~IniSettings()
{
  for (auto& setting : _mapset)
    _settings->setValue(setting.first, setting.second);
  deleteLater();
}


QVariant IniSettings::ReadValue(QString key)
{
  std::map<QString, QVariant>::iterator it = _mapset.find(key);
  return it == _mapset.end() ? defaultSettings(key): it->second;
}


void IniSettings::SetValue(QString key, QVariant value)
{
  _mapset[key] = value;
}


QVariant IniSettings::defaultSettings(QString key)
{
  QVariant result = QVariant();
  std::map<QString, QVariant> defmap =
  {
    {IniFsaFile, "c:/toFSA.xml"},
    {IniArshinFile, "c:/toArshin.xml"},
    {IniInputFile, "c:/1.xlsx"},
  };
  std::map<QString, QVariant>::iterator it = defmap.find(key);
  if (it != defmap.end())
    result = it->second;
  return result;
}

