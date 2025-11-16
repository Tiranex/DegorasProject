#pragma once

#include "tracking.h"
#include "../window_message_box.h"
#include "../dpcore_global.h"

class DP_CORE_EXPORT TrackingFileManager
{
public:
    enum ErrorEnum
    {
        TRACKFILE_NOT_OPEN,
        TRACKFILE_INVALID,
        TRACKFILE_NOT_EXISTS,
        TRACKFILE_NOT_REMOVABLE
    };

    static const QMap<ErrorEnum, QString> ErrorListStringMap;

    static DegorasInformation readTracking(const QString& track_name, const QString &track_dirpath,
                                          const QString &calib_dirpath, Tracking& track);
    static DegorasInformation readTracking(const QString& track_name, const QString &track_dirpath, Tracking& track);
    static DegorasInformation readTracking(const QString& track_name, Tracking& track);
    static DegorasInformation readTrackingDir(const QString& dir, const QString& calib_path,
                                             std::vector<Tracking>& tracks);
    static DegorasInformation readTrackingDir(const QString& dir, std::vector<Tracking>& tracks);
    static DegorasInformation writeTracking(const Tracking& track, const QString& dest_dir = "",
                                           const QString& filename = "");
    static DegorasInformation removeTracking(const QString& track_name);
    static DegorasInformation removeCurrentTracking(const QString& track_name);

    static QString trackingFilename(const Tracking &track);
    static QString findTracking(const QString &track_name);
    static QStringList findTrackings(const QDateTime &start, const QDateTime &end, const QString &object_norad = "",
                                     const QString &cfg_id = "", const QString& dir = "");
    static QStringList currentTrackings(const QString &object_norad = "", const QString &cfg_id = "");

    static QDate startDate(const QString &track_name);
    static DegorasInformation readTrackingFromFile(const QString &file_path, const QString &calib_path, Tracking& track);
    static DegorasInformation writeTrackingPrivate(const Tracking& track, const QString& filepath);
};

