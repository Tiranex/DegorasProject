#include "class_mainwindow.h"
#include <cmath>
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
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QClipboard>
#include <QToolTip>

// Last Save/Load Directory Settings
#include "degoras_settings.h"
#include <QDir>
// <QFileInfo> already present

// Keyboard shortcuts
#include "shortcutmanager.h"
// <QLabel> already present
#include <QKeySequenceEdit>

#include <Tracking/trackingfilemanager.h>
#include <datafilter.h>
#include <LibDegorasSLR/ILRS/algorithms/data/statistics_data.h>

    MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
ui(new Ui::MainWindow),
m_trackingData(nullptr),
m_isChanged(false)
{
    ui->setupUi(this);

    this->setStyleSheet("QToolTip { color: #000000; background-color: #ffffe0; border: 1px solid #888888; }");

    // Initial state
    updateUIState(false);
    setupConnections();

    // Keyboard shortcuts
    ui->dockShortcuts->hide();
    setupShortcuts();

    // Creamos una lista con todos los labels que queremos hacer "interactivos"
    QList<QLabel*> statLabels = {
        // Columna 1
        ui->lbl_rms_ps, ui->lbl_mean_ps, ui->lbl_peak_ps,
        // Columna 2 (Asegúrate de que existen en el .ui, si alguno falla, quítalo de la lista)
        ui->lbl_stderr, ui->lbl_skew, ui->lbl_kurt, ui->lbl_snr,
        // Columna 3
        ui->lbl_returns, ui->lbl_echoes, ui->lbl_noise
    };

    // Aplicamos la configuración a todos en bucle
    for (QLabel* label : statLabels) {
        if (label) { // Protección por si alguno no existe todavía
            label->installEventFilter(this);          // Activa la copia al hacer clic
            label->setCursor(Qt::PointingHandCursor); // Pone la manita al pasar por encima

            // Opcional: Añadir un tooltip por defecto para guiar al usuario
            label->setToolTip("Click to copy value");
        }
    }

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
    // connect(ui->histogramPlot, &ErrorPlot::startedPicking, this, &MainWindow::onHistogramPickingStarted);
    // connect(ui->histogramPlot, &ErrorPlot::finishedPicking, this, &MainWindow::onHistogramPickingFinished);

    // Sync panning between the two plots
    connect(ui->filterPlot->getPanner(), &QwtPlotPanner::panned, this, &MainWindow::syncPanFromMainPlot);
    connect(ui->histogramPlot->getPanner(), &QwtPlotPanner::panned, this, &MainWindow::syncPanFromHistogramPlot);
    connect(ui->actionTreshFilter, &QAction::triggered, this, [this]() {
        applyFilter(FilterOptions::TreshFilter);
    });
    connect(ui->actionMedianFilter, &QAction::triggered, this, [this]() {
        applyFilter(FilterOptions::MedianFilter);
    });
    connect(ui->actionExponentialSmoothing, &QAction::triggered, this, [this]() {

        applyFilter(FilterOptions::ExponentialSmoothing);
    });
    connect(ui->actionMovingAverage, &QAction::triggered, this, [this]() {

        applyFilter(FilterOptions::MovingAverage);
    });

    // Keyboard Shortcuts Layout Panel
    connect(ui->actionConfigure_Keyboard_Shortcuts, &QAction::triggered, this, [=](){
        buildShortcutsUI();
        ui->dockShortcuts->show();
        ui->dockShortcuts->raise();
    });

    QwtSLRPlotMagnifier* magFilter = ui->filterPlot->magnifier;
    QwtSLRPlotMagnifier* magErrorFilter = ui->realHistogramPlot->magnifier;
    QwtSLRPlotMagnifier* magHistogramPlot = ui->histogramPlot->magnifier;

    // 2. Enable sync
    magFilter->setSynchronizationEnabled(true);
    magErrorFilter->setSynchronizationEnabled(true);
    magHistogramPlot->setSynchronizationEnabled(true);
    // 3. Connect A to B
    QObject::connect(magFilter, &QwtSLRPlotMagnifier::zoomed,
                     magErrorFilter, &QwtSLRPlotMagnifier::applySharedZoom);

    // 4. Connect B to A
    QObject::connect(magErrorFilter, &QwtSLRPlotMagnifier::zoomed,
                     magFilter, &QwtSLRPlotMagnifier::applySharedZoom);

    QObject::connect(magFilter, &QwtSLRPlotMagnifier::zoomed,
                     magHistogramPlot, &QwtSLRPlotMagnifier::applySharedZoom);
    QObject::connect(magHistogramPlot, &QwtSLRPlotMagnifier::zoomed,
                     magFilter, &QwtSLRPlotMagnifier::applySharedZoom);
    QObject::connect(magHistogramPlot, &QwtSLRPlotMagnifier::zoomed,
                     magErrorFilter, &QwtSLRPlotMagnifier::applySharedZoom);
    QObject::connect(magErrorFilter, &QwtSLRPlotMagnifier::zoomed,
                     magHistogramPlot, &QwtSLRPlotMagnifier::applySharedZoom);

}

QwtSLRPlotMagnifier::QwtSLRPlotMagnifier(QWidget *canvas)
    : QwtPlotMagnifier(canvas),
    m_syncEnabled(false),       // Default to false or true based on preference
    m_isInternalRescale(false)  // Initialize guard to false
{
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

void MainWindow::setupShortcuts()
{
    //auto& manager = ShortcutManager::instance();

    // ADD CUSTOM SHORTCUTS HERE
    ShortcutManager::instance().registerButton("load_dptr_file", ui->pb_load, "Ctrl+L");



}

void MainWindow::buildShortcutsUI()
{
    //auto& manager = ShortcutManager::instance();

    // --- CLEANUP STEP ---
        // Remove existing rows so we don't stack duplicates if clicked twice.
        // ui->shortcutsLayout must be the QFormLayout, not the QScrollArea.
        while (ui->shortcutsLayout->count() > 0) {
        QLayoutItem* item = ui->shortcutsLayout->takeAt(0);
        if (QWidget* w = item->widget()) {
            delete w; // Delete the label/editor
        }
        delete item;
    }

    // Loop through all registered IDs
    for (const QString& id : ShortcutManager::instance().getRegisteredIds()) {

        // 1. Beautify ID
        QString labelText = id;
        labelText.replace("_", " ");
        if (!labelText.isEmpty()) labelText[0] = labelText[0].toUpper();

        // 2. Create UI elements
        QLabel* label = new QLabel(labelText, this);

        QAction* act = ShortcutManager::instance().getAction(id);
        QKeySequence currentSeq = act ? act->shortcut() : QKeySequence();
        QKeySequenceEdit* editor = new QKeySequenceEdit(currentSeq, this);

        // 3. Connect: When user changes key, update Manager + .ini file
        connect(editor, &QKeySequenceEdit::editingFinished, [=](){
            ShortcutManager::instance().setShortcut(id, editor->keySequence());
            // Optional: Set focus back to main window so shortcuts work immediately
            this->setFocus();
        });

        // 4. Add to the layout defined in your .ui file
        ui->shortcutsLayout->addRow(label, editor);
    }
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

    // 1. Get Last loaded file path
    QSettings* settings = DegorasSettings::instance().config();
    const QString settingKey = "Paths/LastLoadDir";

    QString storedDir;

    // Only read if settings initilized correctly
    if(settings)
    {
        storedDir = settings->value(settingKey).toString();
    }

    // 2. If setting directory is not empty and what's inside exists (it's not a deleted folder or a path in your system):
    if(storedDir.isEmpty() || !QDir(storedDir).exists())
    {
        storedDir = QCoreApplication::applicationDirPath(); // Returns 'build' directory instead of 'workspace/DegorasProject/...'.
    }

    // 3. Dialog: getOpenFileName + filter string "Description (*.ext)"
    QString filter = "Tracking Files (*.dptr)";
    QString filePath = QFileDialog::getOpenFileName(this, "Open Tracking File", storedDir, filter);

    // 4. Load Data
    if (!filePath.isEmpty()) {
        loadTrackingData(filePath);

        // --- 5. Save path for next use ---
        QString newDir = QFileInfo(filePath).absolutePath();
        settings->setValue(settingKey, newDir);
        settings->sync();
        // ---------------------------------
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

    //ui->filterPlot->setTitle("Tracking: "+ m_trackingData->satel_name);
    ui->filterPlot->setSamples(all_samples);
    ui->filterPlot->setBinSize(m_trackingData->data.obj_bs);
    ui->histogramPlot->setBinSize(m_trackingData->data.obj_bs);
    ui->realHistogramPlot->setNumBins(m_trackingData->data.obj_bs);

    QVector<double> errors;
    for (const auto& p : all_samples) {
        errors.append(p.y());
    }
    ui->realHistogramPlot->setValues(errors);

    if(all_samples.size() != selected_samples.size())
        ui->filterPlot->selected_curve->setSamples(selected_samples);
}


void MainWindow::onPlotSelectionChanged()
{
    auto fit_errors = ui->filterPlot->getFitErrors();
    auto selected = ui->filterPlot->getSelectedSamples();
    ui->histogramPlot->setSamples(fit_errors);

    QVector<double> error_values;
    for(const auto& p : fit_errors) {
        error_values.append(p.y());
    }
    QVector<double> y_values;
    for(const auto& p : selected){
        y_values.push_back(p.y());
    }

    ui->realHistogramPlot->setValues(y_values);

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

void MainWindow::applyFilter(FilterOptions f)
{
    if (!m_trackingData) return;


    int paramWindowSize = 5; // Valor por defecto
    double paramAlpha = 0.2; // Valor por defecto

    QDialog dialog(this);
    dialog.setWindowTitle("Filter Parameters");
    QFormLayout form(&dialog);

    QSpinBox* sbWindow = nullptr;
    QDoubleSpinBox* sbAlpha = nullptr;


    if (f == FilterOptions::MedianFilter || f == FilterOptions::MovingAverage) {
        sbWindow = new QSpinBox(&dialog);
        sbWindow->setRange(3, 99);
        sbWindow->setSingleStep(2); // Ventana impar usualmente
        sbWindow->setValue(5);
        form.addRow("Window Size (odd):", sbWindow);
    }
    else if (f == FilterOptions::ExponentialSmoothing) { // Suponiendo que tienes este
        sbAlpha = new QDoubleSpinBox(&dialog);
        sbAlpha->setRange(0.01, 1.0);
        sbAlpha->setSingleStep(0.05);
        sbAlpha->setValue(0.2);
        form.addRow("Smoothing Factor (Alpha):", sbAlpha);
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (sbWindow) paramWindowSize = sbWindow->value();
    if (sbAlpha) paramAlpha = sbAlpha->value();

    ui->gb_tools->setEnabled(false);
    QProgressDialog pd("Autofiltering in progress...", "", 0, 0, this);
    pd.setCancelButton(nullptr);

    ui->filterPlot->pushCurrentStateToUndo();
    ui->histogramPlot->pushCurrentStateToUndo();
    auto future = QtConcurrent::run([this, f, paramWindowSize, paramAlpha] {
        if(f == FilterOptions::TreshFilter){
            int i = 0;
            int changed_count;
            do {
                QMetaObject::invokeMethod(this, &MainWindow::threshFilter, Qt::BlockingQueuedConnection, &changed_count);
                i++;
            } while(changed_count > 0 && i < 20);
        } else{
            auto selectedSamples = ui->filterPlot->getSelectedSamples();
            if(f == FilterOptions::MedianFilter){
                auto res = DataFilter::applyMedianFilter(selectedSamples, paramWindowSize);
                ui->filterPlot->setSamples(res);
            }
            else if(f == FilterOptions::MovingAverage){
                auto res = DataFilter::applyMovingAverage(selectedSamples, paramWindowSize);
                ui->filterPlot->setSamples(res);
            }
            else if(f == FilterOptions::ExponentialSmoothing){
                auto res = DataFilter::applyExponentialSmoothing(selectedSamples, paramAlpha);
                ui->filterPlot->setSamples(res);
            }
        }
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
    if (!m_trackingData) {
        DegorasInformation::showWarning("Filter Tool", "No file has been loaded", "", this);
        return;
    }

    // 1. Get Last saved file path
    QSettings* settings = DegorasSettings::instance().config();
    const QString settingKey = "Paths/LastSaveDir";

    QString storedDir;

    // Only read if settings initilized correctly
    if(settings)
    {
        storedDir = settings->value(settingKey).toString();
    }

    // 2. If setting directory is not empty and what's inside exists (it's not a deleted folder or a path in your system):
    if(storedDir.isEmpty() || !QDir(storedDir).exists())
    {
        if(!m_currentFilePath.isEmpty()) // Check if current file path variable is empty for whatever reason
        {
            storedDir = QFileInfo(m_currentFilePath).absolutePath(); // Get its directory path (not file path)
        }
        else // Current File Path empty. Default to project path
        {
            storedDir = QCoreApplication::applicationDirPath(); // Returns 'build' directory instead of 'workspace/DegorasProject/...'. Does that make sense?
        }
    }

    // 3. Combine folder + current filename to pre-fill the dialog
    QString currentFileName = QFileInfo(m_currentFilePath).fileName();
    QString initialPath = QDir(storedDir).filePath(currentFileName);

    // 4. Dialog: getSaveFileName + filter string "Description (*.ext)"
    QString filter = "Tracking Files (*.dptr)";
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Save Tracking File",
                                                    initialPath, // Use last stored save path if possible
                                                    filter);

    // Check if the path is NOT empty (User did not click Cancel)
    if (!filePath.isEmpty()) {

        // ------- 5. Save path for next use --------------
        // Get the selected folder path
        QString newDir = QFileInfo(filePath).absolutePath();
        // Save it
        settings->setValue(settingKey, newDir);
        // Optional: sync to disk
        settings->sync();
        // ------------------------------------------------

        // 6. Manually ensure the extension is present
        // (Some OS file dialogs don't auto-append the extension)
        if (!filePath.endsWith(".dptr", Qt::CaseInsensitive)) {
            filePath += ".dptr";
        }

        // 7. Update the internal data structure with the current valid samples from the plot
        QVector<QPointF> validSamples = ui->filterPlot->getSelectedSamples();
        std::set<unsigned long long> validTimes;
        for(const auto& p : validSamples) {
            // Convert back to nanoseconds
            validTimes.insert(static_cast<unsigned long long>(std::round(p.x())));
        }

        long double prev_start = -1.L;
        long double offset = 0.L;

        for (auto& shot : this->m_trackingData->data.ranges)
        {
            if (shot.start_time < prev_start)
            {
                offset += 86400.L;
            }
            prev_start = shot.start_time;

            // Reconstruct the time key to match the plot data
            unsigned long long time = static_cast<unsigned long long>((shot.start_time + offset) * 1e9);

            if (validTimes.count(time)) {
                shot.flag = Tracking::RangeData::FilterFlag::DATA;
            } else {
                shot.flag = Tracking::RangeData::FilterFlag::NOISE;
            }
        }

        // 8. Perform the write operation
        TrackingFileManager::writeTrackingPrivate(this->m_trackingData->data, filePath);

        // -----------------------------
        // 9. Update internal path variable
        m_currentFilePath = filePath;
        ui ->le_filePath->setText(m_currentFilePath);
        onFilterSaved();
        DegorasInformation::showInfo("Filter Tool", "File saved successfully.", "", this);
        // -----------------------------
    }
}

void MainWindow::on_pb_calcStats_clicked()
{
    // 1. Validaciones
    if (!m_trackingData || !m_trackingData->dp_tracking) return;
    QVector<QPointF> v = ui->filterPlot->getSelectedSamples();
    if (v.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "Selecciona puntos verdes primero.");
        return;
    }

    // 2. Preparación
    std::set<qulonglong> selected;
    for(const auto& p : v) selected.insert(static_cast<qulonglong>(p.x()));

    // 3. Centrado Manual
    double sum_val = 0.0;
    int count_val = 0;
    long double min_time = 1e20, max_time = -1e20;

    for (const auto* echo : m_trackingData->listAll()) {
        if (selected.count(echo->time)) {
            sum_val += static_cast<double>(echo->difference);

            double t_sec = static_cast<double>(echo->time) * 1.0e-9;
            if (t_sec < min_time) min_time = t_sec;
            if (t_sec > max_time) max_time = t_sec;

            count_val++;
        }
    }
    double manual_mean = (count_val > 0) ? sum_val / count_val : 0.0;

    // 4. RELLENAR DATOS (A PRUEBA DE BALAS)
    dpslr::ilrs::algorithms::RangeDataV rd;
    rd.reserve(count_val);

    for (const auto* echo : m_trackingData->listAll()) {
        if (selected.count(echo->time)) {

            // Usamos el tipo exacto que espera el vector para evitar errores
            dpslr::ilrs::algorithms::RangeDataV::value_type item;

            // Asignación por NOMBRE (No por orden)
            item.ts = static_cast<long double>(echo->time) * 1.0e-9;
            item.resid = static_cast<double>(echo->difference) - manual_mean;

            // Rellenar restos con 0 para seguridad
            // (Si estos campos no existen en tu struct, borra estas líneas, pero resid y ts son seguros)
            // item.tof = 0; item.pred_dist = 0; item.trop_corr = 0;

            rd.push_back(item);
        }
    }

    // 5. BIN SIZE = DURACIÓN TOTAL
    int global_bin_size = static_cast<int>(max_time - min_time) + 100;

    // 6. Calcular
    dpslr::ilrs::algorithms::ResidualsStats resid;
    auto res_error = dpslr::ilrs::algorithms::calculateResidualsStats(global_bin_size, rd, resid);

    if (res_error != dpslr::ilrs::algorithms::ResiStatsCalcErr::NOT_ERROR) {
        DegorasInformation::showWarning("Statistics", "Error Code: " + QString::number((int)res_error), "", this);
        return;
    }

    // 7. Resultados
    auto stats = resid.total_bin_stats.stats_rfrms;

    double val_rms = static_cast<double>(stats.rms);
    double val_mean = manual_mean + static_cast<double>(stats.mean);
    double val_peak = static_cast<double>(stats.peak);
    double val_skew = static_cast<double>(stats.skew);
    double val_kurt = static_cast<double>(stats.kurt);

    double val_stderr = (stats.aptn > 0) ? (val_rms / std::sqrt(static_cast<double>(stats.aptn))) : 0.0;

    // SNR Simple: Peak / RMS
    double val_snr = (val_rms > 1e-9) ? (10.0 * std::log10(std::abs(val_peak / val_rms))) : 0.0;
    if (std::isnan(val_snr) || std::isinf(val_snr)) val_snr = 0.0;

    // 8. Pantalla
    ui->lbl_rms_ps->setText(QString::number(val_rms, 'f', 1));
    ui->lbl_mean_ps->setText(QString::number(val_mean, 'f', 1));
    ui->lbl_peak_ps->setText(QString::number(val_peak, 'f', 1));

    if(ui->lbl_stderr) ui->lbl_stderr->setText(QString::number(val_stderr, 'f', 2));
    if(ui->lbl_skew)   ui->lbl_skew->setText(QString::number(val_skew, 'f', 3));
    if(ui->lbl_kurt)   ui->lbl_kurt->setText(QString::number(val_kurt, 'f', 3));
    if(ui->lbl_snr)    ui->lbl_snr->setText(QString::number(val_snr, 'f', 2));

    ui->lbl_returns->setText(QString::number(resid.total_bin_stats.ptn));
    ui->lbl_echoes->setText(QString::number(stats.aptn));
    ui->lbl_noise->setText(QString::number(stats.rptn));

    // 9. Gráficas
    QVector<QPointF> error_points;
    const auto& mask = resid.total_bin_stats.amask_rfrms;
    for (int idx = mask.size() - 1; idx >= 0; idx--) {
        if (!mask[idx]) error_points.push_back(v.takeAt(idx));
    }

    ui->filterPlot->selected_curve->setSamples(v);
    ui->filterPlot->error_curve->setSamples(error_points);
    ui->filterPlot->replot();

    DegorasInformation::showInfo("Statistics", "Calculated.", "", this);
}

// adición MARIO: funcionamiento botón recalcular una vez cargado el CPF (esta función estaba pero vacía)
void MainWindow::on_pb_recalculate_clicked()
{
    if (!m_trackingData) return;

    // Cargar el Motor
    CPFPredictor predictor;


    if (!predictor.load(m_cpfPath)) {
        QMessageBox::critical(this, "Error", "Failed to initialize CPF Predictor with file:\n" + m_cpfPath);
        return;
    }

    // Recalcular cada punto
    QProgressDialog progress("Recalculating Orbit...", "Abort", 0, m_trackingData->listAll().size(), this);
    progress.setWindowModality(Qt::WindowModal);

    int count = 0;
    for (auto* echo : m_trackingData->listAll()) {
        if (progress.wasCanceled()) break;

        // Conversión de tiempo: echo->time suele ser ns del día o del pase
        // Asumimos acceso al "seconds of day" (SoD).
        // Si echo->time son nanosegundos del día:
        double sod = static_cast<double>(echo->time) * 1.0e-9;

        // LLAMADA AL MOTOR
        long long predicted_tof_ps = predictor.calculateTwoWayTOF(echo->mjd, sod);

        if (predicted_tof_ps > 0) {
            // Actualizamos el RESIDUO (Observed - Calculated)
            // echo->flight_time es el observado en ps
            echo->difference = echo->flight_time - predicted_tof_ps;
        }

        count++;
        if (count % 100 == 0) progress.setValue(count);
    }
    progress.setValue(m_trackingData->listAll().size());

    // 3. Refrescar todo
    updatePlots();
    onFilterChanged(); // Para recalcular estadísticas con los nuevos residuos

    ui->lbl_sessionID->setText("Recalculated: " + QFileInfo(m_cpfPath).fileName());
    DegorasInformation::showInfo("Recalculation", "Residuals updated using new CPF.", "", this);
}

//adición MARIO: cargar CPF
void MainWindow::on_pb_loadCPF_clicked()
{

    // 1. Get Last loaded CPF file path
    QSettings* settings = DegorasSettings::instance().config();
    const QString settingKey = "Paths/LastLoadCPFDir";

    QString storedDir;

    // Only read if settings initilized correctly
    if(settings)
    {
        storedDir = settings->value(settingKey).toString();
    }

    // 2. If setting directory is not empty and what's inside exists (it's not a deleted folder or a path in your system):
    if(storedDir.isEmpty() || !QDir(storedDir).exists())
    {   // Default path
        storedDir = QCoreApplication::applicationDirPath(); // Returns 'build' directory instead of 'workspace/DegorasProject/...'.
    }

    //QString storedDir = QDir::fromNativeSeparators("T:/builds/DegorasProjectLite-main/DeployData/data/SP_DataFiles/SP_CPF");

    QString filter = "CPF Files (*.cpf *.sgf *dgf *.npt *.tjr);;All Files (*)";
    QString dialogTitle = "Select CPF File";


    if (m_trackingData) {
        QString currentFileName = QFileInfo(m_trackingData->file_name).fileName();
        QStringList parts = currentFileName.split('_');


        if (parts.size() >= 4) {
            QString satID = parts[3];
            filter = QString("Satellite %1 (*%1*.cpf *%1*.npt *%1*.tjr);;All Files (*)").arg(satID);
            dialogTitle = "Select CPF for Satellite " + satID;
            qDebug() << "Auto-filter for Satellite ID:" << satID;
        }
    }

    QString path = QFileDialog::getOpenFileName(this,
                                                dialogTitle,
                                                storedDir,
                                                filter);

    // 3. Load Data
    if (!path.isEmpty()) {
        m_cpfPath = path;
        ui->le_cpfPath->setText(path);
        ui->pb_recalculate->setEnabled(true);

        // --- 5. Save path for next use ---
        QString newDir = QFileInfo(path).absolutePath();
        settings->setValue(settingKey, newDir);
        settings->sync();
        // ---------------------------------
    }
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

//Adición Mario, clickar estadísticas

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Solo nos interesa si es un Clic de Ratón (al soltar el botón)
    if (event->type() == QEvent::MouseButtonRelease) {

        // Intentamos ver si el objeto clickado es un QLabel
        QLabel *label = qobject_cast<QLabel*>(obj);

        if (label) {
            QString text = label->text();

            // Solo copiamos si no es el texto por defecto "..."
            if (text != "..." && !text.isEmpty()) {

                // 1. Copiar al portapapeles del sistema
                QApplication::clipboard()->setText(text);

                // 2. Feedback visual (Tooltip temporal)
                QToolTip::showText(QCursor::pos(), "Copied: " + text, label);

                return true; // Indicamos que ya hemos manejado el evento
            }
        }
    }

    // Para cualquier otro evento, dejamos que Qt haga lo normal
    return QMainWindow::eventFilter(obj, event);
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

void MainWindow::on_selectNavMode_clicked()
{
    ui->filterPlot->setSelectionMode();
    ui->histogramPlot->setSelectionMode();
    ui->removeNavMode->setChecked(false);
}


void MainWindow::on_removeNavMode_clicked()
{
    ui->filterPlot->setDeletionMode();
    ui->histogramPlot->setDeletionMode();
    ui->selectNavMode->setChecked(false);
}


void MainWindow::on_undoButton_clicked()
{
    bool undo_status = ui->filterPlot->undo();
    ui->histogramPlot->undo();
    if(!undo_status)
        DegorasInformation::showInfo("Filter Tool", "No more actions to undo.", "", this);
}


void MainWindow::on_redoButton_clicked()
{
    bool redo_status = ui->filterPlot->redo();
    ui->histogramPlot->redo();
    if(!redo_status)
        DegorasInformation::showInfo("Filter Tool", "No more actions to undo.", "", this);
}


void MainWindow::on_deselectButton_clicked()
{
    ui->filterPlot->resetSelection();
    ui->histogramPlot->resetSelection();
}




