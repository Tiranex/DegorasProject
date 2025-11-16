#include "Tracking/trackingfilemanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>

#include "Tracking/calibrationfilemanager.h"
#include "degoras_settings.h"
#include "LibDegorasBase/Helpers/string_helpers.h"

const QString kDateStartKey = QStringLiteral("date_start");
const QString kDateEndKey = QStringLiteral("date_end");
const QString kFilterModeKey = QStringLiteral("filter_mode");
const QString kCfgIdKey = QStringLiteral("cfg_id");
const QString kStationNameKey = QStringLiteral("station_name");
const QString kStationIdKey = QStringLiteral("station_id");
const QString kObjNameKey = QStringLiteral("obj_name");
const QString kObjNoradKey = QStringLiteral("obj_norad");
const QString kObjBSKey = QStringLiteral("obj_bs");
const QString kRFKey = QStringLiteral("rf");
const QString kNShotsKey = QStringLiteral("nshots");
const QString kRNShotsKey = QStringLiteral("rnshots");
const QString kUNShotsKey = QStringLiteral("unshots");
const QString kTRORRFRMSKey = QStringLiteral("tror_rfrms");
const QString kTROR1RMSKey = QStringLiteral("tror_1rms");
const QString kReleaseKey = QStringLiteral("rel");
const QString kEphemerisKey = QStringLiteral("ephemeris");
const QString kStatsRFRMSKey = QStringLiteral("stats_rfrms");
const QString kStats1RMSKey = QStringLiteral("stats_1rms");
const QString kOverallCalKey = QStringLiteral("cal_val_overall");
const QString kCalDataKey = QStringLiteral("cal_data");
const QString kCalFileKey = QStringLiteral("file");
const QString kCalSpanKey = QStringLiteral("span");
const QString kMeteoKey = QStringLiteral("meteo_data");
const QString kRangesKey = QStringLiteral("ranges_data");
const QString kFlagKey = QStringLiteral("flag");
const QString kStartKey = QStringLiteral("start");
const QString kToFKey = QStringLiteral("tof_2w");
const QString kPredKey = QStringLiteral("pre_2w");
const QString kTropCorrKey = QStringLiteral("trop_corr_2w");
const QString kBiasKey = QStringLiteral("bias");
const QString kEtKey = QStringLiteral("et_data");
const QString kTAKey = QStringLiteral("tA");
const QString kTBKey = QStringLiteral("tB");
const QString kETPrecisionKey = QStringLiteral("precision");

const QMap<TrackingFileManager::ErrorEnum, QString> TrackingFileManager::ErrorListStringMap =
{
    {TrackingFileManager::ErrorEnum::TRACKFILE_INVALID,
     "The tracking json file %1 is not valid."},
    {TrackingFileManager::ErrorEnum::TRACKFILE_NOT_OPEN,
     "The tracking json file %1 could not be opened."},
    {TrackingFileManager::ErrorEnum::TRACKFILE_NOT_EXISTS,
     "The tracking json file %1 does not exist."},
    {TrackingFileManager::ErrorEnum::TRACKFILE_NOT_REMOVABLE,
     "The tracking json file %1 could not be removed."},
};

// TODO: long double?
// TODO: validation of values?
DegorasInformation TrackingFileManager::readTracking(const QString &track_name, const QString &track_dirpath,
                                                    const QString &calib_dirpath, Tracking &track)
{   
    return track_dirpath.isEmpty() ? TrackingFileManager::readTracking(track_name, track) :
                                  TrackingFileManager::readTrackingFromFile(track_dirpath + '/' + track_name,
                                                                            calib_dirpath, track);
}

DegorasInformation TrackingFileManager::readTracking(const QString &track_name, const QString &track_dirpath,
                                                    Tracking &track)
{
    return TrackingFileManager::readTrackingFromFile(track_dirpath + '/' + track_name, {}, track);
}

DegorasInformation TrackingFileManager::readTracking(const QString &track_name, Tracking &track)
{

    QString file_path = TrackingFileManager::findTracking(track_name);
    DegorasInformation result;
    if (file_path.isEmpty())
        result = {{TrackingFileManager::ErrorEnum::TRACKFILE_NOT_OPEN,
                   ErrorListStringMap[ErrorEnum::TRACKFILE_NOT_OPEN].arg(track_name)}};
    else
        result = TrackingFileManager::readTrackingFromFile(file_path, {}, track);

    return result;
}

DegorasInformation TrackingFileManager::writeTracking(const Tracking& track,
                                                     const QString& dest_dir,
                                                     const QString& filename )
{
    DegorasInformation errors;

    // If filename is empty, then use default
    QString filename_selected = filename.isEmpty() ? TrackingFileManager::trackingFilename(track) : filename;

    if (dest_dir.isEmpty())
    {
        QString current_path = DegorasSettings::instance().getGlobalConfigString(
                    "SalaraProjectDataPaths/SP_CurrentObservations");
        QString hist_path = DegorasSettings::instance().getGlobalConfigString(
                    "SalaraProjectDataPaths/SP_HistoricalObservations") + '/' + track.date_start.date().toString("yyyyMMdd");

        errors = TrackingFileManager::writeTrackingPrivate(track, current_path + '/' + filename_selected);

        if (QDir().mkpath(hist_path))
            errors.append(TrackingFileManager::writeTrackingPrivate(track, hist_path + '/' + filename_selected));
        else
            errors.append({{0, "Cannot create historical path: " + hist_path}});
    }
    else
    {
        errors = TrackingFileManager::writeTrackingPrivate(track, dest_dir + '/' + filename_selected);
    }

    return errors;


}

DegorasInformation TrackingFileManager::removeTracking(const QString &track_name)
{
    DegorasInformation errors;
    QDate date_start(TrackingFileManager::startDate(track_name));
    QString current_filepath = DegorasSettings::instance().getGlobalConfigString(
                "SalaraProjectDataPaths/SP_CurrentObservations") + '/' + track_name;
    QString hist_filepath = DegorasSettings::instance().getGlobalConfigString(
                "SalaraProjectDataPaths/SP_HistoricalObservations") + '/' +
            date_start.toString("yyyyMMdd") + '/' + track_name;


    if (QFile::exists(current_filepath))
    {
        if (!QFile::remove(current_filepath))
            errors.append({{TrackingFileManager::TRACKFILE_NOT_REMOVABLE,
                            TrackingFileManager::ErrorListStringMap[TRACKFILE_NOT_EXISTS].arg(current_filepath)}});
    }
    else
        errors.append({{TrackingFileManager::TRACKFILE_NOT_EXISTS,
                        TrackingFileManager::ErrorListStringMap[TRACKFILE_NOT_EXISTS].arg(current_filepath)}});

    if (QFile::exists(hist_filepath))
    {
        if (!QFile::remove(hist_filepath))
            errors.append({{TrackingFileManager::TRACKFILE_NOT_REMOVABLE,
                            TrackingFileManager::ErrorListStringMap[TRACKFILE_NOT_EXISTS].arg(hist_filepath)}});
    }
    else
        errors.append({{TrackingFileManager::TRACKFILE_NOT_EXISTS,
                        TrackingFileManager::ErrorListStringMap[TRACKFILE_NOT_EXISTS].arg(hist_filepath)}});

    return errors;

}

DegorasInformation TrackingFileManager::removeCurrentTracking(const QString &track_name)
{
    DegorasInformation errors;
    QString current_filepath = DegorasSettings::instance().getGlobalConfigString(
                "SalaraProjectDataPaths/SP_CurrentObservations") + '/' + track_name;


    if (QFile::exists(current_filepath))
    {
        if (!QFile::remove(current_filepath))
            errors.append({{TrackingFileManager::TRACKFILE_NOT_REMOVABLE,
                            TrackingFileManager::ErrorListStringMap[TRACKFILE_NOT_EXISTS].arg(current_filepath)}});
    }
    else
        errors.append({{TrackingFileManager::TRACKFILE_NOT_EXISTS,
                        TrackingFileManager::ErrorListStringMap[TRACKFILE_NOT_EXISTS].arg(current_filepath)}});


    return errors;
}

DegorasInformation TrackingFileManager::readTrackingDir(const QString &dir, const QString& calib_path,
                                                       std::vector<Tracking> &tracks)
{
    Tracking t;
    DegorasInformation result;
    for (auto&& file : QDir(dir).entryInfoList({"*.dptr"}, QDir::Files))
    {
        auto errors = TrackingFileManager::readTracking(file.fileName(), file.canonicalPath(), calib_path, t);
        result.append(errors);
        if (!errors.hasError())
            tracks.push_back(t);
    }
    return result;
}

DegorasInformation TrackingFileManager::readTrackingDir(const QString &dir, std::vector<Tracking> &tracks)
{
    return TrackingFileManager::readTrackingDir(dir, {}, tracks);
}

QString TrackingFileManager::trackingFilename(const Tracking &track)
{
    return QString::number(track.station_id) + '_' + track.cfg_id + '_' + track.date_start.toString("yyyyMMddhhmm") +
            '_' + track.obj_norad + '_' + QString("%1").arg(track.release, 2, 10, QChar('0')) + ".dptr";
}

QString TrackingFileManager::findTracking(const QString &track_name)
{
    QString result;
    QDate date_start = TrackingFileManager::startDate(track_name);

    if (date_start.isValid())
    {
        QString hist_trackpath =
                DegorasSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_HistoricalObservations");

        result = hist_trackpath + "/" + date_start.toString("yyyyMMdd") + '/' + track_name;

        if (!QFile::exists(result))
            result = QString();
    }

    return result;
}

QStringList TrackingFileManager::findTrackings(const QDateTime &start, const QDateTime &end,
                                               const QString &object_norad, const QString &cfg_id, const QString &dir)
{
    QStringList trackings_selected;
    QString hist_trackpath = dir.isEmpty() ?
            DegorasSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_HistoricalObservations") : dir;
    for (auto date = start.date(); date <= end.date(); date = date.addDays(1))
    {
        QString date_folder(date.toString("yyyyMMdd"));
        QDir dir(hist_trackpath + '/' + date_folder);
        QStringList files = dir.entryList(QDir::Files);

        for (const auto& file : std::as_const(files))
        {
            QStringList name_tokens = file.split('_');
            if (name_tokens.size() > 3)
            {
                bool include = true;
                if (!object_norad.isEmpty() && object_norad != name_tokens[3])
                    include = false;
                if (include && !cfg_id.isEmpty() && cfg_id != name_tokens[1])
                    include = false;

                if (include)
                {
                    Tracking tracking;
                    TrackingFileManager::readTrackingFromFile(dir.path() + '/' + file, {}, tracking);
                    // Include file if it contains at least an interval within start-end
                    include = tracking.date_end >= start && tracking.date_start <= end;
                }
                if (include)
                    trackings_selected.push_back(file);

            }
        }   
    }

    return trackings_selected;
}

QStringList TrackingFileManager::currentTrackings(const QString &object_norad, const QString &cfg_id)
{
    QStringList trackings_selected;
    QString current_trackpath =
            DegorasSettings::instance().getGlobalConfigString("SalaraProjectDataPaths/SP_CurrentObservations");

    QStringList files = QDir(current_trackpath).entryList(QDir::Files);

    for (const auto& file : std::as_const(files))
    {
        QStringList name_tokens = file.split('_');
        if (name_tokens.size() > 3)
        {
            bool include = true;
            if (!object_norad.isEmpty() && object_norad != name_tokens[3])
                include = false;
            if (!include && !cfg_id.isEmpty() && cfg_id != name_tokens[1])
                include = false;

            if (include)
                trackings_selected.push_back(file);
        }
    }

    return trackings_selected;
}

QDate TrackingFileManager::startDate(const QString &track_name)
{
    QStringList splitted = track_name.split('_');
    QDate date;
    if (splitted.size() > 2)
    {
        QString date_string;
        if (12 == splitted[2].size())
            date_string = splitted[2].left(8);
        else if (10 == splitted[2].size())
            date_string = "20" + splitted[2].left(6);
        date = QDate::fromString(date_string, "yyyyMMdd");
    }
    return date;
}

DegorasInformation TrackingFileManager::readTrackingFromFile(const QString &file_path, const QString &calib_path, Tracking &track)
{
    QFile track_file(file_path);
    // Check if file could be opened.
    if(!track_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return DegorasInformation({TrackingFileManager::ErrorEnum::TRACKFILE_NOT_OPEN,
                                  ErrorListStringMap[ErrorEnum::TRACKFILE_NOT_OPEN].arg(file_path)});

    // Read all.
    QString track_string = track_file.readAll().simplified();
    track_file.close();
    // Loads the json document.
    QJsonDocument track_jsondocument = QJsonDocument::fromJson(track_string.toUtf8());

    // Check if scheme is valid
    DegorasInformation errors;

    // Check if data file is valid
    if (track_jsondocument.isNull())
    {
        errors.append({{ErrorEnum::TRACKFILE_INVALID, ErrorListStringMap[ErrorEnum::TRACKFILE_INVALID].arg(file_path)}});
    }
    else
    {
        // Ensure object is cleared
        track = Tracking();

        // Data
        track.date_start = QDateTime::fromString(track_jsondocument[kDateStartKey].toString(), Qt::ISODateWithMs);
        track.date_end = QDateTime::fromString(track_jsondocument[kDateEndKey].toString(), Qt::ISODateWithMs);
        track.filter_mode = static_cast<Tracking::FilterMode>(track_jsondocument[kFilterModeKey].toInt());
        track.station_name = track_jsondocument[kStationNameKey].toString();
        track.station_id = track_jsondocument[kStationIdKey].toInt();
        track.cfg_id = track_jsondocument[kCfgIdKey].toString();
        track.obj_name = track_jsondocument[kObjNameKey].toString();
        track.obj_norad = track_jsondocument[kObjNoradKey].toString();
        track.obj_bs = track_jsondocument[kObjBSKey].toInt();
        track.rf = track_jsondocument[kRFKey].toDouble();
        track.nshots = track_jsondocument[kNShotsKey].toInt();
        track.rnshots = track_jsondocument[kRNShotsKey].toInt();
        track.unshots = track_jsondocument[kUNShotsKey].toInt();
        track.tror_rfrms = track_jsondocument[kTRORRFRMSKey].toDouble();
        track.tror_1rms = track_jsondocument[kTROR1RMSKey].toDouble();
        track.release = track_jsondocument[kReleaseKey].toInt();
        track.ephemeris_file = track_jsondocument[kEphemerisKey].toString();

        // Stats
        track.stats_rfrms = StatsFromJson(track_jsondocument[kStatsRFRMSKey].toObject());
        track.stats_1rms = StatsFromJson(track_jsondocument[kStats1RMSKey].toObject());

        // Meteo
        for (auto&& meteo : track_jsondocument[kMeteoKey].toArray())
        {
            track.meteo_data.push_back(MeteoData::fromJson(meteo.toObject()));
        }

        // Calibration data and overall value
        track.cal_val_overall = track_jsondocument[kOverallCalKey].toDouble();
        QJsonArray array = track_jsondocument[kCalDataKey].toArray();
        for (const auto& elem : std::as_const(array))
        {
            QJsonObject obj = elem.toObject();
            Calibration calib;
            DegorasInformation e = calib_path.isEmpty() ?
                        CalibrationFileManager::readCalibration(obj[kCalFileKey].toString(), calib) :
                        CalibrationFileManager::readCalibration(obj[kCalFileKey].toString(), calib_path, calib);

            if (e.hasError())
                errors.append(e);
            else
            {
                Tracking::CalibrationSpan span = static_cast<decltype(span)>(obj[kCalSpanKey].toInt());
                track.cal_data[span][calib.date_start] = calib;
            }
        }

        // Ranges
        array = track_jsondocument[kRangesKey].toArray();
        for (const auto& elem : std::as_const(array))
        {
            QJsonObject obj = elem.toObject();
            Tracking::RangeData range;
            // TODO: check values of enum
            range.flag = static_cast<Tracking::RangeData::FilterFlag>(obj[kFlagKey].toInt());
            try
            {
                range.start_time = std::stold(obj[kStartKey].toString().toStdString());
            }
            catch (...)
            {
                range.start_time = 0.L;
            }

            range.tof_2w = obj[kToFKey].toDouble();
            range.pre_2w = obj[kPredKey].toDouble();
            range.trop_corr_2w = obj[kTropCorrKey].toDouble();
            range.bias = obj[kBiasKey].toDouble();
            track.ranges.push_back(range);
        }

        // TODO: include fail for failed converssions?
        // ET
        QJsonObject et_object = track_jsondocument[kEtKey].toObject();
        if (!et_object.empty())
        {
            array = et_object[kTAKey].toArray();
            for (const auto& elem : std::as_const(array))
            {
                try
                {
                    track.tA.push_back(std::stold(elem.toString().toStdString()));
                }
                catch(...){}
            }

            array = et_object[kTBKey].toArray();
            for (const auto& elem : std::as_const(array))
            {
                try
                {
                    track.tB.push_back(std::stold(elem.toString().toStdString()));
                }
                catch(...){}
            }

            track.et_precision = et_object[kETPrecisionKey].toInt();
        }

        // TODO telescope
    }

    // Return the errors
    return errors;
}

DegorasInformation TrackingFileManager::writeTrackingPrivate(const Tracking &track, const QString &filepath)
{

    QFile track_file(filepath);
    // Check if file could be opened to write.
    if(!track_file.open(QIODevice::WriteOnly | QIODevice::Text))
        return DegorasInformation({TrackingFileManager::ErrorEnum::TRACKFILE_NOT_OPEN,
                                  ErrorListStringMap[ErrorEnum::TRACKFILE_NOT_OPEN].arg(filepath)});

    QJsonObject track_object;

    // Data
    track_object.insert(kDateStartKey, track.date_start.toString(Qt::ISODateWithMs));
    track_object.insert(kDateEndKey, track.date_end.toString(Qt::ISODateWithMs));
    track_object.insert(kFilterModeKey, static_cast<int>(track.filter_mode));
    track_object.insert(kCfgIdKey, track.cfg_id);
    track_object.insert(kStationNameKey, track.station_name);
    track_object.insert(kStationIdKey, static_cast<int>(track.station_id));
    track_object.insert(kObjNameKey, track.obj_name);
    track_object.insert(kObjNoradKey, track.obj_norad);
    track_object.insert(kObjBSKey, static_cast<int>(track.obj_bs));
    track_object.insert(kRFKey, track.rf);
    track_object.insert(kNShotsKey, static_cast<int>(track.nshots));
    track_object.insert(kRNShotsKey, static_cast<int>(track.rnshots));
    track_object.insert(kUNShotsKey, static_cast<int>(track.unshots));
    track_object.insert(kTRORRFRMSKey, track.tror_rfrms);
    track_object.insert(kTROR1RMSKey, track.tror_1rms);
    track_object.insert(kReleaseKey, static_cast<int>(track.release));
    track_object.insert(kEphemerisKey, track.ephemeris_file);

    // Meteo
    QJsonArray meteo_array;
    std::transform(track.meteo_data.begin(), track.meteo_data.end(),
                   std::back_inserter(meteo_array),[](const auto& m){return m.toJson();});
    track_object.insert(kMeteoKey, meteo_array);

    // Stats
    track_object.insert(kStatsRFRMSKey, StatstoJson(track.stats_rfrms));
    track_object.insert(kStats1RMSKey, StatstoJson(track.stats_1rms));

    // Calibration data and overall value
    track_object.insert(kOverallCalKey, track.cal_val_overall);
    QJsonArray array;
    for (const auto& span_pair : std::as_const(track.cal_data))
    {
        for (const auto& cal_pair : std::as_const(span_pair.second))
        {
            QJsonObject o;
            o[kCalFileKey] = CalibrationFileManager::calibrationFilename(cal_pair.second);
            o[kCalSpanKey] = static_cast<int>(span_pair.first);
            array.push_back(o);
        }
    }
    track_object.insert(kCalDataKey, array);

    // Ranges
    array = {};
    for (const auto& elem : track.ranges)
    {
        QJsonObject obj;
        // TODO: check values of enum
        obj.insert(kFlagKey, static_cast<int>(elem.flag));
        obj.insert(kStartKey, QString::fromStdString(dpbase::helpers::strings::numberToStr(elem.start_time,17,12)));
        obj.insert(kToFKey, elem.flag == Tracking::RangeData::FilterFlag::UNKNOWN ?
                       QJsonValue() : static_cast<long long>(elem.tof_2w));
        obj.insert(kPredKey, elem.flag == Tracking::RangeData::FilterFlag::UNKNOWN ?
                       QJsonValue() : static_cast<long long>(elem.pre_2w));
        obj.insert(kTropCorrKey,
                   elem.flag == Tracking::RangeData::FilterFlag::UNKNOWN ?
                       QJsonValue() : static_cast<long long>(elem.trop_corr_2w));
        obj.insert(kBiasKey, elem.flag == Tracking::RangeData::FilterFlag::UNKNOWN ? QJsonValue() : elem.bias);
        array.push_back(obj);
    }
    track_object.insert(kRangesKey, array.empty() ? QJsonValue() : array);

    // ET
    QJsonObject et_object;
    array = {};
    std::transform(track.tA.begin(), track.tA.end(), std::back_inserter(array),
                   [](const auto& a){return QString::fromStdString(dpbase::helpers::strings::numberToStr(a, 18, 12));});
    if (!array.empty())
        et_object.insert(kTAKey, array);

    array = {};
    std::transform(track.tB.begin(), track.tB.end(), std::back_inserter(array),
                   [](const auto& a){return QString::fromStdString(dpbase::helpers::strings::numberToStr(a, 18, 12));});
    if (!array.empty())
        et_object.insert(kTBKey, array);

    if (!et_object.empty())
        et_object.insert(kETPrecisionKey, static_cast<int>(track.et_precision));

    // Only insert ET precission if there is tA or tB
    track_object.insert(kEtKey, et_object.empty() ? QJsonValue() : et_object);

    // TODO telescope

    QJsonDocument track_jsondocument(track_object);
    track_file.write(track_jsondocument.toJson(QJsonDocument::Indented));
    track_file.close();

    // Return the errors
    return {};
}
