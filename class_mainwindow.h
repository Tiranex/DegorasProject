#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QFile>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void openFile();
    void applyWindow();
    void applyPrefilter();
    void applyPostfilter();
    void removeNoise();
    void detrend();
    void applyStatisticalFilter();

private:
    Ui::MainWindow* ui;
    std::vector<double> times;
    std::vector<double> resids;
    std::vector<double> times_win;
    std::vector<double> resids_win;
    std::vector<double> times_pre;
    std::vector<double> resids_pre;
    std::vector<double> times_post;
    std::vector<double> resids_post;

    std::vector<double> detrend_resids;
    std::vector<double> detrend_resids_data;

};

