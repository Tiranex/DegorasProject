#include <QPainter>
#include "qwt_slrplot_picker.h"

// Transición.
QList<QwtPickerMachine::Command>QwtSLRPlotPicker::QwtSLRPickerMachine::transition(const QwtEventPattern &eventPattern,
                                                                                  const QEvent *e)
{
    QList<QwtSLRPickerMachineCommand> cmd_list;
    QList<QwtPickerMachine::Command> cmd_list_cast;

    switch(e->type())
    {
    case QEvent::MouseButtonPress:
    {
        if (eventPattern.mouseMatch(QwtEventPattern::MouseSelect1, dynamic_cast<const QMouseEvent*>(e)))
        {
            if(state()==0)
            {
                cmd_list += Begin;
                cmd_list += Append;
                setState(1);
            }
            else if(state()==1)
            {
                cmd_list += Append;
                setState(2);
            }
            else if(state()==2)
                cmd_list += Append;
        }
        else if(eventPattern.mouseMatch(QwtEventPattern::MouseSelect2, dynamic_cast<const QMouseEvent*>(e)))
        {
            if (state() > 0)
            {
                setState(0);
                cmd_list += Remove;
                cmd_list += End;
            }
        }
        else if(dynamic_cast<const QMouseEvent*>(e)->button() == Qt::MiddleButton)
        {
            if (state() == 2)
                cmd_list += Remove;
        }
        break;
    }
    case QEvent::MouseMove:
    {
        if (state()>0)
            cmd_list += Move;
        break;
    }
    case QEvent::Wheel:
    {
        if(state()>0)
        {
            if(dynamic_cast<const QWheelEvent*>(e)->angleDelta().y()>0)
                cmd_list += QwtSLRPickerMachineCommand::SpacingPlus;
            else
                cmd_list += QwtSLRPickerMachineCommand::SpacingMinus;
        }
        break;
    }
    default:
        break;
    }

    for (int i=0; i<cmd_list.size(); i++)
        cmd_list_cast.append(static_cast<QwtPickerMachine::Command>(cmd_list[i]));

    return cmd_list_cast;
}

QwtSLRPlotPicker::QwtSLRPlotPicker(QWidget *canvas, int s):
    QwtPicker(canvas), x_axis(-1), y_axis(-1), spacing(s)
{
    if ( !canvas )
        return;
    int xAxis = QwtPlot::xBottom;
    const QwtPlot *plot = this->plot();
    if (!plot->axisEnabled(QwtPlot::xBottom ) && plot->axisEnabled(QwtPlot::xTop))
        xAxis = QwtPlot::xTop;
    int yAxis = QwtPlot::yLeft;
    if (!plot->axisEnabled(QwtPlot::yLeft) && plot->axisEnabled(QwtPlot::yRight ))
        yAxis = QwtPlot::yRight;
    this->setAxis( xAxis, yAxis );
    picker_machine = new QwtSLRPickerMachine();
    this->setStateMachine(picker_machine);
    this->setRubberBand(QwtSLRPlotPicker::UserRubberBand);
    this->setTrackerMode(DisplayMode::AlwaysOn);
    this->setTrackerPen(QPen(QColor("white")));
    this->enable_selector = true;
}


QwtText QwtSLRPlotPicker::trackerText( const QPoint &pos ) const
{
    QString label;
    QPointF pos_t(invTransform(pos));
    double light_speed = 0.000299792458; // m/ps
    double value_seconds = pos_t.x()*0.000000001;
    QString x = QDateTime::fromSecsSinceEpoch(uint(value_seconds)).toUTC().toString("hh:mm:ss")+"h";
    QString y = QString::number(pos_t.y()*light_speed, 'f', 3)+"m";
    QString x2 = QString::number(pos_t.x())+"ns";
    QString y2 = QString::number(pos_t.y(), 'f')+"ps";
    label = x+" | "+x2+"\n"+y+" | "+y2;
    return label;
}

// Observadores.
QwtPlot *QwtSLRPlotPicker::plot()
{
    QWidget *w = canvas();
    if (w)
        w = w->parentWidget();
    return qobject_cast<QwtPlot *>( w );
}

const QwtPlot *QwtSLRPlotPicker::plot() const
{
    const QWidget *w = canvas();
    if (w)
        w = w->parentWidget();
    return qobject_cast<const QwtPlot *>(w);
}


// Modificadores.
void QwtSLRPlotPicker::setAxis( int x, int y)
{
    const QwtPlot *plt = plot();
    if ( !plt )
        return;

    if ( x != x_axis || y != y_axis )
    {
        x_axis = x;
        y_axis = y;
    }
}


// Helpers.

QPointF QwtSLRPlotPicker::invTransform( const QPoint &pos ) const
{
    QwtScaleMap xMap = plot()->canvasMap(this->x_axis);
    QwtScaleMap yMap = plot()->canvasMap(this->y_axis);
    return QPointF(xMap.invTransform(pos.x()),yMap.invTransform(pos.y()));
}


// Transiciones.
void QwtSLRPlotPicker::beginSLRQwtPlotPicker()
{
    if(isActive() || this->enable_selector == false)
        return;
    qInfo()<<"Here BEGIN";
    this->spacing = 10;
    this->secondary_picked_points.clear();
    this->secondary_picked_points.resize(0);
    qInfo()<<"Here BEGIN RESIZE";

    this->canvas()->setCursor(Qt::BlankCursor);
    qInfo()<<"Here BEGIN CANVAS";

    QwtPicker::begin();
    qInfo()<<"Here BEGIN BEGIN";

    updateDisplay();
}

void QwtSLRPlotPicker::appendSLRQwtPlotPicker(const QPoint &pos)
{
//    if(!isActive())
//        return;
    qInfo()<<"Here Append";

    const int idx = this->secondary_picked_points.count()-1;
    QPoint aux(pos.x(), pos.y()+this->spacing);
    QPoint aux2(pos.x(), pos.y()-this->spacing);
    if(idx >=1 && this->secondary_picked_points[idx-1].x()>aux.x()-5)
        return;
    this->secondary_picked_points.resize(idx+2);
    this->secondary_picked_points[idx+1] = aux;
    QwtPicker::append(aux2);
}

void QwtSLRPlotPicker::moveSLRQwtPlotPicker(const QPoint &pos)
{
    if(!isActive())
        return;
    const int idx = this->secondary_picked_points.count()-1;
    QPoint aux(pos.x(), pos.y()+this->spacing);
    QPoint aux2(pos.x(), pos.y()-this->spacing);
    if(idx >=1 && this->secondary_picked_points[idx-1].x()>aux.x()-5)
    {
        this->canvas()->setCursor(Qt::CrossCursor);
        return;
    }
    else if(this->canvas()->cursor()==Qt::CrossCursor)
        this->canvas()->setCursor(Qt::BlankCursor);
    if(idx>=0 && this->secondary_picked_points[idx] != aux)
        this->secondary_picked_points[idx] = aux;
    QwtPicker::move(aux2);
}

void QwtSLRPlotPicker::removeSLRQwtPlotPicker()
{
    if (!isActive())
        return;
    const int idx = this->secondary_picked_points.count()-1;
    if (idx>0)
    {
        const int idx = this->secondary_picked_points.count();
        this->secondary_picked_points.resize(idx - 1);
        updateDisplay();
    }
    QwtPicker::remove();
}

bool QwtSLRPlotPicker::endSLRQwtPlotPicker()
{
    this->canvas()->setCursor(Qt::CrossCursor);
    if(!this->accept(this->secondary_picked_points))
        return false;
    if (!isActive() || !QwtPicker::end() || !this->plot())
        return false;
    const QPolygon points = QwtPicker::selection();
    if(points.size() + this->pickedPoints().size() <= 2)
        return false;
    // Generamos el vector de puntos que componen el polígono.
    QPolygonF poligon;
    foreach(QPoint p, points)
        poligon.append(invTransform(p));
    for(int i=this->secondary_picked_points.size()-1; i>=0;i--)
      poligon.append(invTransform(this->secondary_picked_points[i]));
    this->secondary_picked_points.clear();
    this->secondary_picked_points.resize(0);
    QwtPicker::updateDisplay();
    emit this->selected(poligon);
    return true;
}

void QwtSLRPlotPicker::spacingSLRQwtPlotPicker(int increment)
{
    if(!isActive())
        return;
    QPoint aux(this->pickedPoints().last().x(), this->pickedPoints().last().y()+this->spacing);
    if(this->spacing+increment>0)
        this->spacing =  this->spacing + increment;
    if(this->pickedPoints().size()>0)
    {
        this->moveSLRQwtPlotPicker(aux);
        emit this->spacingChangued(this->spacing);
    }
}

void QwtSLRPlotPicker::transition( const QEvent *event )
{
    if (!stateMachine())
        return;

    const QList<QwtPickerMachine::Command> cmd_list_cast = stateMachine()->transition(*this, event);
    QList<QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand> cmd_list;

    for (int i=0; i<cmd_list_cast.size(); i++)
        cmd_list.append(
                    static_cast<QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand>(cmd_list_cast[i]));

    QPoint pos;
    switch (event->type())
    {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
        const QMouseEvent *me = static_cast< const QMouseEvent * >( event );
        pos = me->pos();
        break;
    }
    default:
        pos = parentWidget()->mapFromGlobal(QCursor::pos());
    }

    for (int i = 0; i < cmd_list.count(); i++)
    {
        switch (cmd_list[i])
        {
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::Begin:
        {
            beginSLRQwtPlotPicker();
            break;
        }
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::Append:
        {
            appendSLRQwtPlotPicker(pos);
            break;
        }
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::Move:
        {
            moveSLRQwtPlotPicker(pos);
            break;
        }
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::Remove:
        {
            removeSLRQwtPlotPicker();
            break;
        }
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::End:
        {
            endSLRQwtPlotPicker();
            break;
        }
            // Wheel positivo. Aumentamos el espaciado.
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::SpacingPlus:
        {
            this->spacingSLRQwtPlotPicker(1);
            break;
        }
            // Wheel negativo. Disminuimos el espaciado.
        case QwtSLRPlotPicker::QwtSLRPickerMachine::QwtSLRPickerMachineCommand::SpacingMinus:
        {
            this->spacingSLRQwtPlotPicker(-1);
            break;
        }
        }
    }
}

void QwtSLRPlotPicker::drawRubberBand(QPainter *painter) const
{
    if (!isActive() || rubberBand() == NoRubberBand || rubberBandPen().style() == Qt::NoPen || !stateMachine())
        return;
    const QPolygon pa = adjustedPoints(pickedPoints());
    const QPolygon pa2 = adjustedPoints(this->secondary_picked_points);
    if (pa.empty() || pa2.empty())
        return;
    painter->drawPolyline(pa);
    painter->drawPolyline(pa2);
    painter->drawLine(pa[0], pa2[0]);
    if(pa[0] !=  pa.last())
        painter->drawLine(pa.last(), pa2.last());
}
