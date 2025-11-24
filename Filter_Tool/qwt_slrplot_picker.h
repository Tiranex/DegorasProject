#pragma once

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QVector>
#include <QDateTime>

#include <qwt/qwt_global.h>
#include <qwt/qwt_picker.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_picker_machine.h>
#include <qwt/qwt_painter.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_text.h>

#include <QDebug>

class QwtPlot;

class QwtSLRPlotPicker: public QwtPicker
{
    Q_OBJECT

public:
    // Constructores y destructores.
    explicit QwtSLRPlotPicker(QWidget *canvas = nullptr, int spacing=10);
    ~QwtSLRPlotPicker() = default;
    // Modificadores.
    inline virtual void setSpacing(int s){this->spacing = s;}
    virtual void setAxis(int x, int y);
    // Observadores.
    QwtPlot *plot();
    const QwtPlot *plot() const;
    void clearSelection();
    inline QWidget *canvas(){return parentWidget();}
    inline const QWidget *canvas() const{return parentWidget();}
    inline void enableSelector(bool flag){this->enable_selector=flag;}

signals:
    void spacingChangued(double spacing);
    void selected(const QPolygonF &pa);

protected:
    // Tracker.
    QwtText trackerText(const QPoint& pos) const;
    // Funciones del picker.
    void beginSLRQwtPlotPicker();
    void appendSLRQwtPlotPicker(const QPoint &pos);
    void moveSLRQwtPlotPicker(const QPoint &pos);
    void removeSLRQwtPlotPicker();
    bool endSLRQwtPlotPicker();
    void spacingSLRQwtPlotPicker(int increment);
    // Función de transición.
    void transition(const QEvent *event);
    // Función de dibujado de rubberband.
    void drawRubberBand(QPainter *painter) const;
    // Helpers.
    QPointF invTransform(const QPoint &) const;
    // Flags.
    bool enable_selector;

private:
    // Custom picker machine for this picker.
    class QwtSLRPickerMachine: public  QwtPickerMachine
    {
    public:
        enum QwtSLRPickerMachineCommand
        {
            Begin = 1,
            Append = 2,
            Remove = 3,
            Move = 4,
            End = 5,
            SpacingPlus = 101,
            SpacingMinus = 102
        };

        QwtSLRPickerMachine():QwtPickerMachine(PolygonSelection){}
        QList<QwtPickerMachine::Command>transition(const QwtEventPattern &eventPattern, const QEvent *e);
    };
    // Variables privadas.
    int x_axis;
    int y_axis;
    int spacing;
    QPolygon secondary_picked_points;
    QwtSLRPickerMachine* picker_machine;
};

