#pragma once

//=========== GENERAL NAMES ============================================================================================
#define NAME_ORGANIZATION "DEGORAS PROJECT TEAM"
#define NAME_PROJECT "DEGORAS PROJECT"
#define NAME_PROJECT_SHORT "DP"
//======================================================================================================================
#define PASSWORD_WRONG "The inserted password is wrong."

//=========== FILES ====================================================================================================
// Global, licenses, and others.
#define FILE_GLOBALCONFIG "SP_GlobalConfig.ini"
#define FILE_GLOBALCONFIGLINK "SP_GlobalConfigLink.ini"
#define FILE_DEGORASPROJECTLICENSE ":DP_License"
#define FILE_STATIONDATA "SP_StationData.json"
#define FILE_LASERDATA "SP_LasersData.json"
#define FILE_EOP "DP_EOPData.json"
// Space Objects.
#define FILE_SPACEOBJECTSDATA "SP_SpaceObjectsData.json"
#define FILE_SPACEOBJECTSSCHEME "SP_SpaceObjectsScheme.json"
#define FILE_SPACEOBJECTSSETS "SP_SPaceObjectsSets.json"
// Predictions.
#define FILE_PREDICTIONSSCHEME "SP_PredictionsScheme.json"
#define FILE_PREDICTIONSDATA "SP_CurrentPredictionsData.json"
#define FILE_HISTORICALPREDICTIONSDATA "SP_HistoricalPredictionsData.json"
// Star Catalog.
#define FILE_STARCATALOGDATA "DP_StarCatalogData.json"
#define FILE_STARCATALOGSCHEME "DP_StarCatalogScheme.json"
#define FILE_STARCATALOGSETS "DP_StarCatalogSets.json"
// Star Observations.
#define FILE_CURRENT_STAR_OBSERVATIONS "DP_CurrentStarObservations.dpso"
// Internal.
#define FILE_SALARAPROJECTABOUT ":/SP_About"
#define FILE_STYLESHEET_DEFAULT ":/stylesheet_default.qss"
//======================================================================================================================

//=========== DEGORAS PROJECT LOGOS ====================================================================================
#define LOGO_DEGORASPROJECT_H ":/DP_logos/DEGORAS PROJECT - HORIZONTAL [vF].png"
#define LOGO_DEGORASPROJECT_V ":/DP_logos/DEGORAS PROJECT - VERTICAL [vF].png"
#define LOGO_SPACEOBJECTSMANAGER ":/DP_logos/SPACE OBJECTS MANAGER [vF].png"
#define LOGO_PREDICTIONSGENERATOR ":/DP_logos/PREDICTIONS GENERATOR [vF].png"
#define LOGO_CPFFILESMANAGER ":/DP_logos/CPF FILES MANAGER [vF].png"
#define LOGO_RANGEGATEGENERATORMANAGER ":/DP_logos/RANGE GATE GENERATOR MANAGER [vF].png"
#define LOGO_STATIONCONTROL ":/DP_logos/STATION CONTROL [vF].png"
#define LOGO_TRACKINGSYSTEM ":/DP_logos/STATION CONTROL [vF].png"
#define LOGO_ENVIRONMENTALMONITOR ":/DP_logos/ENVIRONMENTAL MONITOR [vF].png"

//=========== DEGORAS PROJECT ICONS ====================================================================================
#define ICON_SPACEOBJECTSMANAGER ":/DP_icons/SPACE OBJECTS MANAGER [vF].png"
#define ICON_PREDICTIONSGENERATOR ":/DP_icons/PREDICTIONS GENERATOR [vF].png"
#define ICON_CPFFILESMANAGER ":/DP_icons/CPF FILES MANAGER [vF].png"
#define ICON_RANGEGATEGENERATORMANAGER ":/DP_icons/RANGE GATE GENERATOR MANAGER [vF].png"
#define ICON_STATIONCONTROL ":/DP_icons/STATION CONTROL [vF].png"
#define ICON_TRACKINGSYSTEM ":/DP_icons/STATION CONTROL [vF].png"
#define ICON_ENVIRONMENTALMONITOR ":/DP_icons/ENVIRONMENTAL MONITOR [vF].png"
#define ICON_RESULTSMANAGER ":/DP_icons/PREDICTIONS GENERATOR [vF].png"
#define ICON_POINTINGMODELGENERATOR ":/DP_icons/SPACE OBJECTS MANAGER [vF].png"
// Others.
#define ICON_CLOCKWISE ":/icons/cw_icon&48.png"
#define ICON_COUNTERCLOCKWISE ":/icons/ccw_icon&48.png"
//======================================================================================================================

//=========== TEXTS ====================================================================================================
// GENERIC
#define TEXT_ERROR_GENERIC "An error has occurred. For more details, click 'Show Details...' button."
#define TEXT_ERRORS_GENERIC "Errors have occurred. For more details, click 'Show Details...' button."
#define TEXT_ERROR_PLUGINS "Errors have occurred during plugins loading. For more details, click 'Show Details...' button."
#define TEXT_ERROR_PLUGIN_ALREADY_LOADED "A plugin with same name and versión is already loaded."
#define TEXT_ERROR_PLUGIN_GENERIC_CAST_FAIL "Unable to obtain SPPlugin object from input plugin."
#define TEXT_ERROR_PLUGIN_SPECIFIC_CAST_FAIL "Unable to obtain object derived from SPPlugin."

#define TEXT_WAITING_USER_INPUT "Waiting user input..."
#define TEXT_BACKGROUND_TASK "Background tasks still in progress. Please wait until all jobs finished."
#define TEXT_NO_ENABLED_SO "No space objects enabled loaded."

#define TEXT_EXIT_QUESTION_WITH_TASKS "Background tasks still in progress. Do you want to stop them and exit?"
#define TEXT_ACTION_COPY_NORAD "Copy NORAD"
#define TEXT_ACTION_COPY_ILRSID "Copy ILRS ID"
#define TEXT_ACTION_COPY_NAME "Copy Name"
#define TEXT_ACTION_COPY_COSPAR "Copy COSPAR"
#define TEXT_ACTION_COPY_ILRSNAME "Copy ILRS Name"
#define TEXT_TABLE_FRD_DATA "Full Rate File Data"
#define TEXT_TABLE_NPT_DATA "Normal Point File Data"


// SPACE OBJECT MANGAGER.
#define NAME_SPACEOBJECTMANAGER "SPACE OBJECTS MANAGER"
#define ERROR_SPACEOBJECTMANAGER NAME_PROJECT_SHORT " - " NAME_SPACEOBJECTMANAGER " | ERROR"
#define WARNING_SPACEOBJECTMANAGER NAME_PROJECT_SHORT " - " NAME_SPACEOBJECTMANAGER " | WARNING"
#define INFO_SPACEOBJECTMANAGER NAME_PROJECT_SHORT " - " NAME_SPACEOBJECTMANAGER " | INFO"
#define NAME_SPACEOBJECTSMANAGERCONFIGFILE "SP_SpaceObjectsManagerConfig.ini"
#define NAME_SPACEOBJECTSMANAGER_ABOUT "SpaceObjectsManagerAbout.json"
#define NAME_SPACEOBJECTSMANAGER_LICENSE "GPLv3.txt"
#define DELETE_SET_WARNING(NAME) "The set \"" + NAME + "\" will be deleted, for more details, click 'Show Details...' button. Are you sure?"
#define NO_CURRENT_SET "There is no existing current set defined. Disabling all objects by default."
#define NORADS_MISSING_MESSAGE "Some NORADs were not found at set loading (check them by clicking 'Show Details...')."


// CPF MANAGER
#define NAME_CPFMANAGER "CPF FILES MANAGER"
#define ERROR_CPFMANAGER NAME_PROJECT_SHORT " - " NAME_CPFMANAGER " | ERROR"
#define WARNING_CPFMANAGER NAME_PROJECT_SHORT " - " NAME_CPFMANAGER " | WARNING"
#define INFO_CPFMANAGER NAME_PROJECT_SHORT " - " NAME_CPFMANAGER " | INFO"
#define NAME_CPFMANAGERCONFIGFILE "SP_CPFManagerConfig.ini"
#define TEXT_NO_OUTDATED_FILES_FOUND "No outdated files were found at Current CPF directory"
#define TEXT_OUTDATED_FILES_REMOVED "Outdated files were removed from Current CPF directory. " \
"Click Show Details to see the deleted files."
#define TEXT_CANNOT_CREATE_TEMP "Cannot create temporary folder for downloading CPF"
#define TEXT_CONNECTING(NAME) "Connecting to " + NAME + "..."
#define TEXT_DOWNLOADING(NAME) "Downloading files from " + NAME + "..."
#define TEXT_DOWNLOADING_TLES(NAME) "Downloading files from " + NAME + "..."
#define TEXT_GENERATING_CPF "Generating CPFs from downloaded TLEs..."
#define TEXT_WAITING "Waiting"
#define TEXT_ALREADY_EXISTS "Already exists"
#define TEXT_SELECT_EXT_KEEP "Select extension to keep: "
#define TEXT_SELECT_EXT_DELETE "Select extension to delete: "
#define TEXT_HISTORICAL_NOT_FOUND "CPF folder not found for selected date"
#define TEXT_HISTORICAL_EMPTY "There is no CPF file for selected date"
#define TEXT_INVALID_TLE_DOWNLOADED(NORAD) "Invalid TLE downloaded for object with NORAD: " + NORAD
#define TEXT_CANNOT_CREATE_TLE_FILE "Impossible to create TLE file"
#define TEXT_CANNOT_STORE_GENERATED_CPF "Impossible to store CPF generated from TLE."
#define TEXT_CANCELED_BY_REQUEST "Download process canceled by user request"
#define TEXT_INVALID_CPF_GENERATED "Generated CPF is invalid"
#define TEXT_ABOUT_TO_DELETE "File(s) will be deleted permanently. Are you sure?"
#define TEXT_NO_CPF_FOUND(SITE) "No CPF was found at " + SITE
#define TEXT_OPEN_FILE_ACTION "Open File(s)"
#define TEXT_COPY_FILE_ACTION "Copy File(s)"
#define TEXT_DELETE_FILE_ACTION "Delete File(s)"
#define TEXT_ABOUT_TO_RENAME(FORMAT) "CPF files of current and historical database shall be renamed under the current filename format option: " + FORMAT + ". Are you sure?"
#define TEXT_WAIT_RENAME "Please wait until all CPF files are renamed."
#define TEXT_CANNOT_RENAME(FILE) "Cannot rename file: " + FILE

// PREDICTIONS GENERATOR
#define NAME_PREDICTIONSGENERATOR "PREDICTIONS GENERATOR"
#define ERROR_PREDICTIONSGENERATOR NAME_PROJECT_SHORT " - " NAME_PREDICTIONSGENERATOR " | ERROR"
#define WARNING_PREDICTIONSGENERATOR NAME_PROJECT_SHORT " - " NAME_PREDICTIONSGENERATOR " | WARNING"
#define INFO_PREDICTIONSGENERATOR NAME_PROJECT_SHORT " - " NAME_PREDICTIONSGENERATOR " | INFO"
#define NAME_PREDICTIONSGENERATORCONFIGFILE "SP_PredictionsGeneratorConfig.ini"
#define TEXT_PREDICTION_PROCESS_CANCELLED "Predictions calculation stopped due to user request."
#define TEXT_NO_PASSES_FOUND "No passes found."
#define TEXT_GENERATING_PREDICTIONS "Generating predictions..."
#define TEXT_GENERATING_PREDICTIONS_DIALOG "Generating predictions. Please wait a few seconds..."
#define TEXT_SO_WITHOUT_CPF "Space objects without valid CPFs have been detected. Those will be ignored for the predictions calculation."
#define TEXT_PREDICTIONS_IGNORED "Some predictions have been ignored since the associated objects are disabled. Click 'Show Details...' button to see the NORADS."
#define TEXT_UNSAVED_PREDICTION "There are unsaved predictions. Quit without saving anyway?"
#define TEXT_DELETE_PREDICTION_ACTION "Delete Prediction(s)"
#define TEXT_PREDICTIONS_SAVED "New predictions saved. For more details, click 'Show Details...' button."
#define TEXT_SEND_NOTIF_QUESTION "Automatic notifications are disabled. Do you want to sent the new predictions notification?."


// STATION CONTROL
#define NAME_STATIONCONTROL "STATION CONTROL"
#define ERROR_STATIONCONTROL NAME_PROJECT_SHORT " - " NAME_STATIONCONTROL " | ERROR"
#define WARNING_STATIONCONTROL NAME_PROJECT_SHORT " - " NAME_STATIONCONTROL " | WARNING"
#define INFO_STATIONCONTROL NAME_PROJECT_SHORT " - " NAME_STATIONCONTROL " | INFO"
#define NAME_STATIONCONTROLCONFIGFILE "SP_StationControlConfig.ini"
#define WARNING_LASERCONTROL "WARNING - EKSPLA NL317-SH Laser Control"
#define WARNING_LASERCONTROL_TEXT \
    "<b>Warning: some components of the optical table are not connected.</b>\n" \
    "Check if laser rack and station rack are turned off. If the problem persits, contact with technical staff. "
#define INFO_LASERCONTROL "INFO - EKSPLA NL317-SH Laser Control"
#define LASER_HEATING_DIALOG "Laser warmup in progress. This process shall take %1 minutes"
#define LASER_WARMUP_FINISHED "Laser warmup process finished. Laser system shall switch to Wait Mode."
#define LASER_ALREADY_WARMEDUP "Laser is already warmed up. Laser system shall switch to Wait Mode."
#define LOCATING_COMPONENTS "Setting up laser system components. Please, wait..."
#define CONNECTING_COMPONENTS "Connecting laser system components and sending them to home position. Please, wait..."
#define DISCONNECTING_COMPONENTS "Disconnecting laser system components. Please, wait..."

#define MANUAL_SETUP "Warning: You will start laser system manual control. Proceed with caution."
#define LASER_ERROR "<b>WARNING: Laser system is in ERROR state.</b>. Current laser system will be disconnected. " \
    "Check cooling system temperature and " \
    "security bars up. Afterwards, reboot laser system. If the problem persists contact the technical " \
    "staff. Please remember to wear security glasses."
#define REMOCONTROL_MISSING_ERROR "<b>WARNING: File REMOTECONTROL.CSV not found.</b> Check that the file is " \
    "located in the same directory as the executable file."
#define WARNING_NOT_SELECTED "<b>WARNING: There is no object selected to be observated.</b> Select the object you " \
    "want to observe at the planning table. Laser will switch to stop mode."
#define TOOLTIP_CONNECT_CB "Impossible to change laser system while it is connected. Disconnect first."
#define TEXT_EXIT_CONFIRM "Are you sure to exit Station Control?"
#define TEXT_WARMUP_CONFIG "Laser is not warmed up. Force warm up state?"
#define TEXT_NO_LASER_SYSTEM "No laser system was found."
#define TEXT_NO_RGG_CONTROLLER "No RGG controller client was found."
#define TEXT_RGG_CONTROLLER_NOT_CONFIG "RGG controller client was not configured."
#define TEXT_RGG_CONTROLLER_NOT_FOUND "No RGG controller client was found with name %1."
#define TEXT_WAIT_STATION_CONTROLLER "Waiting for Station Controller Server acknowledge"
#define TEXT_PASS_FINISHED "Pase actual finalizado. Volviendo a modo Espera."
#define TEXT_NEXT_PASS "Siguiente paso:"
#define TEXT_NEXT_PASSES "Siguientes pasos:"
#define TEXT_INTRO "Bienvenido al sistema DÉGORAS para el control de estaciones de telemetría láser."

// TRACKING SYSTEM
#define NAME_TRACKINGSYSTEM "TRACKING SYSTEM"
#define ERROR_TRACKINGSYSTEM NAME_PROJECT_SHORT " - " NAME_TRACKINGSYSTEM " | ERROR"
#define WARNING_TRACKINGSYSTEM NAME_PROJECT_SHORT " - " NAME_TRACKINGSYSTEM " | WARNING"
#define INFO_TRACKINGSYSTEM NAME_PROJECT_SHORT " - " NAME_TRACKINGSYSTEM " | INFO"
#define NAME_TRACKINGSYSTEMCONFIGFILE "DP_TrackingSystemConfig.ini"
#define STATE_TRACKING "TRACKING"
#define STATE_WAIT "WAIT"
#define STATE_CALIB "CALIBRATION"
#define STATE_CALIB_RECORD "CALIBRATION \n RECORDING"
#define STATE_SIMULATION_TRACKING "TRACKING \n SIMULATION"
#define STATE_SIMULATION_CALIBRATION "CALIBRATION \n SIMULATION"
#define STATE_WAIT_FOR_TRACKING "WAITING FOR \n TRACKING START"
#define MOUNT_CONNECT_FAIL "Mount system connection failed."
#define MOUNT_CONNECT_OK "Mount system connection successful."
#define MOUNT_CONNECTION_LOST "Mount system connection broken."
#define MOUNT_STOP "Mount stop command issued."
#define MOUNT_GO_CALIBRATION "Mount go to calibration command issued."
#define MOUNT_GO_HOME "Mount go home command issued."
#define MOUNT_MANUAL_START "Mount system enters Manual Adjust mode."
#define MOUNT_MANUAL_STOP "Mount system exits Manual Adjust mode."
#define MOUNT_TRACKING_START "Mount system enters Tracking mode."
#define MOUNT_TRACKING_STOP "Mount system exits Tracking mode."
#define MOUNT_COMMAND_FAILED "Mount system command %1 failed."

// RGG Manager
#define NAME_RGGMANAGER "RANGE GATE GENERATOR MANAGER"
#define ERROR_RGGMANAGER NAME_PROJECT_SHORT " - " NAME_RGGMANAGER " | ERROR"
#define WARNING_RGGMANAGER NAME_PROJECT_SHORT " - " NAME_RGGMANAGER " | WARNING"
#define INFO_RGGMANAGER NAME_PROJECT_SHORT " - " NAME_RGGMANAGER " | INFO"
#define NAME_RGGMANAGERCONFIGFILE "DP_RGGManagerConfig.ini"

// Results Manager
#define NAME_RESULTSMANAGER "RESULTS MANAGER"
#define ERROR_RESULTSMANAGER NAME_PROJECT_SHORT " - " NAME_RESULTSMANAGER " | ERROR"
#define WARNING_RESULTSMANAGER NAME_PROJECT_SHORT " - " NAME_RESULTSMANAGER " | WARNING"
#define INFO_RESULTSMANAGER NAME_PROJECT_SHORT " - " NAME_RESULTSMANAGER " | INFO"
#define NAME_RESULTSMANAGERCONFIGFILE "DP_ResultsManagerConfig.ini"

// POINTING MODEL GENERATOR.
#define NAME_POINTINGMODELGENERATOR "POINTING MODEL GENERATOR"
#define ERROR_POINTINGMODELGENERATOR NAME_PROJECT_SHORT " - " NAME_SPACEOBJECTMANAGER " | ERROR"
#define WARNING_POINTINGMODELGENERATOR NAME_PROJECT_SHORT " - " NAME_SPACEOBJECTMANAGER " | WARNING"
#define INFO_POINTINGMODELGENERATOR NAME_PROJECT_SHORT " - " NAME_SPACEOBJECTMANAGER " | INFO"
#define NAME_POINTINGMODELGENERATORCONFIGFILE "DP_PointingModelGeneratorConfig.ini"

#define ID_MISSING_MESSAGE "Some IDs were not found at set loading (check them by clicking 'Show Details...')."


// Generic strings
#define MSG(TYPE, APP) TYPE##_MSG(APP)
#define INFO_MSG(NAME) INFO_##NAME
#define WARNING_MSG(NAME) WARNING_##NAME
#define ERROR_MSG(NAME) ERROR_##NAME

//=========== TIME STRINGS =============================================================================================
#define TIME_STORESTRING "hhmmss"
#define DATE_STORESTRING "yyyyMMdd"
#define CPF_DATESTRING "yyMMdd"
#define DATETIME_STORESTRING "yyyyMMdd.hhmmss"
#define DATETIME_LABELSTRING "dd-MM-yyyy hh:mm:ss UTC"
    //======================================================================================================================
