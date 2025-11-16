#pragma once

#include "calibration.h"
#include "../window_message_box.h"
#include "../dpcore_global.h"

class QFile;

class DP_CORE_EXPORT CalibrationFileManager
{
public:
    enum ErrorEnum
    {
        CALIBFILE_NOT_OPEN,
        CALIBFILE_INVALID,
        CALIB_NOT_FOUND
    };

    static const QMap<ErrorEnum, QString> ErrorListStringMap;

    static DegorasInformation readCalibration(const QString& cal_name, const QString &dir_path, Calibration& calib);
    static DegorasInformation readCalibration(const QString& cal_name, Calibration& calib);
    static DegorasInformation readCalibrationDir(const QString& dir, std::vector<Calibration>& calibs);
    static DegorasInformation readLastCalib(Calibration &calib);
    static DegorasInformation writeCalibration(const Calibration& calib, const QString& dest_dir,
                                              const QString& dest_file = "" );
    static QString calibrationFilename(const Calibration &calib);
    static QString findCalibration(const QString &calib_name);

    static QDate startDate(const QString &calib_name);
    static QDateTime startDateTime(const QString &calib_name);

private:
    static DegorasInformation readCalibrationFromFile(const QString &filepath, Calibration& calib);
};
