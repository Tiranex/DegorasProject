#pragma once

#include <QMainWindow>
#include <QCloseEvent>

// Forward declarations
namespace Ui { class MainWindow; }
class TrackingData;
class Plot;
class ErrorPlot;
class CPF;

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Slots for UI actions from .ui file (auto-connected)
    void on_pb_load_clicked();
    void on_pb_recalculate_clicked();
    void on_pb_autoFilter_clicked();
    void on_pb_rmsFilter_clicked();
    void on_pb_calcStats_clicked();
    void on_actionLoad_triggered();
    void on_actionSave_triggered();
    void on_actionDiscard_triggered();
    
    // Custom slots for plot interactions
    void onPlotSelectionChanged();
    void onPlotPickingStarted();
    void onPlotPickingFinished();
    void onHistogramPickingStarted();
    void onHistogramPickingFinished();
    
    // Sync plot navigation
    void syncPanFromMainPlot();
    void syncPanFromHistogramPlot();
    
    // Slot to update status
    void onFilterChanged();
    void onFilterSaved();


private:
    void setupConnections();
    void loadTrackingData(const QString& filePath);
    void updateUIState(bool hasData);
    void updatePlots();
    void clearStatistics();
    
    int threshFilter();

    Ui::MainWindow *ui;
    TrackingData* m_trackingData;
    QString m_currentFilePath;
    bool m_isChanged;
};
