#ifndef INTERACTIVEFILTERWIDGET_H
#define INTERACTIVEFILTERWIDGET_H

#include <QWidget>

namespace Ui {
class interactivefilterwidget;
}

class interactivefilterwidget : public QWidget
{
    Q_OBJECT

public:
    explicit interactivefilterwidget(QWidget *parent = nullptr);
    ~interactivefilterwidget();

private:
    Ui::interactivefilterwidget *ui;
};

#endif // INTERACTIVEFILTERWIDGET_H
