#pragma once

#include "guiserialization.hxx"

class MagicLanguage
{
public:
    QString languageFilePath;
    QString languageFileName;

    struct LanguageInfo {
        // basic parameters
        unsigned int version;
        QString name;
        QString nameInOriginal;
        QString abbr;
        QString authors;
    } info;

    QHash<QString, QString> strings;

    bool lastSearchSuccesfull;

    static QString getStringFormattedWithVars(QString&& string);

    static QString keyNotDefined(QString key);

    static bool getLanguageInfo(QString filepath, LanguageInfo &info);

    bool loadText();

    void clearText();

    QString getText(QString key, bool *found = NULL);
};

class MagicLanguages
{
public:
    unsigned int getNumberOfLanguages();

    QString getByKey(QString key, bool *found = NULL);

    void scanForLanguages(QString languagesFolder);

    void updateLanguageContext();

    bool selectLanguageByIndex(unsigned int index);

    bool selectLanguageByLanguageName(QString langName);

    bool selectLanguageByLanguageAbbr(QString abbr);

    bool selectLanguageByFileName(QString filename);

    inline MagicLanguages( void )
    {
        this->isInitialized = false;

        this->mainWnd = NULL;

        this->currentLanguage = -1;
    }

    inline void Initialize( MainWindow *mainWnd )
    {
        this->mainWnd = mainWnd;

        // read languages
        scanForLanguages(mainWnd->makeAppPath("languages"));

        bool hasAcquiredLanguage = false;

        // First we try detecting a language by the system locale.
        if ( !hasAcquiredLanguage )
        {
            QLocale::Language lang = QLocale::system().language();

            QString langString = QLocale::languageToString( lang );

            hasAcquiredLanguage = selectLanguageByLanguageName( std::move( langString ) );
        }

        if ( !hasAcquiredLanguage )
        {
            // Select the default language then.
            if ( selectLanguageByLanguageName(DEFAULT_LANGUAGE) )
            {
                hasAcquiredLanguage = true;
            }
        }

        // If anything else failed, we just try selecting the first language that is registered.
        if ( !hasAcquiredLanguage )
        {
            if ( selectLanguageByIndex( 0 ) )
            {
                hasAcquiredLanguage = true;
            }
        }

        if ( !hasAcquiredLanguage )
        {
            // If we have not acquired a language, we must initialize using the placeholders.
            updateLanguageContext();
        }

        // Allow initialization of language items during registration.
        this->isInitialized = true;
    }

    inline void Shutdown( MainWindow *mainWnd )
    {
        this->isInitialized = false;

        this->mainWnd = NULL;
    }

    void Load( MainWindow *mainWnd, rw::BlockProvider& configBlock )
    {
        std::wstring langfile_str;

        RwReadUnicodeString( configBlock, langfile_str );

        mainWnd->lastLanguageFileName = QString::fromStdWString( langfile_str );

        // Load the language.
        bool loadedLanguage = selectLanguageByFileName( mainWnd->lastLanguageFileName );

        // Since loading the configuration is optional, we do not care if we failed to load a language here.
    }

    void Save( const MainWindow *mainWnd, rw::BlockProvider& configBlock ) const
    {
        RwWriteUnicodeString( configBlock, mainWnd->lastLanguageFileName.toStdWString() );
    }

    bool isInitialized;

    MainWindow *mainWnd;

    QVector<MagicLanguage> languages;

    int currentLanguage; // index of current language in languages array, -1 if not defined

    localizations_t culturalItems;
};

extern MagicLanguages ourLanguages;