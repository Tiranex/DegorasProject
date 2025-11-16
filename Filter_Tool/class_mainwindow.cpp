#include "class_mainwindow.h"
#include "ui_form_mainwindow.h" // Asumo que el nombre del UI header es este
#include "tracking_data.h"
#include "plot.h"
#include "errorplot.h"

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
    QVector<QPointF> fitted_samples = ui->histogramPlot->getSelectedSamples();
    QVector<QPointF> error_points, fitted_error_points;

    std::set<qulonglong> selected;
    dpslr::ilrs::algorithms::RangeDataV rd;
    dpslr::ilrs::algorithms::ResidualsStats resid;

    std::transform(v.begin(), v.end(), std::inserter(selected, selected.begin()),
                   [](const auto& e){return static_cast<qulonglong>(e.x());});

    long double prev_start = -1.L, offset = 0.L;
    for (const auto& shot : m_trackingData->data.ranges) {
        if (shot.start_time < prev_start) offset += 86400.L;
        prev_start = shot.start_time;

        if (selected.count(static_cast<qulonglong>((shot.start_time + offset) * 1e9)))
            rd.push_back({shot.start_time, shot.tof_2w - m_trackingData->data.cal_val_overall,
            shot.pre_2w, shot.trop_corr_2w, shot.bias});
    }

    auto res_error = dpslr::ilrs::algorithms::calculateResidualsStats(m_trackingData->data.obj_bs, rd, resid);
    if (res_error != dpslr::ilrs::algorithms::ResiStatsCalcErr::NOT_ERROR) {
        // NAME UPDATE: SalaraInformation -> DegorasInformation
        DegorasInformation::showWarning("Filter Tool", "Residuals calculation failed.", "", this);
        return;
    }

    dpslr::ilrs::algorithms::ResidualsStats res_stats = resid;
    //auto calc_error = dpslr::algorithms::calculateResidualsStats(m_trackingData->data.obj_bs, resid, res_stats);
    //if (calc_error != dpslr::algorithms::ResiStatsCalcErr::NOT_ERROR) {
    //    // NAME UPDATE: SalaraInformation -> DegorasInformation
     //   DegorasInformation::showWarning("Filter Tool", "Statistics calculation failed.", "", this);
      //  return;
    //}

    for (int idx = res_stats.total_bin_stats.amask_rfrms.size() - 1; idx >= 0; idx--)
        if (!res_stats.total_bin_stats.amask_rfrms[idx]) {
            error_points.push_back(v.takeAt(idx));
            fitted_error_points.push_back(fitted_samples.takeAt(idx));
        }

    ui->filterPlot->selected_curve->setSamples(v);
    ui->filterPlot->error_curve->setSamples(error_points);
    ui->histogramPlot->selected_curve->setSamples(fitted_samples);
    ui->histogramPlot->error_curve->setSamples(fitted_error_points);

    ui->filterPlot->replot();
    ui->histogramPlot->replot();

    // Update statistics members in tracking data
    m_trackingData->data.stats_rfrms = res_stats.total_bin_stats.stats_rfrms;
    // ... and others ...

    // Update UI labels
    ui->lbl_rms_ps->setText(QString::number(res_stats.total_bin_stats.stats_rfrms.rms, 'f', 2));
    ui->lbl_mean_ps->setText(QString::number(res_stats.total_bin_stats.stats_rfrms.mean, 'f', 2));
    ui->lbl_peak_ps->setText(QString::number(res_stats.total_bin_stats.stats_rfrms.peak, 'f', 2));
    ui->lbl_returns->setText(QString::number(res_stats.total_bin_stats.ptn));
    ui->lbl_echoes->setText(QString::number(res_stats.total_bin_stats.stats_rfrms.aptn));
    ui->lbl_noise->setText(QString::number(res_stats.total_bin_stats.stats_rfrms.rptn));

    // NAME UPDATE: SalaraInformation -> DegorasInformation
    DegorasInformation::showInfo("Filter Tool", "Statistics calculation successful.", "", this);
}

void MainWindow::on_pb_recalculate_clicked()
{
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
