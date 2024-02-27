#include "interfif.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  InterFIF w;
  w.show();
  return a.exec();
}
