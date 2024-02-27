#ifndef INFOWINDOW_H
#define INFOWINDOW_H

#include <QMainWindow>

namespace Ui {
class InfoWindow;
}

class InfoWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit InfoWindow(QWidget *parent = nullptr);
  ~InfoWindow();

private:
  Ui::InfoWindow *_ui;
  void fillHelp();
};

#endif // INFOWINDOW_H
