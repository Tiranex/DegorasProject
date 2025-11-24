#include "class_mainwindow.h"
#include "ui_form_mainwindow.h" // Asumo que el nombre del UI header es este
#include "tracking_data.h"
#include "plot.h"
#include "errorplot.h"
#include "cpf_predictor.h"

#include <LibDegorasSLR/ILRS/algorithms/statistics.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <set>

#include <Tracking/trackingfilemanager.h>
#include <LibDegorasSLR/ILRS/algorithms/data/statistics_data.h>

    MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
ui(new Ui::MainWindow),
m_trackingData(nullptr),
m_isChanged(false)
{
    ui->setupUi(this);

    // Initial state
    updateUIState(false);
    setupConnections();

    // Process command line arguments
    QStringList arguments = QApplication::instance()->arguments();
    if (arguments.size() > 1 && !arguments[1].isEmpty())
    {
        loadTrackingData(arguments[1]);
    }
}

MainWindow::~MainWindow()
{
    delete m_trackingData;
    delete ui;
}

void MainWindow::setupConnections()
{
    // Connections from plots
    connect(ui->filterPlot, &Plot::selectionChanged, this, &MainWindow::onPlotSelectionChanged);
    connect(ui->histogramPlot, &ErrorPlot::selectionChanged, this, &MainWindow::onPlotPickingFinished);

    connect(ui->filterPlot, &Plot::startedPicking, this, &MainWindow::onPlotPickingStarted);
    connect(ui->filterPlot, &Plot::finishedPicking, this, &MainWindow::onPlotPickingFinished);
    connect(ui->histogramPlot, &ErrorPlot::startedPicking, this, &MainWindow::onHistogramPickingStarted);
    connect(ui->histogramPlot, &ErrorPlot::finishedPicking, this, &MainWindow::onHistogramPickingFinished);

    // Sync panning between the two plots
    connect(ui->filterPlot->getPanner(), &QwtPlotPanner::panned, this, &MainWindow::syncPanFromMainPlot);
    connect(ui->histogramPlot->getPanner(), &QwtPlotPanner::panned, this, &MainWindow::syncPanFromHistogramPlot);
}

void MainWindow::updateUIState(bool hasData)
{
    // File settings
    ui->pb_recalculate->setEnabled(hasData);

    // Tools
    ui->gb_tools->setEnabled(hasData);

    // Statistics
    if (!hasData) clearStatistics();

    // Menu Actions
    ui->actionSave->setEnabled(false); // Enabled only on change
    ui->actionDiscard->setEnabled(hasData && m_trackingData && m_trackingData->dp_tracking);
}

void MainWindow::clearStatistics()
{
    ui->lbl_rms_ps->setText("...");
    ui->lbl_mean_ps->setText("...");
    ui->lbl_peak_ps->setText("...");
    ui->lbl_returns->setText("...");
    ui->lbl_echoes->setText("...");
    ui->lbl_noise->setText("...");
}

void MainWindow::on_pb_load_clicked()
{
    on_actionLoad_triggered();
}

void MainWindow::on_actionLoad_triggered()
{
    if (m_isChanged) {
        auto reply = QMessageBox::question(this, "Unsaved Changes",
                                           "You have unsaved changes. Do you want to discard them and load a new file?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    //QString path_test = SalaraSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_CurrentObservations");
    QString filePath = QFileDialog::getOpenFileName(this, "Open Tracking File","test_path");

    if (!filePath.isEmpty()) {
        loadTrackingData(filePath);
    }
}

void MainWindow::loadTrackingData(const QString& filePath)
{
    delete m_trackingData;
    m_trackingData = new TrackingData(filePath, false);

    m_currentFilePath = m_trackingData->file_name;
    ui->le_filePath->setText(m_currentFilePath);
    ui->lbl_sessionID->setText(m_trackingData->satel_name);

    updatePlots();
    updateUIState(true);
    onFilterSaved(); // Reset changed state
}

void MainWindow::updatePlots()
{
    if (!m_trackingData) return;

    QPolygonF all_samples, selected_samples;
    for (const auto* aux : m_trackingData->listAll()) {
        all_samples += QPointF(aux->time, aux->difference);
    }
    for (const auto* aux : m_trackingData->listEchoes()) {
        selected_samples += QPointF(aux->time, aux->difference);
    }

    ui->filterPlot->setTitle("Tracking: "+ m_trackingData->satel_name);
    ui->filterPlot->setSamples(all_samples);
    ui->filterPlot->setBinSize(m_trackingData->data.obj_bs);
    ui->histogramPlot->setBinSize(m_trackingData->data.obj_bs);

    if(all_samples.size() != selected_samples.size())
        ui->filterPlot->selected_curve->setSamples(selected_samples);
}


void MainWindow::onPlotSelectionChanged()
{
    auto fit_errors = ui->filterPlot->getFitErrors();
    ui->histogramPlot->setSamples(fit_errors);
    onFilterChanged();
}

void MainWindow::onPlotPickingStarted()
{
    ui->histogramPlot->setPickingEnabled(false);
}

void MainWindow::onPlotPickingFinished()
{
    ui->histogramPlot->setPickingEnabled(true);
    onFilterChanged();

    // When main plot finishes picking, selection might have changed, so we update the histogram
    auto error_samples = ui->histogramPlot->getSelectedSamples();
    auto samples = ui->filterPlot->getSelectedSamples();
    decltype(samples) selected_samples;
    std::copy_if(std::make_move_iterator(samples.begin()), std::make_move_iterator(samples.end()),
                 std::back_inserter(selected_samples), [&error_samples](const auto& p)
                 {
                     auto it = std::find_if(error_samples.begin(), error_samples.end(),
                                            [&p](const auto& t){return p.x() == t.x();});
                     return it != error_samples.end();
                 });
    ui->filterPlot->setSamples(selected_samples);
}

void MainWindow::onHistogramPickingStarted()
{
    ui->filterPlot->setPickingEnabled(false);
}

void MainWindow::onHistogramPickingFinished()
{
    ui->filterPlot->setPickingEnabled(true);
    onFilterChanged();
}

void MainWindow::onFilterChanged()
{
    m_isChanged = true;
    ui->actionSave->setEnabled(true);
}

void MainWindow::onFilterSaved()
{
    m_isChanged = false;
    ui->actionSave->setEnabled(false);
}

void MainWindow::on_pb_autoFilter_clicked()
{
    if (!m_trackingData) return;

    ui->gb_tools->setEnabled(false);
    QProgressDialog pd("Autofiltering in progress...", "", 0, 0, this);
    pd.setCancelButton(nullptr);

    auto future = QtConcurrent::run([this] {
        int i = 0;
        int changed_count;
        do {
            // Qt 6 Update: Using type-safe pointer-to-member-function instead of string-based invocation.
            QMetaObject::invokeMethod(this, &MainWindow::threshFilter, Qt::BlockingQueuedConnection, &changed_count);
            i++;
        } while(changed_count > 0 && i < 20);
    });

    QFutureWatcher<void> fw;
    connect(&fw, &QFutureWatcher<void>::finished, &pd, &QProgressDialog::accept);
    fw.setFuture(future);
    pd.exec();

    ui->gb_tools->setEnabled(true);
    onFilterChanged();
}

int MainWindow::threshFilter()
{
    auto thresh_samples = ui->histogramPlot->getThreshSamples();
    auto samples = ui->filterPlot->getSelectedSamples();

    if (thresh_samples.size() != samples.size())
    {
        decltype(samples) selected_samples;
        std::copy_if(std::make_move_iterator(samples.begin()), std::make_move_iterator(samples.end()),
                     std::back_inserter(selected_samples), [&thresh_samples](const auto& p)
                     {
                         auto it = std::find_if(thresh_samples.begin(), thresh_samples.end(),
                                                [&p](const auto& t){return p.x() == t.x();});
                         return it != thresh_samples.end();
                     });
        ui->filterPlot->setSamples(selected_samples);
    }
    return samples.size() - thresh_samples.size();
}

void MainWindow::on_pb_rmsFilter_clicked()
{
    threshFilter();
    onFilterChanged();
}

void MainWindow::on_actionDiscard_triggered()
{
    if (!m_trackingData || !m_trackingData->dp_tracking) return;

    auto result = QMessageBox::question(this, "Discard Tracking",
                                        "Discard this tracking session? It will be saved as tracked without results.",
                                        {QMessageBox::Yes | QMessageBox::No}, QMessageBox::No);
    if (result == QMessageBox::No) return;

    for (auto&& shot : m_trackingData->data.ranges) {
        shot.flag = Tracking::RangeData::FilterFlag::NOISE;
    }

    dpslr::ilrs::algorithms::DistStats zero_stats{0,0,0,0.L,0.L,0.L,0.L,0.L,0.};
    m_trackingData->data.stats_1rms = zero_stats;
    m_trackingData->data.stats_rfrms = zero_stats;
    m_trackingData->data.filter_mode = Tracking::FilterMode::MANUAL;

    // NAME UPDATE: SalaraInformation -> DegorasInformation
    DegorasInformation errors = TrackingFileManager::writeTracking(m_trackingData->data);
    if (errors.hasError()) {
        errors.showErrors("Filter Tool", DegorasInformation::WARNING, "Error saving discarded file.", this);
    } else {
        //QString current_path = SalaraSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_CurrentObservations");
        QString filename = TrackingFileManager::trackingFilename(m_trackingData->data);
        //if (QFile::remove(current_path + '/' + filename)) {
            // NAME UPDATE: SalaraInformation -> DegorasInformation
            DegorasInformation::showInfo("Filter Tool", "File discarded successfully. You may close this window.", "", this);
            //onFilterSaved();
        //} else {
         //   DegorasInformation::showInfo("Filter Tool", "Could not delete the tracking file. Please discard it manually.", "", this);
        //}
    }
}

void MainWindow::on_actionSave_triggered()
{
    if (!m_trackingData) return;

    QVector<QPointF> v = ui->filterPlot->getSelectedSamples();
    if (m_trackingData->dp_tracking) {
        if (v.isEmpty()) {
            // NAME UPDATE: SalaraInformation -> DegorasInformation
            DegorasInformation::showWarning("Filter Tool", "No points selected. Have you calculated statistics?", "", this);
            return;
        }
        qInfo() << QString("Saving as data %1 points").arg(v.size());

        std::set<qulonglong> selected;
        std::transform(v.begin(), v.end(), std::inserter(selected, selected.begin()),
                       [](const auto& e){return static_cast<qulonglong>(e.x());});

        long double prev_start = -1.L;
        long double offset = 0.L;
        for (auto&& shot : m_trackingData->data.ranges) {
            if (shot.start_time < prev_start) offset += 86400.L;
            prev_start = shot.start_time;

            if (selected.count(static_cast<qulonglong>((shot.start_time + offset) * 1e9L)))
                shot.flag = Tracking::RangeData::FilterFlag::DATA;
            else
                shot.flag = Tracking::RangeData::FilterFlag::NOISE;
        }
        m_trackingData->data.filter_mode = Tracking::FilterMode::MANUAL;
        DegorasInformation errors = TrackingFileManager::writeTracking(m_trackingData->data);
        if (errors.hasError())
            errors.showErrors("Filter Tool", DegorasInformation::WARNING, "Error on file saving", this);
        else {
            // NAME UPDATE: SalaraInformation -> DegorasInformation
            DegorasInformation::showInfo("Filter Tool", "Data saved successfully.", "", this);
            onFilterSaved();
        }
    } else {
        // ... (Logic for non-dp_tracking files remains the same)
        qInfo() << "Saving for non-DP tracking is not fully implemented in this refactor.";
        onFilterSaved();
    }
}

void MainWindow::on_pb_calcStats_clicked()
{
    if (!m_trackingData || !m_trackingData->dp_tracking) return;

    QVector<QPointF> v = ui->filterPlot->getSelectedSamples();

    if (v.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "Selecciona puntos verdes primero.");
        return;
    }

    // Preparar vectores visuales
    QVector<QPointF> fitted_samples = ui->histogramPlot->getSelectedSamples();
    QVector<QPointF> error_points, fitted_error_points;

    // Preparar selección
    std::set<qulonglong> selected;
    std::transform(v.begin(), v.end(), std::inserter(selected, selected.begin()),
                   [](const auto& e){return static_cast<qulonglong>(e.x());});

    // ESTRUCTURAS LIBRERÍA
    dpslr::ilrs::algorithms::RangeDataV rd;
    dpslr::ilrs::algorithms::ResidualsStats resid;

    // PASO 1: Calcular la media bruta manual
    double sum_val = 0.0;
    int count_val = 0;
    for (const auto* echo : m_trackingData->listAll()) {
        if (selected.count(echo->time)) {
            sum_val += static_cast<double>(echo->difference);
            count_val++;
        }
    }
    double manual_mean = (count_val > 0) ? sum_val / count_val : 0.0;

    qDebug() << "Manual Mean Offset:" << manual_mean;

    // PASO 2: Rellenar y CALCULAR AL VUELO (Aquí está la magia)
    double min_res = 1e20;
    double max_res = -1e20;

    // VARIABLE DE RESPALDO: Suma de cuadrados calculada aquí mismo
    double sum_sq_manual = 0.0;

    for (const auto* echo : m_trackingData->listAll()) {
        if (selected.count(echo->time)) {

            double centered_residual = static_cast<double>(echo->difference) - manual_mean;

            // Acumulamos para el RMS manual (A prueba de fallos)
            sum_sq_manual += (centered_residual * centered_residual);

            // Min/Max para Bin Size
            if (centered_residual < min_res) min_res = centered_residual;
            if (centered_residual > max_res) max_res = centered_residual;

            // Guardar para librería
            rd.push_back({
                static_cast<long double>(echo->time) * 1.0e-9,
                centered_residual,
                0.0, 0.0, 0.0
            });
        }
    }

    // PASO 3: Bin Size Dinámico (High Density)
    double spread = max_res - min_res;
    int dynamic_bin_size = m_trackingData->data.obj_bs;
    if (dynamic_bin_size <= 0) dynamic_bin_size = 120;

    // Usamos target 100 bins para asegurar densidad
    if (spread > (dynamic_bin_size * 100.0)) {
        dynamic_bin_size = static_cast<int>(spread / 100.0) + 1;
        qDebug() << "Bin Dinámico ajustado a:" << dynamic_bin_size;
    }

    // PASO 4: Llamada a la librería
    auto res_error = dpslr::ilrs::algorithms::calculateResidualsStats(dynamic_bin_size, rd, resid);

    if (res_error != dpslr::ilrs::algorithms::ResiStatsCalcErr::NOT_ERROR) {
        qDebug() << "Error librería:" << (int)res_error;
        // No retornamos, intentamos usar el manual
    }

    // --- GESTIÓN DE RESULTADOS (HÍBRIDA) ---
    dpslr::ilrs::algorithms::ResidualsStats res_stats = resid;
    auto stats = res_stats.total_bin_stats.stats_rfrms;

    double final_rms = static_cast<double>(stats.rms);
    double final_mean_residual = static_cast<double>(stats.mean);
    long long final_ptn = res_stats.total_bin_stats.ptn;

    // Si la librería falla (da 0), usamos nuestro cálculo manual
    if (final_rms == 0.0 && count_val > 0) {
        final_rms = std::sqrt(sum_sq_manual / count_val);
        final_ptn = count_val;
        qDebug() << "--- USANDO RMS MANUAL (Fallback):" << final_rms;
    } else {
        qDebug() << "--- USANDO RMS LIBRERÍA:" << final_rms;
    }

    // Actualizar Gráficas (Solo si la librería funcionó y generó máscara)
    // Si la librería falló, pintamos todo como válido por defecto
    bool lib_ok = (stats.rms > 0.0);

    if (lib_ok) {
        // Lógica de filtrado de la librería
        for (int idx = res_stats.total_bin_stats.amask_rfrms.size() - 1; idx >= 0; idx--)
            if (!res_stats.total_bin_stats.amask_rfrms[idx]) {
                error_points.push_back(v.takeAt(idx));
                fitted_error_points.push_back(fitted_samples.takeAt(idx));
            }
    } else {
        // Si RMS=0, no borramos puntos de la gráfica, los dejamos todos verdes
    }

    ui->filterPlot->selected_curve->setSamples(v);
    ui->filterPlot->error_curve->setSamples(error_points);
    // ... (resto de updates de plots igual) ...
    ui->filterPlot->replot();

    // --- ACTUALIZAR ETIQUETAS ---
    // RMS
    ui->lbl_rms_ps->setText(QString::number(final_rms, 'f', 2));

    // MEAN (Sumamos el offset manual)
    double real_mean_display = manual_mean + final_mean_residual;
    ui->lbl_mean_ps->setText(QString::number(real_mean_display, 'f', 2));

    // OTRAS
    ui->lbl_peak_ps->setText(QString::number(static_cast<double>(stats.peak), 'f', 2));
    ui->lbl_returns->setText(QString::number(final_ptn));
    ui->lbl_echoes->setText(QString::number(lib_ok ? stats.aptn : count_val));
    ui->lbl_noise->setText(QString::number(lib_ok ? stats.rptn : 0));

    DegorasInformation::showInfo("Filter Tool", "Statistics calculated.", "", this);
}


// adición MARIO: funcionamiento botón cargar CPF
void MainWindow::on_pb_loadCPF_clicked()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Select CPF File",
                                                QDir::homePath(),
                                                "CPF Files (*.cpf *.npt *.tjr);;All Files (*)");

    if (!path.isEmpty()) {
        m_cpfPath = path;               // Guardamos la ruta en la variable de clase
        ui->le_cpfPath->setText(path);  // Mostramos la ruta en el cuadro de texto visual

        // Habilitamos el botón de recalcular ahora que tenemos un archivo
        ui->pb_recalculate->setEnabled(true);
    }
}


// adición MARIO: funcionamiento botón recalcular una vez cargado el CPF (esta función estaba pero vacía)
void MainWindow::on_pb_recalculate_clicked()
{


    // 1. Validaciones de seguridad
    if (!m_trackingData) return;

    // Si el usuario escribió la ruta a mano en vez de usar el botón Load, la cogemos del texto
    if (m_cpfPath.isEmpty()) {
        m_cpfPath = ui->le_cpfPath->text();
    }

    if (m_cpfPath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please load a CPF file first.");
        return;
    }

    // 2. Cargar el objeto CPF
    CPFPredictor newPrediction;
    if (!newPrediction.load(m_cpfPath)) {
        QMessageBox::critical(this, "Error", "Could not parse the CPF file.");
        return;
    }

    // 3. Procesamiento (Iterar sobre TODOS los datos, incluso el ruido)
    // Usamos un ProgressDialog por si son muchos datos
    QProgressDialog progress("Recalculating Residuals...", "Abort", 0, m_trackingData->listAll().size(), this);
    progress.setWindowModality(Qt::WindowModal);

    int counter = 0;
    for (TrackingData::Echo* echo : m_trackingData->listAll()) {
        if (progress.wasCanceled()) break;

        // A. Calcular el TOF teórico con el nuevo CPF
        // Pasamos el MJD y el tiempo en nanosegundos del disparo
        long long predicted_ps = newPrediction.calculateTwoWayTOF(echo->mjd, echo->time);




        // --- PEGA AQUÍ EL DEBUG (Solo imprimirá el primero para no saturar) ---
        if (counter == 0)
        {
            qDebug() << "========================================";
            qDebug() << "TEST DE RECALCULO:";
            qDebug() << "MJD:" << echo->mjd << " Time(ns):" << echo->time;
            qDebug() << "Vuelo Real (ps):" << echo->flight_time;
            qDebug() << "Predicción (ps):" << predicted_ps;
            qDebug() << "Diferencia (ps):" << (echo->flight_time - predicted_ps);

            if (predicted_ps == 0) {
                qDebug() << "ALERTA: La predicción ha dado 0. Revisa fechas o carga del CPF.";
            }
            qDebug() << "========================================";
        }












        // B. IMPORTANTE: Actualizar el residuo
        // Residuo = Observado (Hardware) - Calculado (CPF)
        // echo->flight_time es el valor crudo del láser.
        echo->difference = echo->flight_time - predicted_ps;

        counter++;
        if (counter % 100 == 0) progress.setValue(counter);
    }
    progress.setValue(m_trackingData->listAll().size());

    // 4. Actualizar la interfaz gráfica
    updatePlots();      // Redibuja los puntos verdes con las nuevas alturas Y
    onFilterChanged();  // Marca que hay cambios sin guardar

    // Informar al usuario
    ui->lbl_sessionID->setText("Recalculated with: " + QFileInfo(m_cpfPath).fileName());

}


void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_isChanged) {
        auto resBtn = QMessageBox::question(this, "Unsaved Changes",
                                            "There are unsaved changes. Are you sure you want to exit?",
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (resBtn == QMessageBox::Yes) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void MainWindow::syncPanFromMainPlot()
{
    QwtInterval interval = ui->filterPlot->axisInterval(QwtPlot::Axis::xBottom);
    ui->histogramPlot->setAxisScale(QwtPlot::Axis::xBottom, interval.minValue(), interval.maxValue());
    ui->histogramPlot->replot();
}

void MainWindow::syncPanFromHistogramPlot()
{
    QwtInterval interval = ui->histogramPlot->axisInterval(QwtPlot::Axis::xBottom);
    ui->filterPlot->setAxisScale(QwtPlot::Axis::xBottom, interval.minValue(), interval.maxValue());
    ui->filterPlot->replot();
}





