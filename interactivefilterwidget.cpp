#include "interactivefilterwidget.h"
#include "ui_interactivefilterwidget.h"

interactivefilterwidget::interactivefilterwidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::interactivefilterwidget)
{
    ui->setupUi(this);
}

interactivefilterwidget::~interactivefilterwidget()
{
    delete ui;
}
