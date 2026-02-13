#include <QFileDialog>
#include <QDate>
#include <QAxObject>
#include <QXmlStreamWriter>
#include <QProcess>

#include "interfif.h"
#include "ui_interfif.h"
#include "inisettings.h"
#include "infowindow.h"

const bool DEBUG_MODE = false;

struct VerifyRecord
{
  QString NumberPi;
  QString GRSI;
  QString Modification;
  QString FactoryNumber;
  QString Owner;
  QDate VerifyData;
  QDate ValidityData;
  QString InstructionName;
  QString VerifyType;
  bool Valid;
  QString Reason; // причина
  QString FIO;
  QString Etalon;
  QString SI1Name;
  QString SI1Number;
  QString SI2Name;
  QString SI2Number;
  QString SI3Name;
  QString SI3Number;
  QString SI4Name;
  QString SI4Number;
  QString Temperature;
  QString Pressure;
  QString Humidity;
  QString FaceType;
  QString GUID;
  QString Contact;
  QString RegNumber;
  QDate VerifyDataFSA;
  QDate ValidityDataFSA;
  QString TypeSI;
  bool Result;
  QString F;
  QString I;
  QString O;
  QString SNILS;
  QString ArshinNumber;
  QString AdditionInfo;
};


InterFIF::InterFIF(QWidget *parent)
  : QWidget(parent)
  , _ui(new Ui::InterFIF)
  , _settings(new IniSettings(this))
  , _mode(0)
{
  _ui->setupUi(this);
  _ui->ProtokolName->setText(_settings->ReadValue(IniInputFile).toString());
  _ui->FsaName->setText(_settings->ReadValue(IniFsaFile).toString());
  _ui->ArshinName->setText(_settings->ReadValue(IniArshinFile).toString());
  connect(_ui->Exiter, &QAbstractButton::released, this, &InterFIF::close);
  connect(_ui->PointerFsaFile, &QAbstractButton::released, this, &InterFIF::setFsaFile);
  connect(_ui->PointerArshinFile, &QAbstractButton::released, this, &InterFIF::setArshinFile);
  connect(_ui->SelectorProtocol, &QAbstractButton::released, this, &InterFIF::selectProtocol);
  connect(_ui->SelectArshin, &QAbstractButton::released, this, &InterFIF::setMode);
  connect(_ui->SelectFSA, &QAbstractButton::released, this, &InterFIF::setMode);
  connect(_ui->SelectLoadFromArshin, &QAbstractButton::released, this, &InterFIF::setMode);
  connect(_ui->pushRun, &QAbstractButton::released, this, &InterFIF::route);
  setMode();
  this->setWindowTitle("InterFIF v1.5");

  InfoWindow* info = new InfoWindow(this);
  connect(_ui->Informator, &QAbstractButton::released, info, &InfoWindow::show);
}

InterFIF::~InterFIF()
{
  _settings->SetValue(IniInputFile, _ui->ProtokolName->text());
  _settings->SetValue(IniFsaFile, _ui->FsaName->text());
  _settings->SetValue(IniArshinFile, _ui->ArshinName->text());
  delete _ui;
}


void InterFIF::setMode()
{
  if (_ui->SelectArshin->isChecked())
    _mode = 0;
  else if (_ui->SelectFSA->isChecked())
    _mode = 1;
  else
    _mode = 2;
  _ui->ArshinName->setEnabled(_ui->SelectArshin->isChecked());
  _ui->PointerArshinFile->setEnabled(_ui->SelectArshin->isChecked());
  _ui->FsaName->setEnabled(_ui->SelectFSA->isChecked());
  _ui->PointerFsaFile->setEnabled(_ui->SelectFSA->isChecked());
}


QString InterFIF::getSelectXMLFile(QString fName)
{
  QFileDialog dialog(this,
                     "Выберите файл",
                     fName);
  dialog.setNameFilter("XML файл (*.xml)");
  dialog.setFileMode(QFileDialog::AnyFile);
  if (dialog.exec())
    return dialog.selectedFiles().at(0);
  else
    return fName;
}

void InterFIF::setFsaFile()
{
  _ui->FsaName->setText(getSelectXMLFile(_ui->FsaName->text()));
}


void InterFIF::setArshinFile()
{
  _ui->ArshinName->setText(getSelectXMLFile(_ui->ArshinName->text()));
}


void InterFIF::selectProtocol()
{
  QFileDialog dialog(this,
                     "Выберите протокол",
                     _settings->ReadValue(IniInputFile).toString());
  dialog.setNameFilter("Ексель файл (*.xls *.xlsx)");
  dialog.setFileMode(QFileDialog::ExistingFile);
  if (dialog.exec())
    _ui->ProtokolName->setText(dialog.selectedFiles().at(0));
}


void InterFIF::route()
{
  _ui->Logger->clear();
  QString protocol = _ui->ProtokolName->text();
  if (protocol.isEmpty())
  {
    _ui->Logger->append("Файл протокола не указан. Выберите существующий файл\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  if (!QFile::exists(protocol))
  {
    _ui->Logger->append("Файл " + protocol + " не существует. Выберите существующий файл\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  _ui->Logger->append("Выбран файл " + protocol + ".\n");
  qApp->processEvents(); // обрабатываем события, для вывода в лог

  switch (_mode)
  {
  case 0:
    _ui->Logger->append("Начинаю создание файла для загрузки в ФГИС Аршин.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    makeToArshin(loadProtocol(protocol));
    break;
  case 1:
    _ui->Logger->append("Начинаю создание файла для загрузки в ЕИС ФСА.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    makeToFsa(loadProtocol(protocol));
    break;
  case 2:
    _ui->Logger->append("Загрузка производится с помощью скрипта Python.\n");
    getNumbersFromArshin(protocol);
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    break;
  default:
    break;
  }

  _ui->Logger->append("Обработка закончена.\n");
}


QString InterFIF::getCellToString(QAxObject* sheet, int r, int c)
{
  return sheet->querySubObject("Cells(QVariant,QVariant)", r, c)->property("Text").toString().trimmed();
}


QDate InterFIF::getCellToDate(QAxObject* sheet, int r, int c)
{
  return QDate::fromString(getCellToString(sheet, r, c), "dd.MM.yyyy");
}


std::vector<VerifyRecord> InterFIF::loadProtocol(QString protocol)
{
  std::vector<VerifyRecord> result;
  if (DEBUG_MODE)
  {
    VerifyRecord vr;
    vr.NumberPi = "";
    vr.GRSI = "111-11";
    vr.Modification = "modification";
    vr.FactoryNumber = "factoryNumber";
    vr.Owner = "Owner";
    vr.VerifyData = QDate::currentDate();
    vr.ValidityData = QDate::currentDate();
    vr.InstructionName = "InstructionName";
    vr.VerifyType = "VerifyType";
    vr.Valid = true;
    vr.Reason = "reason";
    vr.FIO = "FIO";
    vr.Etalon = "etalon";
    vr.SI1Name = "si1name";
    vr.SI1Number = "si1number";
    vr.SI2Name = "si2name";
    vr.SI2Number = "si2number";
    vr.SI3Name = "si3name";
    vr.SI3Number = "si3number";
    vr.SI4Name = "si4name";
    vr.SI4Number = "si4number";
    vr.Temperature = "temperature";
    vr.Pressure = "pressure";
    vr.Humidity = "humidity";
    vr.FaceType = "facetype";
    vr.GUID = "guid";
    vr.Contact = "contact";
    vr.RegNumber = "regnumber";
    vr.VerifyDataFSA = QDate::currentDate();
    vr.ValidityDataFSA = QDate::currentDate();
    vr.TypeSI = "typesi";
    vr.Result = true;
    vr.F = "f";
    vr.I = "i";
    vr.O = "o";
    vr.SNILS = "snils";
    vr.ArshinNumber = "arshinnumber";
    vr.AdditionInfo = "additioninfo";
    result.push_back(vr);
  }
  else if (!protocol.isEmpty())
  {
    _ui->Logger->append("Читаю файл протокола.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    // получаем указатель на Excel
    QAxObject* mExcel = new QAxObject("Excel.Application",this);
    mExcel->setProperty("DisplayAlerts", 0); // Игнорировать сообщения
    // на книги
    QAxObject* workbooks = mExcel->querySubObject("Workbooks");
    // на директорию, откуда грузить книг
    QAxObject* workbook = workbooks->querySubObject("Open(const QString&)", protocol);
    // на листы
    QAxObject* mSheets = workbook->querySubObject("Sheets");
    // указываем, какой лист выбрать
    QAxObject* statSheet = mSheets->querySubObject("Item(const QVariant&)", QVariant("Лист1"));
    // получение указателя на ячейку [row][col] ((!)нумерация с единицы)
    //QAxObject* cell = statSheet->querySubObject("Cells(QVariant,QVariant)", 1, 1);
    for (int row = 2; row < 65536; ++row)
    {
      int col = 1;
      QString text = getCellToString(statSheet, row, col++);
      if (text.isEmpty())
        break;
      VerifyRecord vr;
      vr.NumberPi = text;
      vr.GRSI = getCellToString(statSheet, row, col++);
      vr.Modification = getCellToString(statSheet, row, col++);
      vr.FactoryNumber = getCellToString(statSheet, row, col++);
      vr.Owner = getCellToString(statSheet, row, col++);
      vr.VerifyData = getCellToDate(statSheet, row, col++);
      vr.ValidityData = getCellToDate(statSheet, row, col++);
      vr.InstructionName = getCellToString(statSheet, row, col++);
      vr.VerifyType = getCellToString(statSheet, row, col++);
      vr.Valid = getCellToString(statSheet, row, col++).toLower() == "пригодно";
      vr.Reason = getCellToString(statSheet, row, col++);
      vr.FIO = getCellToString(statSheet, row, col++);
      vr.Etalon = getCellToString(statSheet, row, col++);
      vr.SI1Name = getCellToString(statSheet, row, col++);
      vr.SI1Number = getCellToString(statSheet, row, col++);
      vr.SI2Name = getCellToString(statSheet, row, col++);
      vr.SI2Number = getCellToString(statSheet, row, col++);
      vr.SI3Name = getCellToString(statSheet, row, col++);
      vr.SI3Number = getCellToString(statSheet, row, col++);
      vr.SI4Name = getCellToString(statSheet, row, col++);
      vr.SI4Number = getCellToString(statSheet, row, col++);
      vr.Temperature = getCellToString(statSheet, row, col++);
      vr.Pressure = getCellToString(statSheet, row, col++);
      vr.Humidity = getCellToString(statSheet, row, col++);
      vr.FaceType = getCellToString(statSheet, row, col++);
      vr.GUID = getCellToString(statSheet, row, col++);
      vr.Contact = getCellToString(statSheet, row, col++);
      vr.RegNumber = getCellToString(statSheet, row, col++);
      vr.VerifyDataFSA = getCellToDate(statSheet, row, col++);
      vr.ValidityDataFSA = getCellToDate(statSheet, row, col++);
      vr.TypeSI = getCellToString(statSheet, row, col++);
      vr.Result = getCellToString(statSheet, row, col++).toLower() == "годен";
      vr.F = getCellToString(statSheet, row, col++);
      vr.I = getCellToString(statSheet, row, col++);
      vr.O = getCellToString(statSheet, row, col++);
      vr.SNILS = getCellToString(statSheet, row, col++);
      vr.ArshinNumber = getCellToString(statSheet, row, col++);
      vr.AdditionInfo = getCellToString(statSheet, row, col++);
      result.push_back(vr);
    }
    // освобождение памяти
    delete statSheet;
    delete mSheets;
    workbook->dynamicCall("Save()");
    delete workbook;
    //закрываем книги
    delete workbooks;
    //закрываем Excel
    mExcel->dynamicCall("Quit()");
    mExcel->setProperty("DisplayAlerts", 1);
    delete mExcel;
    _ui->Logger->append("Файл протокола прочитан.\n");
  }
  _ui->Logger->append("Прочитано " + QString::number(result.size()) + " записей.\n");
  qApp->processEvents(); // обрабатываем события, для вывода в лог
  return result;
}


void InterFIF::makeToFsa(std::vector<VerifyRecord> recs)
{
  QString newFile = _ui->FsaName->text();
  if (newFile.isEmpty())
  {
    _ui->Logger->append("Не указан путь файла для загрузки в ЕИС ФСА.\n");
    _ui->Logger->append("Файл для загрузки в ЕИС ФСА не создан.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  if (recs.size() == 0)
  {
    _ui->Logger->append("Отсутствуют записи для отправки в ФСА.\n");
    _ui->Logger->append("Файл для загрузки в ЕИС ФСА не создан.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  _ui->Logger->append("Создаю файл для загрузки в ЕИС ФСА.\n");
  qApp->processEvents(); // обрабатываем события, для вывода в лог
  if (QFile::exists(newFile))
    QFile::remove(newFile);

  QFile* file = new QFile(newFile);
  if (!file->open(QIODevice::WriteOnly))
  {
    _ui->Logger->append("Не удалось создать файл для загрузки в ЕИС ФСА.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  QXmlStreamWriter xmlWriter(file);
  xmlWriter.setAutoFormatting(true);
  xmlWriter.writeStartDocument();
  //Message xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  QString url = "http://www.w3.org/2001/XMLSchema-instance";
  xmlWriter.writeStartElement("Message");
  xmlWriter.writeAttribute("xmlns:xsi", url);
  xmlWriter.writeStartElement("VerificationMeasuringInstrumentData");
  for (auto& rec : recs)
  {
    xmlWriter.writeStartElement("VerificationMeasuringInstrument");
    xmlWriter.writeTextElement("NumberVerification", rec.ArshinNumber);
    xmlWriter.writeTextElement("DateVerification", rec.VerifyData.toString("yyyy-MM-dd"));
    if (rec.Valid)
      xmlWriter.writeTextElement("DateEndVerification", rec.ValidityData.toString("yyyy-MM-dd"));
    xmlWriter.writeTextElement("TypeMeasuringInstrument", rec.TypeSI);
    xmlWriter.writeStartElement("ApprovedEmployees");
    xmlWriter.writeStartElement("Name");
    xmlWriter.writeTextElement("Last", rec.F);
    xmlWriter.writeTextElement("First", rec.I);
    xmlWriter.writeEndElement(); // Name
    xmlWriter.writeTextElement("SNILS", rec.SNILS);
    xmlWriter.writeEndElement(); // ApprovedEmployees
    xmlWriter.writeTextElement("ResultVerification", rec.Valid ? "1" : "2");
    xmlWriter.writeEndElement(); // VerificationMeasuringInstrument
  }
  xmlWriter.writeEndElement(); // VerificationMeasuringInstrumentData
  xmlWriter.writeTextElement("SaveMethod", "2");
  xmlWriter.writeEndElement(); // Message
  xmlWriter.writeEndDocument();

  file->close();

  _ui->Logger->append("Создан файл для загрузки в ЕИС ФСА " + newFile + ".\n");
  qApp->processEvents(); // обрабатываем события, для вывода в лог
}


void InterFIF::setStringToCell(QAxObject* sheet, int r, int c, QString text)
{
  sheet->querySubObject("Cells(int,int)", r, c)->dynamicCall("SetValue(const QString&)", text);
}


void InterFIF::makeToArshin(std::vector<VerifyRecord> recs)
{
  bool error = false;
  QString newFile = _ui->ArshinName->text();
  if (newFile.isEmpty())
  {
    _ui->Logger->append("Не указан файл для загрузки в ФГИС Аршин.\n");
    _ui->Logger->append("Файл для загрузки в ФГИС Аршин не создан.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  if (recs.size() == 0)
  {
    _ui->Logger->append("Отсутствуют записи для отправки в ФГИС Аршин.\n");
    _ui->Logger->append("Файл для загрузки в ФГИС Аршин не создан.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  _ui->Logger->append("Создаю файл для загрузки в ФГИС Аршин.\n");
  qApp->processEvents(); // обрабатываем события, для вывода в лог

  if (QFile::exists(newFile))
    QFile::remove(newFile);

  QFile* file = new QFile(newFile);
  if (!file->open(QIODevice::WriteOnly))
  {
    _ui->Logger->append("Не удалось создать файл для загрузки в ФГИС Аршин.\n");
    qApp->processEvents(); // обрабатываем события, для вывода в лог
    return;
  }
  QXmlStreamWriter xmlWriter(file);
  xmlWriter.setAutoFormatting(true);
  xmlWriter.writeStartDocument();
  QString url = "urn://fgis-arshin.gost.ru/module-verifications/import/2020-06-19";
  xmlWriter.writeNamespace(url, "gost");
  xmlWriter.writeStartElement(url, "application");
  int countPassed = 0;
  int countFailed = 0;
  int countAll = 0;
  for (auto rec : recs)
  {
    ++countAll;
    xmlWriter.writeStartElement("gost:result");
    xmlWriter.writeStartElement("gost:miInfo");
    xmlWriter.writeStartElement("gost:singleMI");
    xmlWriter.writeTextElement("gost:mitypeNumber", rec.GRSI);
    xmlWriter.writeTextElement("gost:manufactureNum", rec.FactoryNumber);
    xmlWriter.writeTextElement("gost:modification", rec.Modification.isEmpty() ? "-" : rec.Modification);
    xmlWriter.writeEndElement(); // singleMI
    xmlWriter.writeEndElement(); // miInfo
    xmlWriter.writeTextElement("gost:signCipher", "ДЮЮ");
    xmlWriter.writeTextElement("gost:miOwner", publicName(rec.Owner));
    xmlWriter.writeTextElement("gost:vrfDate", rec.VerifyData.toString("yyyy-MM-dd+04:00"));
    if (rec.Valid)
      xmlWriter.writeTextElement("gost:validDate", rec.ValidityData.toString("yyyy-MM-dd+04:00"));
    xmlWriter.writeTextElement("gost:type", "2");  // Всегда периодическая
    xmlWriter.writeTextElement("gost:calibration", "false"); // Никакой калибровки
    if (rec.Valid)
    {
      ++countPassed;
      xmlWriter.writeStartElement("gost:applicable");
      xmlWriter.writeTextElement("gost:signPass", "false");
      xmlWriter.writeTextElement("gost:signMi", "false");
      xmlWriter.writeEndElement(); // applicable
    }
    else
    {
      ++countFailed;
      xmlWriter.writeStartElement("gost:inapplicable");
      xmlWriter.writeTextElement("gost:reasons", rec.Reason.isEmpty()
                                                 ? "Не соответствует требованиям методики поверки"
                                                 : rec.Reason);
      xmlWriter.writeEndElement(); // inapplicable
    }
    xmlWriter.writeTextElement("gost:docTitle", rec.InstructionName);
    xmlWriter.writeTextElement("gost:metrologist", rec.FIO);
    xmlWriter.writeStartElement("gost:means");
    xmlWriter.writeStartElement("gost:mieta");
    QStringList sl = rec.Etalon.split(" ");
    if (sl.size() == 0)
    {
      error = true;
      _ui->Logger->append("Обнаружена ошибка : Не удалось определить эталон в строке " + rec.NumberPi + ".\n");
      _ui->Logger->append("Пример записи: 60661.15.3Р.00535679 60661-15 Установки поверочные Эталон 3-го разряда\n");
      _ui->Logger->append("Пример записи: 60661.15.3Р.00535679\n");
      _ui->Logger->append("Важно наличие конца строки или пробела после номера эталона.\n");
      qApp->processEvents(); // обрабатываем события, для вывода в лог
    }
    xmlWriter.writeTextElement("gost:number", sl.size() > 0 ? sl.at(0) : " ");
    xmlWriter.writeEndElement(); // mieta
    std::vector<std::pair<QString, QString>> mis;
    sl = rec.SI1Name.split(" ");
    if (sl.size() > 0 && !rec.SI1Number.isEmpty())
      mis.push_back({sl.at(0), rec.SI1Number});
    sl = rec.SI2Name.split(" ");
    if (sl.size() > 0 && !rec.SI2Number.isEmpty())
      mis.push_back({sl.at(0), rec.SI2Number});
    sl = rec.SI3Name.split(" ");
    if (sl.size() > 0 && !rec.SI3Number.isEmpty())
      mis.push_back({sl.at(0), rec.SI3Number});
    sl = rec.SI4Name.split(" ");
    if (sl.size() > 0 && !rec.SI4Number.isEmpty())
      mis.push_back({sl.at(0), rec.SI4Number});
    if (mis.size() > 0)
    {
      xmlWriter.writeStartElement("gost:mis");
      for (auto& mi : mis)
      {
        xmlWriter.writeStartElement("gost:mi");
        xmlWriter.writeTextElement("gost:typeNum", mi.first);
        xmlWriter.writeTextElement("gost:manufactureNum", mi.second);
        xmlWriter.writeEndElement(); // mi
      }
      xmlWriter.writeEndElement(); // mis
    }
    xmlWriter.writeEndElement(); // means
    xmlWriter.writeStartElement("gost:conditions");
    xmlWriter.writeTextElement("gost:temperature", rec.Temperature.isEmpty() ? "-" : (rec.Temperature + " \u00B0C"));
    xmlWriter.writeTextElement("gost:pressure", rec.Pressure.isEmpty() ? "-" : (rec.Pressure + " кПа"));
    xmlWriter.writeTextElement("gost:hymidity", rec.Humidity.isEmpty() ? "-" : (rec.Humidity + " %"));
    xmlWriter.writeEndElement(); // conditions
    if (rec.AdditionInfo.length() > 0)
    {
        xmlWriter.writeTextElement("gost:additional_info", rec.AdditionInfo);
    }
    xmlWriter.writeEndElement(); // result
  }
  xmlWriter.writeEndElement(); // application
  xmlWriter.writeEndDocument();
  file->close();
  _ui->Logger->append("Отработано записей : " + QString::number(countAll) + " \n");
  _ui->Logger->append("Годных : " + QString::number(countPassed) + " \n");
  _ui->Logger->append("Не годных : " + QString::number(countFailed) + " \n");
  _ui->Logger->append("Создан файл для загрузки в ФГИС Аршин : " + newFile + " .\n");
  if (error)
    _ui->Logger->append("!!!ВНИМАНИЕ!!! Файл содержит ошибки.\n");
  qApp->processEvents(); // обрабатываем события, для вывода в лог
}


QString InterFIF::publicName(QString realName)
{
  QString result = "Физическое лицо";
  if (realName.contains("ООО")
      || realName.contains("АО")
      || realName.contains("ИП")
      || realName.contains("АК")
      || realName.contains("АМО")
      || realName.contains("АНО")
      || realName.contains("АПОУ")
      || realName.contains("АСУСО")
      || realName.contains("АУ")
      || realName.contains("БОУ")
      || realName.contains("БПОУ")
      || realName.contains("БУ")
      || realName.contains("ВЧ")
      || realName.contains("УЗ")
      || realName.contains("БОУ")
      || realName.contains("БУ")
      || realName.contains("ГКУ")
      || realName.contains("УП")
      || realName.contains("УЗ")
      || realName.contains("Инспекция")
      || realName.contains("Колхоз")
      || realName.contains("КХ")
      || realName.contains("КУ")
      || realName.contains("КЦСОН")
      || realName.contains("ЛОК")
      || realName.contains("ДОУ")
      || realName.contains("МАУ")
      || realName.contains("СОУ")
      || realName.contains("КОУ")
      || realName.contains("МУ")
      || realName.contains("НБ")
      || realName.contains("НИЦ")
      || realName.contains("НО")
      || realName.contains("НП")
      || realName.contains("НУЗ")
      || realName.contains("ПО")
      || realName.contains("Райпо")
      || realName.contains("СК")
      || realName.contains("СНТ")
      || realName.contains("СПК")
      || realName.contains("СТ")
      || realName.contains("ТС")
      || realName.contains("ТС")
      || realName.contains("ФГ")
      || realName.contains("ФБ")
      || realName.contains("ФК")
      || realName.contains("ОУ")
      || realName.contains("Фонд")
      || realName.contains("Церковь")
      || realName.contains("ФБУ")
      || realName.contains("ФГБУ")
      || realName.contains("обществ"))
    result = realName;
  return result;
}


void InterFIF::getNumbersFromArshin(QString protocolFileName)
{
  QString program = "interFIF.py";
  QStringList arguments;
  QString arg;
  for (auto s : protocolFileName)
  {
    arg += s;
    if (s == "/")
      arg += s;
  }
  arguments << arg;

  QProcess* myProcess = new QProcess(this);
  myProcess->startDetached(program, arguments);
}



