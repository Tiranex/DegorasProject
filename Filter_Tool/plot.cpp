#include "plot.h"
#include <qwt/qwt_curve_fitter.h>
#include <qwt/qwt_series_data.h>
#include <qwt/qwt_point_data.h>
#include <LibDegorasBase/Statistics/fitting.h>

#include <cmath>

void Plot::QwtSLRPlotMagnifier::rescale( double factor )
{
    QwtPlot* plt = plot();

    if (!plt)
        return;

    factor = qAbs(factor);
    if (qFuzzyCompare(factor, 1.0) || qFuzzyCompare(factor, 0.0) )
        return;

    bool doReplot = false;

    const bool autoReplot = plt->autoReplot();
    plt->setAutoReplot( false );

    for ( int axisId = 0; axisId < QwtPlot::axisCnt; axisId++ )
    {
        if (isAxisEnabled(axisId))
        {
            const QwtScaleMap scaleMap = plt->canvasMap(axisId);

            double v1 = scaleMap.s1();
            double v2 = scaleMap.s2();

            if ( scaleMap.transformation() )
            {
                v1 = scaleMap.transform(v1);
                v2 = scaleMap.transform(v2);
            }

            const double center = 0.5 * (v1 + v2) * factor;
            const double width_2 = 0.5 * (v2 - v1) * factor;

            v1 = center - width_2;
            v2 = center + width_2;

            if ( scaleMap.transformation() )
            {
                v1 = scaleMap.invTransform( v1 );
                v2 = scaleMap.invTransform( v2 );
            }

            plt->setAxisScale(axisId, v1, v2);
            doReplot = true;
        }
    }

    plt->setAutoReplot( autoReplot );

    if ( doReplot )
        plt->replot();
}

Plot::Plot(QWidget *parent, QString title):
    QwtPlot( parent ),
    picking(false),
    plot_curve( nullptr )
{
    canvas()->setStyleSheet("border: 2px solid Black;"
                            "border-radius: 15px;"
                            "background-color: rgb(70,70,70);");

    this->setAutoReplot(false);

    // Pinceles.
    QPen pen_grid(Qt::SolidPattern, 0.5, Qt::DashLine);
    QPen pen_mark(Qt::SolidPattern, 0.5, Qt::DashLine);
    pen_grid.setColor(QColor("gray"));
    pen_mark.setColor(QColor("red"));

    // Marks.
    this->mark_0 = new QwtPlotMarker;
    this->mark_inicio = new QwtPlotMarker;
    this->mark_final = new QwtPlotMarker;
    this->mark_0->setLineStyle(QwtPlotMarker::LineStyle::HLine);
    this->mark_inicio->setLineStyle(QwtPlotMarker::LineStyle::VLine);
    this->mark_final->setLineStyle(QwtPlotMarker::LineStyle::VLine);
    this->mark_0->setLinePen(pen_mark);
    this->mark_inicio->setLinePen(pen_mark);
    this->mark_final->setLinePen(pen_mark);
    this->mark_0->setZ(1);
    this->mark_inicio->setZ(1);
    this->mark_final->setZ(1);
    this->mark_0->attach(this);
    this->mark_final->attach(this);
    this->mark_inicio->attach(this);

    // Curva principal.
    plot_curve = new QwtPlotCurve;
    plot_curve->setPen(QColor("White"));
    this->plot_curve->setData(new QwtSLRArraySeriesData());
    plot_curve->setZ(2);
    plot_curve->attach(this);

    // Curva de seleccionados.
    this->selected_curve = new QwtPlotCurve;
    this->selected_curve->setPen(QColor("White"));
    this->selected_curve->setData(new QwtSLRArraySeriesData());
    this->selected_curve->setZ(2);
    this->selected_curve->attach(this);

    // Curva de errores por filtrado auto.
    this->error_curve = new QwtPlotCurve;
    this->error_curve->setPen(QColor("White"));
    this->error_curve->setData(new QwtSLRArraySeriesData());
    this->error_curve->setZ(2);
    this->error_curve->attach(this);

    // Curva de ajuste.
    this->adjust_curve = new QwtPlotCurve;
    this->adjust_curve->setPen(QColor("Green"));
    this->adjust_curve->setData(new QwtSLRArraySeriesData());
    this->adjust_curve->setZ(3);
    this->adjust_curve->attach(this); // PONER DE NUEVO!!!

    // Renderizado y estilo.
    this->plot_curve->setRenderThreadCount(0); // 0: use QThread::idealThreadCount()
    this->plot_curve->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->plot_curve->setStyle(QwtPlotCurve::CurveStyle::Dots);
    this->plot_curve->setSymbol(new QwtSymbol(QwtSymbol::Hexagon, QColor(240,240,240), Qt::NoPen, QSize(3,3)));
    this->selected_curve->setRenderThreadCount(0); // 0: use QThread::idealThreadCount()
    this->selected_curve->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->selected_curve->setStyle(QwtPlotCurve::CurveStyle::Dots);
    this->selected_curve->setSymbol(new QwtSymbol(QwtSymbol::Hexagon, QColor(0,255,0), Qt::NoPen, QSize(3,3)));
    this->error_curve->setRenderThreadCount(0); // 0: use QThread::idealThreadCount()
    this->error_curve->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->error_curve->setStyle(QwtPlotCurve::CurveStyle::Dots);
    this->error_curve->setSymbol(new QwtSymbol(QwtSymbol::Hexagon, QColor(255,0,0), Qt::NoPen, QSize(3,3)));
    this->adjust_curve->setRenderThreadCount(0); // 0: use QThread::idealThreadCount()
    this->adjust_curve->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, true);
    this->adjust_curve->setStyle(QwtPlotCurve::CurveStyle::Lines);

//    this->plot_curve->setCurveAttribute(QwtPlotCurve::CurveAttribute::Fitted, true);

//    QwtSplineCurveFitter *fitter = new QwtSplineCurveFitter();
//       fitter->setFitMode(QwtSplineCurveFitter::ParametricSpline);
//       fitter->setSplineSize(1000);
//       this->plot_curve->setCurveFitter(fitter);


    // Axis y titulo.
    this->titleLabel()->setFont(QFont("Open Sans Semibold", 14));
    QwtText xt("Tiempo de salida (hh:mm:s)");
    QwtText yt("Deviation from predicted time of flight (ps)");
    QwtText x2t("Time (ns)");
    QwtText y2t("Deviation from predicted time of flight (m)");
    xt.setFont(QFont("Open Sans Semibold", 8));
    yt.setFont(QFont("Open Sans Semibold", 8));
    x2t.setFont(QFont("Open Sans Semibold", 8));
    y2t.setFont(QFont("Open Sans Semibold", 8));
    this->enableAxis(QwtPlot::yRight, true);
    this->enableAxis(QwtPlot::xTop, false);
    this->enableAxis(QwtPlot::yLeft, true);
    this->enableAxis(QwtPlot::xBottom, true);
    this->setAxisTitle(QwtPlot::Axis::yRight, y2t);
    this->setAxisTitle(QwtPlot::Axis::xTop, x2t);
    this->setAxisTitle(QwtPlot::Axis::yLeft, yt);
    this->setAxisTitle(QwtPlot::Axis::xBottom, xt);
    this->setAxisFont(QwtPlot::Axis::yRight, QFont("Open Sans", 6));
    this->setAxisFont(QwtPlot::Axis::xTop, QFont("Open Sans", 6));
    this->setAxisFont(QwtPlot::Axis::yLeft, QFont("Open Sans", 6));
    this->setAxisFont(QwtPlot::Axis::xBottom, QFont("Open Sans", 6));
    this->setAxisMaxMajor(QwtPlot::xTop, 20);
    this->setAxisMaxMajor(QwtPlot::yRight, 20);
    this->setAxisMaxMajor(QwtPlot::xBottom, 20);
    this->setAxisMaxMajor(QwtPlot::yLeft, 20);
    this->setAxisScaleDraw(QwtPlot::Axis::yLeft, new QScaleDraw);
    this->setAxisScaleDraw(QwtPlot::Axis::yRight, new QwtPsToMScaleDraw);
    this->setAxisScaleDraw(QwtPlot::Axis::xBottom, new QwtNanoseconds2TimeScaleDraw);
    //this->setAxisScaleDraw(QwtPlot::Axis::xTop, new QScaleDraw);




    // Grid.
        QwtPlotGrid *grid = new QwtPlotGrid();
        grid->setPen(pen_grid);
        grid->setZ(0);
        grid->attach(this);


    // Panner (desplazamiento).
    this->panner = new QwtPlotPanner(canvas());
    panner->setMouseButton(Qt::MouseButton::RightButton);

    // zoom in/out with the wheel
    this->magnifier = new QwtSLRPlotMagnifier(canvas());
    magnifier->setMouseButton(Qt::NoButton);
    magnifier->setAxisEnabled(QwtPlot::Axis::xBottom, false);
    magnifier->setAxisEnabled(QwtPlot::Axis::xTop, false);
    magnifier->setEnabled(true);

    QPen pen(Qt::SolidPattern, 1.5, Qt::SolidLine, Qt::RoundCap);
    pen.setColor(QColor(0,255,0));

    this->picker = new QwtSLRPlotPicker(canvas());
    picker->setRubberBandPen(pen);

    QObject::connect(picker, &QwtPicker::activated, [this](bool on)
    {
        this->magnifier->setEnabled(!on);
        this->panner->setEnabled(!on);
        if(on)
            this->adjust_curve->detach();
        else
            this->adjust_curve->attach(this);
        this->replot();
    });

    QObject::connect(picker, &QwtSLRPlotPicker::selected, this,  &Plot::selectPoints);
}


void Plot::selectPoints(const QPolygonF& pol)
{
    if(pol.isEmpty())
        return;

    if (!this->picking)
    {
        emit this->startedPicking();
        this->picking = true;
        static_cast<QwtSLRArraySeriesData *>(this->selected_curve->data())->clear();
    }

    QVector<QPointF> selected_points;
    QVector<QPointF> deleted_points;
    QwtSLRArraySeriesData *selected_data = static_cast<QwtSLRArraySeriesData *>(this->selected_curve->data());
    QwtSLRArraySeriesData *curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());

    // Buscamos los limites para la eliminación de puntos.
    double maxX = 0;
    for(const auto& p : std::as_const(pol))
        if(p.x()>maxX)
            maxX = p.x();

    // Insertamos los puntos seleccionados.
    for(size_t i=0; i<this->plot_curve->dataSize(); i++)
    {
        QPointF aux = this->plot_curve->sample(int(i));
        bool inside = pol.containsPoint(aux, Qt::FillRule::OddEvenFill);

        if(inside)
        {
            selected_points.append(aux);
            deleted_points.append(aux);
        }
        else if(aux.x()<=maxX)
            deleted_points.append(aux);
    }

    // Eliminamos los puntos que no nos interesan (arriba y abajo de la zona seleccionada dentro de los intervalos.
    curve_data->remove(deleted_points);
    // Añadimos los puntos que nos interesan al contenedor.
    selected_data->append(selected_points);

    // Si ya hemos seleccionado todos los puntos, actualizamos ejes.
    if(curve_data->size()==0)
    {
        static_cast<QwtSLRArraySeriesData *>(this->error_curve->data())->clear();
        this->setSamples(selected_data->samples());
        this->picking = false;

        emit this->finishedPicking();
    }
    else // Repintamos.
        this->replot();
}

void Plot::setBinSize(int bin_size)
{
    this->bin_size = bin_size;
}

void Plot::setPickingEnabled(bool enabled)
{
    this->picker->enableSelector(enabled);
}




void Plot::setSamples(const QVector<QPointF> &samples)
{
    QwtSLRArraySeriesData *selected_data = static_cast<QwtSLRArraySeriesData *>(this->selected_curve->data());
    QwtSLRArraySeriesData *curve_data = static_cast<QwtSLRArraySeriesData *>(this->plot_curve->data());
    // Borramos los puntos anteriores.
    this->original_curve.clear();
    selected_data->clear();
    curve_data->clear();
    // Almacenamos los puntos.
    this->original_curve.append(samples);
    curve_data->append(samples);
    //this->plot_curve->setData(QwtArraySeriesData<QwtDoublePoint>());
    //this->plot_curve->setSamples(samples);

    // Marcas.
    this->mark_0->setValue(0, 0);
    this->mark_inicio->setValue(plot_curve->minXValue(), 0);
    this->mark_final->setValue(plot_curve->maxXValue(), 0);
    // Margen para trabajar mejor
    double margen_x = (plot_curve->maxXValue()-plot_curve->minXValue())/80.0;
    double margen_y = (plot_curve->maxYValue()-plot_curve->minYValue())/50.0;
    // Ajustamos Axis y repintamos.
    this->setAxisScale(QwtPlot::Axis::xBottom, plot_curve->minXValue()-margen_x, plot_curve->maxXValue()+margen_x);
    this->setAxisScale(QwtPlot::Axis::xTop, plot_curve->minXValue()-margen_x, plot_curve->maxXValue()+margen_x);
    this->setAxisScale(QwtPlot::Axis::yLeft, plot_curve->minYValue()-margen_y, plot_curve->maxYValue()+margen_y);
    this->setAxisScale(QwtPlot::Axis::yRight, plot_curve->minYValue()-margen_y, plot_curve->maxYValue()+margen_y);

    QwtSLRArraySeriesData *fitt_data = static_cast<QwtSLRArraySeriesData *>(this->adjust_curve->data());

    fitt_data->clear();

    auto curve_samples = curve_data->samples();
    std::sort(curve_samples.begin(), curve_samples.end(), [](const auto& a, const auto& b){return a.x() < b.x();});
    QVector<QPointF> oY;
    QVector<double> oY_original;

    std::vector<double> xbin, ybin;
    double time_orig = curve_samples.front().x();

    for (const auto& p : std::as_const(curve_samples))
    {
        if (p.x() - time_orig > this->bin_size * 1e9)
        {
            auto coefs = dpbase::stats::polynomialFit(xbin, ybin, 9);
            for (auto it_x = xbin.begin(), it_y = ybin.begin(); it_x != xbin.end() && it_y != ybin.end(); ++it_x, ++it_y)
            {
                oY.append({*it_x, dpbase::stats::applyPolynomial(coefs, *it_x)});
                oY_original.append(*it_y);
            }
            xbin.clear();
            ybin.clear();
            time_orig = p.x();

        }

        xbin.push_back(p.x());
        ybin.push_back(p.y());
    }

    // TODO: control polynomial fit (matrix is not inversible)
    if (!xbin.empty() && !ybin.empty())
    {
        auto coefs = dpbase::stats::polynomialFit(xbin, ybin, 9);
        for (auto it_x = xbin.begin(), it_y = ybin.begin(); it_x != xbin.end() && it_y != ybin.end(); ++it_x, ++it_y)
        {
            oY.append({*it_x, dpbase::stats::applyPolynomial(coefs, *it_x)});
            oY_original.append(*it_y);
        }
    }



//    for (const auto& p : std::as_const(curve_samples))
//    {
//        double y = coefs[0];

//        for(int i=1; i<coefs.size(); i++)
//            y=y+coefs[i]*std::pow(p.x(),i);

//        oY.append(QPointF(p.x(),y));
//        oY_original.append(p.y());


//    }


    fitt_data->append(oY);

    this->points_fiterrors.clear();

    for(int i = 0; i<curve_data->samples().size(); i++)
    {
        double error = curve_data->samples()[i].y() - fitt_data->samples()[i].y();
        this->points_fiterrors.append(QPointF(curve_data->samples()[i].x(),
                                              std::isnan(error) ? curve_data->samples()[i].y() : error));
    }

    emit this->fitCalculated(fitt_data->samples());
    emit this->selectionChanged();


    this->replot();
}







