#include "mainwindow.h"
#include "optionsdialog.h"
#include "qtutils.h"

#include "languages.hxx"

OptionsDialog::OptionsDialog( MainWindow *mainWnd ) : QDialog( mainWnd )
{
    this->mainWnd = mainWnd;

    setWindowFlags( this->windowFlags() & ~Qt::WindowContextHelpButtonHint );

    setAttribute( Qt::WA_DeleteOnClose );

    // This will be a fairly complicated dialog.
    MagicLayout<QVBoxLayout> layout(this);

    QTabWidget *optTabs = new QTabWidget( this );

    // Create a tab with main editor configurations.
    QWidget *mainTab = new QWidget( this );

    QVBoxLayout *mainTabLayout = new QVBoxLayout( this );

    this->optionShowLogOnWarning = CreateCheckBoxL( "Main.Options.ShowLog" );
    this->optionShowLogOnWarning->setChecked( mainWnd->showLogOnWarning );
    mainTabLayout->addWidget(optionShowLogOnWarning);
    this->optionShowGameIcon = CreateCheckBoxL( "Main.Options.DispIcn" );
    this->optionShowGameIcon->setChecked(mainWnd->showGameIcon);
    mainTabLayout->addWidget( optionShowGameIcon );

    // Display language select.
    this->languageBox = new QComboBox();

    this->languageBox->setFixedWidth(300);
    {
        size_t languageCount = ourLanguages.languages.size();

        for (unsigned int i = 0; i < languageCount; i++)
        {
            const MagicLanguage& lang = ourLanguages.languages[ i ];

            this->languageBox->addItem(lang.info.nameInOriginal + " - " + lang.info.name);
        }
    }

    connect(this->languageBox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &OptionsDialog::OnChangeSelectedLanguage);

    QFormLayout *languageFormLayout = new QFormLayout();
    languageFormLayout->addRow(CreateLabelL( "Lang.Lang" ), languageBox);

    mainTabLayout->addLayout(languageFormLayout);

    this->languageAuthorLabel = new QLabel();

    mainTabLayout->addWidget(this->languageAuthorLabel);

    this->languageBox->setCurrentIndex(ourLanguages.currentLanguage);

    mainTabLayout->setAlignment(this->languageAuthorLabel, Qt::AlignRight);

    mainTab->setLayout( mainTabLayout );

    optTabs->addTab( mainTab, "Main" );

    layout.top->addWidget( optTabs );

    QPushButton *buttonAccept = CreateButtonL( "Main.Options.Accept" );
    layout.bottom->addWidget(buttonAccept);
    QPushButton *buttonCancel = CreateButtonL( "Main.Options.Cancel" );
    layout.bottom->addWidget(buttonCancel);

    connect( buttonAccept, &QPushButton::clicked, this, &OptionsDialog::OnRequestApply );
    connect( buttonCancel, &QPushButton::clicked, this, &OptionsDialog::OnRequestCancel );

    RegisterTextLocalizationItem( this );

    mainWnd->optionsDlg = this;
}

OptionsDialog::~OptionsDialog( void )
{
    mainWnd->optionsDlg = NULL;

    UnregisterTextLocalizationItem( this );
}

void OptionsDialog::updateContent( MainWindow *mainWnd )
{
    setWindowTitle( MAGIC_TEXT("Main.Options.Desc") );
}

void OptionsDialog::OnRequestApply( bool checked )
{
    this->serialize();

    this->close();
}

void OptionsDialog::OnRequestCancel( bool checked )
{
    this->close();
}

void OptionsDialog::serialize( void )
{
    // We want to store things into the main window.
    MainWindow *mainWnd = this->mainWnd;

    mainWnd->showLogOnWarning = this->optionShowLogOnWarning->isChecked();

    if (mainWnd->showGameIcon != this->optionShowGameIcon->isChecked()) {
        mainWnd->showGameIcon = this->optionShowGameIcon->isChecked();
        mainWnd->updateFriendlyIcons();
    }

    if (ourLanguages.currentLanguage != -1) {
        MagicLanguage& magLang = ourLanguages.languages[ ourLanguages.currentLanguage ];

        if (mainWnd->lastLanguageFileName != magLang.languageFileName) {
            mainWnd->lastLanguageFileName = magLang.languageFileName;
        }
    }
}

void OptionsDialog::OnChangeSelectedLanguage(int newIndex)
{
    this->languageAuthorLabel->setText("");

    ourLanguages.selectLanguageByIndex( newIndex );

    if (newIndex >= 0 && ourLanguages.languages[newIndex].info.authors != "Magic.TXD Team") {
        QString names;
        bool found = false;
        QString namesFormat = MAGIC_TEXT_CHECK_AVAILABLE("Lang.Authors", &found);

        if (found)
            names = QString(namesFormat).arg(ourLanguages.languages[newIndex].info.authors);
        else
            names = QString("by " + ourLanguages.languages[newIndex].info.authors);
        this->languageAuthorLabel->setText(names);
    }
}