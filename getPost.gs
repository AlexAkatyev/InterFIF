// Функция автоматически создает кастомное меню при открытии таблицы
function onOpen() {
  const ui = SpreadsheetApp.getUi();
  ui.createMenu('Парсер ВНИИМС')
    .addItem('Запустить для диапазона строк...', 'showRowRangePrompt')
    .addToUi();
}

// Функция создания диалогового окна
function showRowRangePrompt() {
  const ui = SpreadsheetApp.getUi();

  // Выводим окно с запросом
  const result = ui.prompt(
    'Запуск обработки',
    'Введите диапазон строк через дефис (например: 2-15 или 5-5):',
    ui.ButtonSet.OK_CANCEL
  );

  const button = result.getSelectedButton();
  const text = result.getResponseText().trim();

  if (button == ui.Button.OK) {
    // Валидация ввода регулярным выражением (ищем формат "число-число")
    const match = text.match(/^(\d+)-(\d+)$/);

    if (!match) {
      ui.alert('Ошибка', 'Неверный формат! Введите диапазон в формате "число-число" (например, 2-10).', ui.ButtonSet.OK);
      return;
    }

    const startRow = parseInt(match[1], 10);
    const endRow = parseInt(match[2], 10);

    if (startRow < 1 || endRow < startRow) {
      ui.alert('Ошибка', 'Начальная строка должна быть не меньше 1, а конечная строка — больше или равна начальной.', ui.ButtonSet.OK);
      return;
    }

    // Запуск основного процесса обработки
    processRows(startRow, endRow);
  }
}

// Основной процесс обработки строк
function processRows(startRow, endRow) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const ui = SpreadsheetApp.getUi();

  // НАСТРОЙКИ СТОЛБЦОВ: Замените буквы на ваши реальные столбцы
  const COL_MIT_NUMBER = 'A'; // Столбец для cletB
  const COL_MI_NUMBER = 'B';  // Столбец для cletD
  const COL_DATE = 'C';       // Столбец для cletF
  const COL_RESULT = 'D';     // Столбец, куда записывать результат выполнения getPost

  // Хелпер для перевода буквы столбца в индекс (A->1, B->2 и т.д.)
  const letterToColumn = (letter) => {
    let column = 0, length = letter.length;
    for (let i = 0; i < length; i++) {
      column += (letter.charCodeAt(i) - 64) * Math.pow(26, length - i - 1);
    }
    return column;
  };

  const colBIndex = letterToColumn(COL_MIT_NUMBER);
  const colDIndex = letterToColumn(COL_MI_NUMBER);
  const colFIndex = letterToColumn(COL_DATE);
  const colResIndex = letterToColumn(COL_RESULT);

  let successCount = 0;

  // Построчный обход диапазона
  for (let row = startRow; row <= endRow; row++) {
    // Извлекаем значения ячеек. Приводим к строке, чтобы избежать ошибок с substring
    let cletB = String(sheet.getRange(row, colBIndex).getValue()).trim();
    let cletD = String(sheet.getRange(row, colDIndex).getValue()).trim();
    let cletF = String(sheet.getRange(row, colFIndex).getValue()).trim();

    // Проверяем, чтобы обязательные ячейки не были пустыми
    if (!cletB || !cletD || !cletF) {
      Logger.log(`Строка ${row} пропущена: заполнены не все данные.`);
      continue;
    }

    // Если дата вернулась в формате JS Date (из-за форматирования ячейки Google Таблиц)
    if (cletF.includes('GMT') || cletF.length > 20) {
      const cellValue = sheet.getRange(row, colFIndex).getValue();
      if (cellValue instanceof Date) {
        // Преобразуем объект даты в строку формата YYYY-MM-DD
        cletF = Utilities.formatDate(cellValue, Session.getScriptTimeZone(), "yyyy-MM-dd");
      }
    }

    Logger.log(`Обработка строки ${row}: cletB=${cletB}, cletD=${cletD}, cletF=${cletF}`);

    // Вызов вашей функции getPost (код функции должен находиться в этом же проекте)
    let docNum = getPost(cletB, cletD, cletF);

    // Записываем результат в таблицу
    sheet.getRange(row, colResIndex).setValue(docNum);
    successCount++;

    // Сбрасываем кэш изменений, чтобы пользователь видел обновление ячеек в реальном времени
    SpreadsheetApp.flush();
  }

  ui.alert('Готово', `Обработка завершена.\nУспешно обработано строк: ${successCount} из ${endRow - startRow + 1}.`, ui.ButtonSet.OK);
}

// Вставьте сюда вашу функцию getPost(cletB, cletD, cletF) из предыдущего ответа...























function getPost(cletB, cletD, cletF) {
  const baseUrl = 'https://fgis.gost.ru/fundmetrology/eapi/vri';
  const options = {
    'method': 'get',
    'muteHttpExceptions': true
  };

  // --- ПЕРВЫЙ БЛОК: Цикл до 10 попыток с параметром 'start' ---
  let attempt = 0;
  while (attempt < 10) {
    Utilities.sleep(1000); // Пауза 1 секунда (аналог time.sleep(1))

    // Формируем параметры для первого типа запроса
    let queryParams = [
      'year=' + encodeURIComponent(cletF.substring(0, 4)),
      'mit_number=' + encodeURIComponent(cletB),
      'mi_number=' + encodeURIComponent(cletD),
      'start=' + encodeURIComponent(attempt * 100),
      'rows=100'
    ].join('&');

    let fullUrl = baseUrl + '?' + queryParams;

    // Логирование запроса (аналог ваших print)
    Logger.log('----------------------------- ' + attempt);
    Logger.log('Запрос: GET ' + fullUrl);

    try {
      let response = UrlFetchApp.fetch(fullUrl, options);
      let statusCode = response.getResponseCode();
      let responseText = response.getContentText();

      Logger.log('Ответ (Код: ' + statusCode + '): ' + responseText);
      Logger.log('-----------------------------');

      if (statusCode === 200) {
        let data = JSON.parse(responseText);
        if (data && data.result) {
          let res = data.result;

          if (res.count === 0) {
            return '';
          }

          if (res.items && Array.isArray(res.items)) {
            for (let i = 0; i < res.items.length; i++) {
              let item = res.items[i];
              // Проверяем наличие 'ДЮЮ' в номере документа
              if (item.result_docnum && item.result_docnum.indexOf('ДЮЮ') !== -1) {
                return item.result_docnum;
              }
            }
          }
        }
      }
    } catch (e) {
      Logger.log('Ошибка запроса: ' + e.toString());
    }

    attempt++;
  }

  // --- ВТОРОЙ БЛОК: Цикл до 2 попыток с параметром 'verification_date' ---
  attempt = 0;
  while (attempt < 2) {
    Utilities.sleep(1000);

    // Формируем параметры для второго типа запроса
    let queryParams = [
      'verification_date=' + encodeURIComponent(cletF.substring(0, 10)),
      'mit_number=' + encodeURIComponent(cletB),
      'mi_number=' + encodeURIComponent(cletD),
      'rows=100'
    ].join('&');

    let fullUrl = baseUrl + '?' + queryParams;

    Logger.log('----------------------------- ' + attempt);
    Logger.log('Запрос (Блок 2): GET ' + fullUrl);

    try {
      let response = UrlFetchApp.fetch(fullUrl, options);
      let statusCode = response.getResponseCode();
      let responseText = response.getContentText();

      Logger.log('Ответ (Код: ' + statusCode + '): ' + responseText);
      Logger.log('-----------------------------');

      if (statusCode === 200) {
        let data = JSON.parse(responseText);
        if (data && data.result) {
          let res = data.result;

          if (res.count === 0) {
            return '';
          }

          if (res.items && Array.isArray(res.items)) {
            for (let i = 0; i < res.items.length; i++) {
              let item = res.items[i];
              if (item.result_docnum && item.result_docnum.indexOf('ДЮЮ') !== -1) {
                return item.result_docnum;
              }
            }
          }
        }
      }
    } catch (e) {
      Logger.log('Ошибка запроса: ' + e.toString());
    }

    attempt++;
  }

  return '';
}



function sendRequestAsLocal() {
  // 1. Узнать свой внешний локальный IP
  var responseIp = UrlFetchApp.fetch('https://ipify.org');
  var myIp = responseIp.getContentText();

  // 2. Сделать GET-запрос с подменой заголовка
  var url = 'URL_ВАШЕГО_СЕРВЕРА';
  var options = {
    'method': 'get',
    'headers': {
      'X-Forwarded-For': myIp // Передаем ваш локальный IP
    }
  };

  var response = UrlFetchApp.fetch(url, options);
  Logger.log(response.getContentText());
}



