#pragma once

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

class SpaceObjectSet
{
public:
    SpaceObjectSet(const QString& name, const QStringList& enabled);
    inline const QString& getName() const {return this->name;}
    inline void setName(const QString& name) {this->name = name;}
    inline const QStringList& getEnabled() const {return this->list_enabled;}
    inline void setEnabled(const QStringList& enabled) {this->list_enabled = enabled;}
private:
    QString name;
    QStringList list_enabled;
};

class SpaceObject
{
public:
    static constexpr const char* kAllCPFProvider = "All";
    static constexpr const char* kNoTLEProvider = "No TLE";

    enum TrackPolicy { NO_TRACK_POLICY, TRACK_ALWAYS, TRACK_ONLY_IF_VISIBLE, TRACK_ONLY_AT_NIGHT, CUSTOM_TRACK_POLICY_1, CUSTOM_TRACK_POLICY_2 };
    enum EnablementPolicy { NO_ENABLEMENT_POLICY, DISABLED, ENABLED, ALWAYS_DISABLED, ALWAYS_ENABLED };

    static const QMap<TrackPolicy, QString> TrackPolicyStringMap;
    static const QMap<EnablementPolicy, QString> EnablementPolicyStringMap;
    static const QMap<QString, SpaceObject::EnablementPolicy> EnablementPolicyEnumMap;

    // --- CONSTRUCTOR MODIFICADO (Respetando estructura) ---
    // Cambio 1: 'abbreviation' ahora es 'alias'
    // Cambio 2: Añadido 'comments' después de 'picture'
    SpaceObject(QString name, QString ILRSname, QString alias, QString cospar, QString ILRScospar,
                QString norad, QString sic, QString classification, QString laserid, QString detectorid,
                QString counterid, QString cpfprovider, QString picture, QString comments,
                TrackPolicy trackpolicy, EnablementPolicy enablementpolicy,
                int npi, int bs, int altitude, int amplification, int priority,
                double inclination, double rcs, bool llr, bool debris,
                QMap<QString, QString>& extraparameters);

    SpaceObject(); // Constructor vacío

    SpaceObject(const QJsonValue &json_value, const QStringList &extrakeys);

    // --- GETTERS (Observadores) ---
    inline const QString& getName()           const {return this->name;}
    inline const QString& getILRSname()       const {return this->ILRSname;}
    inline const QString& getPreferredName()  const {return this->ILRSname.isEmpty() ? this->name :  this->ILRSname;}

    // NUEVO: Alias
    inline const QString& getAlias()          const {return this->alias;}

    inline const QString& getCospar()         const {return this->cospar;}
    inline const QString& getILRSID()         const {return this->ILRSID;}
    inline const QString& getNorad()          const {return this->norad;}
    inline const QString& getSIC()            const {return this->sic;}
    inline const QString& getClassification() const {return this->classification;}
    inline const QString& getLaserID()        const {return this->laserid;}
    inline const QString& getDetectorID()     const {return this->detectorid;}
    inline const QString& getCounterID()      const {return this->counterid;}
    inline const QString& getCPFProvider()    const {return this->cpfprovider;}
    inline const QString& getPicture()        const {return this->picture;}

    // NUEVO: Comments
    inline const QString& getComments()       const {return this->comments;}

    inline TrackPolicy getTrackPolicy()       const {return this->trackpolicy;}
    inline EnablementPolicy getEnablementPolicy() const {return this->enablementpolicy;}
    inline const int& getNormalPointIndicator()   const {return this->npi;}
    inline const int& getBinSize()                const {return this->bs;}
    inline const int& getAltitude()               const {return this->altitude;}
    inline const int& getAmplification()          const {return this->amplification;}
    inline const int& getPriority()               const {return this->priority;}
    inline const double& getInclination()         const {return this->inclination;}
    inline const double& getRadarCrossSection()   const {return this->rcs;}
    inline const bool& hasLRR()  const {return this->lrr;}
    inline const bool& isDebris()                 const {return this->debris;}
    inline const QMap<QString, QString>& getExtraParameters() const {return this->extraparameters;}
    inline const bool& isLaserRetroReflectorChecked()         const {return this->llr_checked;}
    inline const bool& isDebrisChecked()           const {return this->debris_checked;}
    inline const bool& isInitialized()             const {return this->initialized;}

    // --- SETTERS (Modificadores) ---
    inline void setName(QString name)                 {this->name = name;}
    inline void setILRSname(QString ILRSname)         {this->ILRSname = ILRSname;}

    // NUEVO: Setter Alias
    inline void setAlias(QString alias)               {this->alias = alias;}

    inline void setCospar(QString cospar)             {this->cospar = cospar;}
    inline void setILRScospar(QString ILRScospar)     {this->ILRSID = ILRScospar;}
    inline void setNorad(QString norad)               {this->norad = norad;}
    inline void setSic(QString sic)                   {this->sic = sic;}
    inline void setClassification(QString classifi)   {this->classification = classifi;}
    inline void setLaserID(QString laserid)           {this->laserid = laserid;}
    inline void setDetectorID(QString detectorid)     {this->detectorid = detectorid;}
    inline void setCounterID(QString counterid)       {this->counterid = counterid;}
    inline void setCPFProvider(QString cpf)           {this->cpfprovider = cpf;}
    inline void setPicture(QString picture)           {this->picture = picture;}

    // NUEVO: Setter Comments
    inline void setComments(QString comments)         {this->comments = comments;}

    inline void setTrackPolicy(TrackPolicy policy)          {this->trackpolicy = policy;}
    inline void setEnablementPolicy(EnablementPolicy policy){this->enablementpolicy = policy;}
    inline void setNormalPointIndicator(int npi)      {this->npi = npi;}
    inline void setBinSize(int bs)                    {this->bs = bs;}
    inline void setAltitude(int altitude)             {this->altitude = altitude;}
    inline void setAmplification(int amplification)   {this->amplification = amplification;}
    inline void setPriority(int priority)             {this->priority = priority;}
    inline void setInclination(double inclination)    {this->inclination = inclination;}
    inline void setRadarCrossSection(double rcs)      {this->rcs = rcs;}
    inline void setHasLRR(bool llr) {this->lrr = llr; this->llr_checked = true;}
    inline void setIsDebris(bool debris)              {this->debris = debris; this->debris_checked = true;}
    inline void setExtraParameters(const QMap<QString, QString>& extra){this->extraparameters = extra;}
    inline void setInitialized(bool ini)  {this->initialized = ini;}

    const QString toString() const;
    const QJsonObject toJson() const;
    static QJsonObject generateEmptyJsonobject();

protected:
    // --- IMPORTANTE: EL ORDEN AQUÍ DEBE SER IDÉNTICO AL DEL CONSTRUCTOR .CPP ---
    QString name;
    QString ILRSname;
    QString alias; // Renombrado de abbreviation
    QString cospar;
    QString ILRSID;
    QString norad;
    QString sic;
    QString classification;
    QString laserid;
    QString detectorid;
    QString counterid;
    QString cpfprovider;
    QString picture;
    QString comments; // AÑADIDO AQUÍ (Justo después de los Strings)

    TrackPolicy trackpolicy;
    EnablementPolicy enablementpolicy;
    int npi;
    int bs;
    int altitude;
    int amplification;
    int priority;
    double inclination;
    double rcs;
    bool lrr;
    bool debris;
    QMap<QString, QString> extraparameters;
    bool llr_checked;
    bool debris_checked;
    bool initialized;
};

Q_DECLARE_METATYPE(SpaceObject*)
Q_DECLARE_METATYPE(SpaceObjectSet*)
