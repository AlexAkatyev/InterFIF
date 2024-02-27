#ifndef INTERFIF_H
#define INTERFIF_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class InterFIF; }
QT_END_NAMESPACE

class IniSettings;
class QAxObject;
struct VerifyRecord;

class InterFIF : public QWidget
{
  Q_OBJECT

public:
  InterFIF(QWidget *parent = nullptr);
  ~InterFIF();

private:
  Ui::InterFIF* _ui;

  IniSettings* _settings;
  int _mode;

  void setFsaFile();
  void setArshinFile();
  void selectProtocol();
  void route();
  std::vector<VerifyRecord> loadProtocol(QString protocol);
  void makeToFsa(std::vector<VerifyRecord> res);
  void makeToArshin(std::vector<VerifyRecord> res);
  QString getCellToString(QAxObject* sheet, int r, int c);
  QDate getCellToDate(QAxObject* sheet, int r, int c);
  void setStringToCell(QAxObject* sheet, int r, int c, QString text);
  QString publicName(QString realName);
  QString getSelectXMLFile(QString fName);
  void setMode();
  void getNumbersFromArshin(QString protocolFileName);

};
#endif // INTERFIF_H
