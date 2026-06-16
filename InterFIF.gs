function onOpen() {
  SpreadsheetApp.getUi()
    .createMenu(' Отчетность во ФГИС')
    .addItem('1. Выгрузка данных в Аршин', 'createXMLToArshin')
    .addItem('2. Скачать готовый скрипт выгрузки данных из Аршина (.py)', 'showExportPyDialogArshin')
    .addItem('3. Загрузить JSON с ответами из Аршина', 'showImportDialogArshin')
    .addItem('4. Создать XML для ЕИС ФСА', 'exportToFSA')
    .addToUi();
}


// ------------------------------------------------------------------------------------------------------------------
// 1
// ------------------------------------------------------------------------------------------------------------------

function createXMLToArshin() {
  const ui = SpreadsheetApp.getUi();

  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const data = sheet.getDataRange().getValues();

  if (data.length <= 2) {
    SpreadsheetApp.getUi().alert('Таблица пуста или содержит только заголовки! Создание файла выгрузки быссмысленно.');
    return;
  }

  const htmlTemplate = HtmlService.createTemplateFromFile('ExportDataToArshin');
  const htmlOutput = htmlTemplate.evaluate().setWidth(400).setHeight(150);
  SpreadsheetApp.getUi().showModalDialog(htmlOutput, 'Экспорт в XML');
}


// Генерация XML строки (вызывается из HTML интерфейса)
function generateXmlContent() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const data = sheet.getDataRange().getValues();

  let xmlString = '<?xml version="1.0" encoding="UTF-8"?>\n<gost:application xmlns:gost="urn://fgis-arshin.gost.ru/module-verifications/import/2020-06-19">\n';
  const headers = data[0];

  for (let i = 2; i < data.length; i++) {
    xmlString += '    <gost:result>\n';
    let valid = data[i][9] == 'Пригодно';
    xmlString += '        <gost:miInfo>\n';
    xmlString += '            <gost:singleMI>\n';
    xmlString += `                <gost:mitypeNumber>${data[i][1]}</gost:mitypeNumber>\n`;
    xmlString += `                <gost:manufactureNum>${data[i][3]}</gost:manufactureNum>\n`;
    xmlString += `                <gost:modification>${data[i][2]}</gost:modification>\n`;
    xmlString += '            </gost:singleMI>\n';
    xmlString += '        </gost:miInfo>\n';
    xmlString += `        <gost:signCipher>ДЮЮ</gost:signCipher>\n`;
    xmlString += `        <gost:miOwner>${data[i][4]}</gost:miOwner>\n`;
    xmlString += `        <gost:vrfDate>${getDataFromStr(data[i][5])}</gost:vrfDate>\n`;
    if (valid) {
      xmlString += `        <gost:validDate>${getDataFromStr(data[i][6])}</gost:validDate>\n`;
    }
    xmlString += `        <gost:type>2</gost:type>\n`; // всегда периодическая поверка
    xmlString += `        <gost:calibration>false</gost:calibration>\n`; // калибровку не делают
    if (valid) {
      xmlString += '        <gost:applicable>\n';
      xmlString += `            <gost:signPass>false</gost:signPass>\n`;
      xmlString += `            <gost:signMi>false</gost:signMi>\n`;
      xmlString += '        </gost:applicable>\n';
    }
    else {
      xmlString += '        <gost:inapplicable>\n';
      let reason = data[i][10];
      if (reason == '') {
        reason = 'Не соответствует требованиям методики поверки';
      }
      xmlString += `            <gost:reasons>${reason}</gost:reasons>\n`;
      xmlString += '        </gost:inapplicable>\n';
    }
    xmlString += `        <gost:docTitle>${data[i][7]}</gost:docTitle>\n`;
    xmlString += `        <gost:metrologist>${data[i][11]}</gost:metrologist>\n`;
    xmlString += `        <gost:means>\n`;
    xmlString += `            <gost:mieta>\n`;
    xmlString += `                <gost:number>${data[i][12].split(' ')[0]}</gost:number>\n`;
    xmlString += `            </gost:mieta>\n`;
    xmlString += `        </gost:means>\n`;
    xmlString += `        <gost:conditions>\n`;
    xmlString += `            <gost:temperature>${data[i][21]} °C</gost:temperature>\n`;
    xmlString += `            <gost:pressure>${data[i][22]} кПа</gost:pressure>\n`;
    xmlString += `            <gost:hymidity>${data[i][23]} %</gost:hymidity>\n`;
    xmlString += `        </gost:conditions>\n`;
    let addInfo = data[i][37];
    if (addInfo != ''){
      xmlString += `        <gost:additional_info>${addInfo}</gost:additional_info>\n`;
    }
    xmlString += '    </gost:result>\n';
  }
  xmlString += '</gost:application>';

  return Utilities.base64Encode(xmlString, Utilities.Charset.UTF_8);
}


function getDataFromStr(input) {
  let rawDateStr = input //"Thu May 07 2026 00:00:00 GMT+0400 (Самарское стандартное время)";

  // 1. Превращаем строку в реальный объект даты
  let dateObject = new Date(rawDateStr);

  // 2. Форматируем в YYYY-MM-DD
  return Utilities.formatDate(dateObject, Session.getScriptTimeZone(), "yyyy-MM-dd");
}



// ----------------------------------------------------------------------------------------------------------------------------
// 2
// ----------------------------------------------------------------------------------------------------------------------------

// Показ диалога выгрузки Python-скрипта
function showExportPyDialogArshin() {
  const htmlOutput = HtmlService.createHtmlOutputFromFile('ExportPyDialog')
      .setWidth(400)
      .setHeight(250)
      .setTitle('Генерация скрипта Python');
  SpreadsheetApp.getUi().showModalDialog(htmlOutput, 'Экспорт в .py файл');
}


// Функция сборки Python кода со встроенными данными (вызывается из ExportPyDialog)
function generatePythonScriptArshin(startRow, endRow) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Лист1");
  const data = [];

  startRow = parseInt(startRow, 10);
  endRow = parseInt(endRow, 10);
  if (isNaN(startRow) || startRow < 2) startRow = 2;
  if (isNaN(endRow) || endRow < startRow) endRow = sheet.getLastRow();

  for (let r = startRow; r <= endRow; r++) {
    let cellAK = sheet.getRange(r, 37).getValue();
    if (cellAK != '') continue;

    let cellB = sheet.getRange(r, 2).getValue();
    let cellD = sheet.getRange(r, 4).getValue();

    // Форматируем дату в нужный вам формат YYYY-MM-DDT00:00:00Z на стороне облака
    let cellFValue = sheet.getRange(r, 6).getValue();
    let formattedDate = '';

    if (cellFValue) {
      let d = null;
      if (cellFValue instanceof Date) {
        d = cellFValue;
      } else if (typeof cellFValue === 'number') {
        d = new Date(1899, 11, 30);
        d.setDate(d.getDate() + cellFValue);
      } else {
        let displayStr = sheet.getRange(r, 6).getDisplayValue().trim();
        let parts = displayStr.match(/^(\d{1,2})\.(\d{1,2})\.(\d{4})/);
        if (parts) {
          d = new Date(parseInt(parts[3], 10), parseInt(parts[2], 10) - 1, parseInt(parts[1], 10));
        } else {
          let parsedTimestamp = Date.parse(displayStr);
          if (!isNaN(parsedTimestamp)) d = new Date(parsedTimestamp);
        }
      }

      if (d && !isNaN(d.getTime())) {
        let year = d.getFullYear();
        let month = String(d.getMonth() + 1).padStart(2, '0');
        let day = String(d.getDate()).padStart(2, '0');
        formattedDate = year + '-' + month + '-' + day + 'T00:00:00Z';
      } else {
        formattedDate = String(cellFValue);
      }
    }

    if (!cellB && !cellD && !cellFValue) continue;

    data.push({
      row: r,
      cletB: String(cellB || ''),
      cletD: String(cellD || ''),
      cletF: formattedDate
    });
  }

  // Превращаем массив в строку JSON для встраивания в Python
  const jsonDataString = JSON.stringify(data, null, 4);

  // Шаблон вашего Python-скрипта с плейсхолдером под встроенные данные
  const pythonTemplate = `import datetime
import json
import os
import sys
import time
import requests

# Отключаем предупреждения об отсутствии SSL-верификации (так как в вашем коде verify=False)
import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

# Встроенные данные из Google Таблицы
DATA_FROM_SHEETS = ${jsonDataString}

def getTypeSIFromStringCell(v):
    r = v[-12:]
    result = ''
    for s in r:
        if s in ['-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9']:
            result += s
    return result

def getSerialNumberFromStringCell(v):
    if not v: return ''
    r = v.split(',')[-1:][0]
    result = ''
    for s in r:
        if s not in [')', '(', '"']:
            result += s
    return result

def getStrFromDataCell(v):
    # Если дата пришла числом из формулы Excel
    if "COMPUTED_VALUE" in v or "," in v:
        try:
            r = v.split(',')[1].split('.')[0]
            dd = ''.join([s for s in r if s not in [')', '(', '"']])
            d = datetime.date(1899, 12, 30) + datetime.timedelta(int(dd))
            return d.strftime("%Y-%m-%dT00:00:00Z")
        except:
            pass
    # Если дата уже в виде строки (ДД.ММ.ГГГГ или ГГГГ-ММ-ДД)
    try:
        if "." in v:
            d = datetime.datetime.strptime(v.strip(), "%d.%m.%Y")
            return d.strftime("%Y-%m-%dT00:00:00Z")
        elif "-" in v:
            return v.strip()[:10] + "T00:00:00Z"
    except:
        print(f"Не удалось распознать формат даты: {v}")
    return v

def getPost(cletB, cletD, cletF):
    u = 'https://fgis.gost.ru/fundmetrology/eapi/vri'
    attempt = 0
    while attempt < 10:
        time.sleep(1)
        param = {'year': cletF[:4], 'mit_number': cletB, 'mi_number': cletD, 'start': attempt * 100, 'rows': 100}
        try:
            resp = requests.get(url=u, params=param, verify=False, timeout=10)
            if resp.status_code == 200:
                data = resp.json()
                if 'result' in data and data['result']['count'] == 0:
                    return ''
                for item in data.get('result', {}).get('items', []):
                    if 'ДЮЮ' in item.get('result_docnum', ''):
                        return item['result_docnum']
        except Exception as e:
            print(f"Ошибка сети: {e}")
        attempt += 1

    param = {'verification_date': cletF[:10], 'mit_number': cletB, 'mi_number': cletD, 'rows': 100}
    attempt = 0
    while attempt < 2:
        time.sleep(1)
        try:
            resp = requests.get(url=u, params=param, verify=False, timeout=10)
            if resp.status_code == 200:
                data = resp.json()
                if 'result' in data and data['result']['count'] == 0:
                    return ''
                for item in data.get('result', {}).get('items', []):
                    if 'ДЮЮ' in item.get('result_docnum', ''):
                        return item['result_docnum']
        except Exception as e:
            print(f"Ошибка сети: {e}")
        attempt += 1
    return ''

def getShortArshinNumber(fullNumber):
    if not fullNumber: return ''
    return fullNumber.split('/')[-1]

def main():
    rows = DATA_FROM_SHEETS
    output_filename = "arshin_results.json"

    print(f"Загружено встроенных строк для обработки: {len(rows)}")

    for i, item in enumerate(rows):
        print(f"\\n+++ Обработка строки {item['row']} ({i+1}/{len(rows)}) +++")
        vCellB = getTypeSIFromStringCell(item['cletB'])
        vCellD = getSerialNumberFromStringCell(item['cletD'])
        vCellF = getStrFromDataCell(item['cletF'])

        print(f"Параметры: ТИП={vCellB}, ЗАВ.№={vCellD}, ДАТА={vCellF}")

        resasq = getPost(vCellB, vCellD, vCellF)

        if resasq == '':
            print('-> Данные в Аршине отсутствуют')
        else:
            print(f'-> Найдено: {resasq}')

        item['resultArshin'] = getShortArshinNumber(resasq)

    print(f"\\nСохранение результатов в {output_filename}...")
    with open(output_filename, "w", encoding="utf-8") as f:
        json.dump(rows, f, indent=2, ensure_ascii=False)

    print("Работа окончена. Файл результатов создан. Загрузите arshin_results.json обратно через меню Google Таблицы.")
    input("Нажмите Enter, чтобы закрыть окно...")

if __name__ == "__main__":
    main()`;

  return pythonTemplate;
}




// ----------------------------------------------------------------------------------------------------------------------------
// 3
// ----------------------------------------------------------------------------------------------------------------------------

// Показ диалога загрузки данных
function showImportDialogArshin() {
  const htmlOutput = HtmlService.createHtmlOutputFromFile('ImportDialog')
      .setWidth(450)
      .setHeight(200)
      .setTitle('Загрузка ответов из JSON');
  SpreadsheetApp.getUi().showModalDialog(htmlOutput, 'Импорт данных');
}

// Функция сбора данных из таблицы для передачи в HTML (вызывается из ExportDialog)
function getDataForExport(startRow, endRow) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Лист1");
  const data = [];

  // Проверяем корректность диапазона
  startRow = parseInt(startRow, 10);
  endRow = parseInt(endRow, 10);
  if (isNaN(startRow) || startRow < 2) startRow = 2;
  if (isNaN(endRow) || endRow < startRow) endRow = sheet.getLastRow();

  for (let r = startRow; r <= endRow; r++) {
    let cellB = sheet.getRange(r, 2).getValue();
    let cellD = sheet.getRange(r, 4).getValue();

    // Получаем значение ячейки F как объект даты/числа или строку
    let cellFValue = sheet.getRange(r, 6).getValue();
    let formattedDate = '';

    if (cellFValue) {
      let d = null;

      // 1. Если это объект Date (Google Таблицы часто сами преобразуют даты в объект Date)
      if (cellFValue instanceof Date) {
        d = cellFValue;
      }
      // 2. Если это число (внутренний формат даты Excel/Google Sheets)
      else if (typeof cellFValue === 'number') {
        // Переводим порядковое число дней Excel в дату JS
        d = new Date(1899, 11, 30);
        d.setDate(d.getDate() + cellFValue);
      }
      // 3. Если это строка (текстовый формат или результат сложной формулы)
      else {
        let displayStr = sheet.getRange(r, 6).getDisplayValue().trim();
        // Пробуем распарсить стандартный российский формат ДД.ММ.ГГГГ
        let parts = displayStr.match(/^(\d{1,2})\.(\d{1,2})\.(\d{4})/);
        if (parts) {
          d = new Date(parseInt(parts[3], 10), parseInt(parts[2], 10) - 1, parseInt(parts[1], 10));
        } else {
          // Пробуем стандартный парсинг JS (для форматов ГГГГ-ММ-ДД и др.)
          let parsedTimestamp = Date.parse(displayStr);
          if (!isNaN(parsedTimestamp)) {
            d = new Date(parsedTimestamp);
          }
        }
      }

      // Если дата успешно распознана, приводим её к строгому формату YYYY-MM-DDT00:00:00Z
      if (d && !isNaN(d.getTime())) {
        let year = d.getFullYear();
        let month = String(d.getMonth() + 1).padStart(2, '0');
        let day = String(d.getDate()).padStart(2, '0');
        formattedDate = year + '-' + month + '-' + day + 'T00:00:00Z';
      } else {
        // Если ничего не сработало, отдаем текст как есть
        formattedDate = String(cellFValue);
      }
    }

    if (!cellB && !cellD && !cellFValue) continue; // Пропускаем полностью пустые строки внутри диапазона

    data.push({
      row: r,
      cletB: String(cellB || ''),
      cletD: String(cellD || ''),
      cletF: formattedDate
    });
  }
  return JSON.stringify(data, null, 2);
}

// Функция записи готовых данных обратно в таблицу (вызывается из ImportDialog)
function processImportedJSON(jsonString) {
  try {
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Лист1");
    const data = JSON.parse(jsonString);

    // Записываем результаты в колонку AK (37)
    data.forEach(item => {
      if (item.row && item.resultArshin !== undefined) {
        sheet.getRange(item.row, 37).setValue(item.resultArshin);
      }
    });
    return "Успешно! Обновлено строк: " + data.length;
  } catch(e) {
    return "Ошибка обработки файла: " + e.message;
  }
}





// ----------------------------------------------------------------------------------------------------------------------------
// 4
// ----------------------------------------------------------------------------------------------------------------------------

// Показ диалога выгрузки XML-файла
function exportToFSA() {
  const htmlOutput = HtmlService.createHtmlOutputFromFile('ExportXmlToFSADialog')
      .setWidth(400)
      .setHeight(250)
      .setTitle('Генерация XML для ЕИС ФСА');
  SpreadsheetApp.getUi().showModalDialog(htmlOutput, 'Экспорт в XML');
}


// Функция генерации XML по вашему шаблону (вызывается из ExportXmlDialog)
function generateFsaXml(startRow, endRow) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Лист1");

  startRow = parseInt(startRow, 10);
  endRow = parseInt(endRow, 10);
  if (isNaN(startRow) || startRow < 2) startRow = 2;
  if (isNaN(endRow) || endRow < startRow) endRow = sheet.getLastRow();

  // Вспомогательная функция для форматирования даты в yyyy-MM-dd
  const formatDateFsa = (val) => {
    if (!val) return '';
    let d = (val instanceof Date) ? val : new Date(val);
    if (isNaN(d.getTime()) && typeof val === 'number') {
      d = new Date(1899, 11, 30);
      d.setDate(d.getDate() + val);
    }
    if (isNaN(d.getTime())) return '';
    let month = String(d.getMonth() + 1).padStart(2, '0');
    let day = String(d.getDate()).padStart(2, '0');
    return d.getFullYear() + '-' + month + '-' + day;
  };

  // Начало формирования XML документа
  let xml = '<?xml version="1.0" encoding="UTF-8"?>\n';
  xml += '<Message xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">\n';
  xml += '  <VerificationMeasuringInstrumentData>\n';

  for (let r = startRow; r <= endRow; r++) {
    let arshinNumber = String(sheet.getRange(r, 37).getValue() || '').trim(); // AK (37)
    let typeSI = String(sheet.getRange(r, 2).getValue() || '').trim();       // B (2)

    // Пропускаем строки без номера Аршина или Типа СИ
    if (!arshinNumber && !typeSI) continue;

    let verifyDate = formatDateFsa(sheet.getRange(r, 6).getValue());        // F (6)
    let validityDate = formatDateFsa(sheet.getRange(r, 7).getValue());    // G (7)
    let lastName = String(sheet.getRange(r, 33).getValue() || '').trim();    // AG (33)
    let firstName = String(sheet.getRange(r, 34).getValue() || '').trim();   // AH (44)
    let snils = String(sheet.getRange(r, 36).getValue() || '').trim().replace(/-/g, ''); // AJ (36)
    let isNoValidText = String(sheet.getRange(r, 42).getValue() || '').trim() == 'Не годен'; // AF (32)

    let resultVerification = validityDate == '' ? '2' : '1';

    xml += '    <VerificationMeasuringInstrument>\n';
    xml += '      <NumberVerification>' + arshinNumber + '</NumberVerification>\n';
    xml += '      <DateVerification>' + verifyDate + '</DateVerification>\n';

    if (validityDate != '') {
      xml += '      <DateEndVerification>' + validityDate + '</DateEndVerification>\n';
    }

    xml += '      <TypeMeasuringInstrument>' + typeSI + '</TypeMeasuringInstrument>\n';
    xml += '      <ApprovedEmployees>\n';
    xml += '        <Name>\n';
    xml += '          <Last>' + lastName + '</Last>\n';
    xml += '          <First>' + firstName + '</First>\n';
    xml += '        </Name>\n';
    xml += '        <SNILS>' + snils + '</SNILS>\n';
    xml += '      </ApprovedEmployees>\n';
    xml += '      <ResultVerification>' + resultVerification + '</ResultVerification>\n';
    xml += '    </VerificationMeasuringInstrument>\n';
  }

  xml += '  </VerificationMeasuringInstrumentData>\n';
  xml += '  <SaveMethod>2</SaveMethod>\n';
  xml += '</Message>';

  return xml;
}

