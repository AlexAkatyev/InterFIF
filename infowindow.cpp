#include "infowindow.h"
#include "ui_infowindow.h"

InfoWindow::InfoWindow(QWidget *parent) :
  QMainWindow(nullptr),
  _ui(new Ui::InfoWindow)
{
  _ui->setupUi(this);
  fillHelp();
  connect(_ui->Exiter, &QAbstractButton::released, this, &InfoWindow::close);
  connect(parent, &QWidget::destroyed, this, [&](){
    deleteLater();
  });
}

InfoWindow::~InfoWindow()
{
  delete _ui;
}


void InfoWindow::fillHelp()
{
  _ui->textBrowser->setText(
"<html>"
"<body>"
"<p>"
"<b><font size=""7"">Программа формирования файлов загрузок</font></b><br>"
"<br>"
"<b><font size=""6"">Руководство пользователя</font></b><br>"
"<br>"
"Программа формирует на основе результатов поверки водосчетчиков и газосчетчиков два типа файлов:<br>"
"1. Файл для загрузки в ФГИС Аршин.<br>"
"Этот файл необходимо вручную загрузить в ФГИС Аршин.<br>"
"2. Файл для загрузки в ЕИС ФСА<br>"
"Файл формируется после загрузки данных в ФГИС Аршин и внесения в протокол номеров поверки.<br>"
"Этот файл необходимо вручную загрузить в ЕИС ФСА.<br>"
"<br>"
"<br>"
"<b>Последовательность действий</b><br>"
"1.Для результативной работы программы необходимо предварительно, в зависимости от того, что требуется сделать<br>"
"1.1. Указать имя файла протокола с расширением XLSX<br>"
"1.2. Указать имя файла с расширением XML, который будет предназначен для загрузки в ФГИС Аршин<br>"
"1.3. Указать имя файла с расширением XML, который будет предназначен для загрузки в ЕИС ФСА<br>"
"2. Нажать на кнопку ВЫПОЛНИТЬ. После нажатия кнопки ВЫПОЛНИТЬ, запустится процедура создания указанных файлов выгрузки.<br>"
"3. В процессе обработки и создания файлов для выгрузки в окне программы проводится протоколирование"
" работ и вывод сообщений об ошибках, если таковые имеются. Информация о результаты работы также "
"выводится в окно.<br>"
"<br>"
"<b>Передача сведений о владельце СИ</b><br>"
"Программа автоматически фильтрует владельца СИ. Если в имени владельца не указана организационно-"
"правовая форма владельца прописными буквами, например ООО или ИП, программа формирует информацию"
" о владельце СИ в ФГИС Аршин строкой ""Физическое лицо"".<br>"
"<br>"
"<br>"
"Информация для технических специалистов, обслуживающих машины с установленным ПО InterFIF:<br>"
"Программа для получения открытых данных из ФГИС Аршин использует скрипт на языке Pyton3 и библиотеку openpyxl.<br>"
"<br>"
"<br>"
"<b>Решение вопросов</b><br>"
"С разработчиком программы можно связаться по адресу <b>aleksey.akatyev@gmail.com</b><br>"
"<br>"
"</p>"
"</body>"
"</html>"
);
}

