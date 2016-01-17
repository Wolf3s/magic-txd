#include "mainwindow.h"
#include "qtutils.h"
#include <regex>
#include "testmessage.h"

static const QRegExp forbPathChars("[/:?\"<>|\\[\\]\\\\]");

CreateTxdDialog::CreateTxdDialog(MainWindow *MainWnd) : QDialog(MainWnd) {
    this->mainWnd = MainWnd;

    this->setWindowTitle(MAGIC_TEXT("New.Desc"));

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowModality(Qt::WindowModality::WindowModal);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create our GUI interface
    MagicLayout<QVBoxLayout> layout(this);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    QLabel *nameLabel = new QLabel(MAGIC_TEXT("New.Name"));
    nameLabel->setObjectName("label25px");
    QLineEdit *nameEdit = new QLineEdit();

    connect(nameEdit, &QLineEdit::textChanged, this, &CreateTxdDialog::OnUpdateTxdName);

    nameEdit->setFixedWidth(300);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    this->txdName = nameEdit;

    /************* Set ****************/
    QHBoxLayout *selectGameLayout = new QHBoxLayout;
    QLabel *gameLabel = new QLabel(MAGIC_TEXT("Main.SetupTV.Set"));
    gameLabel->setObjectName("label25px");
    QComboBox *gameComboBox = new QComboBox;
    gameComboBox->setFixedWidth(300);
    gameComboBox->addItem(MAGIC_TEXT("Main.SetupTV.Custom"));   /// HAXXXXXXX
    for (unsigned int i = 0; i < this->mainWnd->versionSets.sets.size(); i++)
        gameComboBox->addItem(this->mainWnd->versionSets.sets[i].name);
    this->gameSelectBox = gameComboBox;

    connect(gameComboBox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &CreateTxdDialog::OnChangeSelectedGame);

    selectGameLayout->addWidget(gameLabel);
    selectGameLayout->addWidget(gameComboBox);

    /************* Platform ****************/
    QHBoxLayout *selectPlatformLayout = new QHBoxLayout;
    QLabel *platLabel = new QLabel(MAGIC_TEXT("Main.SetupTV.Plat"));
    platLabel->setObjectName("label25px");
    QComboBox *platComboBox = new QComboBox;
    platComboBox->setFixedWidth(300);
    this->platSelectBox = platComboBox;

    connect(platComboBox, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &CreateTxdDialog::OnChangeSelecteedPlatform);

    selectPlatformLayout->addWidget(platLabel);
    selectPlatformLayout->addWidget(platComboBox);

    /************* Data type ****************/
    QHBoxLayout *selectDataTypeLayout = new QHBoxLayout;
    QLabel *dataTypeLabel = new QLabel(MAGIC_TEXT("Main.SetupTV.Data"));
    dataTypeLabel->setObjectName("label25px");
    QComboBox *dataTypeComboBox = new QComboBox;
    dataTypeComboBox->setFixedWidth(300);
    this->dataTypeSelectBox = dataTypeComboBox;

    selectDataTypeLayout->addWidget(dataTypeLabel);
    selectDataTypeLayout->addWidget(dataTypeComboBox);

    QHBoxLayout *versionLayout = new QHBoxLayout;
    QLabel *versionLabel = new QLabel(MAGIC_TEXT("Main.SetupTV.Version"));
    versionLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    versionLabel->setObjectName("label25px");

    QHBoxLayout *versionNumbersLayout = new QHBoxLayout;
    QLineEdit *versionLine1 = new QLineEdit;

    versionLine1->setInputMask("0.00.00.00");
    versionLine1->setFixedWidth(80);

    versionNumbersLayout->addWidget(versionLine1);

    versionNumbersLayout->setMargin(0);

    this->versionLineEdit = versionLine1;

    connect(versionLine1, &QLineEdit::textChanged, this, &CreateTxdDialog::OnChangeVersion);

    QLabel *buildLabel = new QLabel(MAGIC_TEXT("Main.SetupTV.Build"));
    buildLabel->setObjectName("label25px");
    buildLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QLineEdit *buildLine = new QLineEdit;
    buildLine->setInputMask(">HHHH");
    buildLine->clear();
    buildLine->setFixedWidth(60);

    this->buildLineEdit = buildLine;

    versionLayout->addWidget(versionLabel);
    versionLayout->addLayout(versionNumbersLayout);
    versionLayout->addWidget(buildLabel);
    versionLayout->addWidget(buildLine);

    versionLayout->setAlignment(Qt::AlignRight);

    layout.top->addLayout(nameLayout);
    layout.top->addSpacing(8);
    layout.top->addLayout(selectGameLayout);
    layout.top->addLayout(selectPlatformLayout);
    layout.top->addLayout(selectDataTypeLayout);
    layout.top->addSpacing(8);
    layout.top->addLayout(versionLayout);

    QPushButton *buttonAccept = CreateButton(MAGIC_TEXT("New.Create"));
    QPushButton *buttonCancel = CreateButton(MAGIC_TEXT("New.Cancel"));

    this->applyButton = buttonAccept;

    connect(buttonAccept, &QPushButton::clicked, this, &CreateTxdDialog::OnRequestAccept);
    connect(buttonCancel, &QPushButton::clicked, this, &CreateTxdDialog::OnRequestCancel);

    layout.bottom->addWidget(buttonAccept);
    layout.bottom->addWidget(buttonCancel);

    // Initiate the ready dialog.
    this->OnChangeSelectedGame(0);
    
    this->UpdateAccessibility();
}

bool CreateTxdDialog::GetSelectedVersion(rw::LibraryVersion& verOut)
{
    QString currentVersionString = this->versionLineEdit->text();

    std::string ansiCurrentVersionString = qt_to_ansi(currentVersionString);

    rw::LibraryVersion theVersion;

    bool hasValidVersion = false;

    // Verify whether our version is valid while creating our local version struct.
    unsigned int rwLibMajor, rwLibMinor, rwRevMajor, rwRevMinor;
    bool hasProperMatch = false;
    {
        std::regex ver_regex("(\\d)\\.(\\d{1,2})\\.(\\d{1,2})\\.(\\d{1,2})");

        std::smatch ver_match;

        std::regex_match(ansiCurrentVersionString, ver_match, ver_regex);

        if (ver_match.size() == 5)
        {
            rwLibMajor = std::stoul(ver_match[1]);
            rwLibMinor = std::stoul(ver_match[2]);
            rwRevMajor = std::stoul(ver_match[3]);
            rwRevMinor = std::stoul(ver_match[4]);

            hasProperMatch = true;
        }
    }

    if (hasProperMatch)
    {
        if ((rwLibMajor >= 3 && rwLibMajor <= 6) &&
            (rwLibMinor <= 15) &&
            (rwRevMajor <= 15) &&
            (rwRevMinor <= 63))
        {
            theVersion.rwLibMajor = rwLibMajor;
            theVersion.rwLibMinor = rwLibMinor;
            theVersion.rwRevMajor = rwRevMajor;
            theVersion.rwRevMinor = rwRevMinor;

            hasValidVersion = true;
        }
    }

    if (hasValidVersion)
    {
        // Also set the build number, if valid.
        QString buildNumber = this->buildLineEdit->text();

        std::string ansiBuildNumber = qt_to_ansi(buildNumber);

        unsigned int buildNum;

        int matchCount = sscanf(ansiBuildNumber.c_str(), "%x", &buildNum);

        if (matchCount == 1)
        {
            if (buildNum <= 65535)
            {
                theVersion.buildNumber = buildNum;
            }
        }

        // Having an invalid build number does not mean that our version is invalid.
        // The build number is just candy anyway.
    }

    if (hasValidVersion)
    {
        verOut = theVersion;
    }

    return hasValidVersion;
}

void CreateTxdDialog::UpdateAccessibility(void)
{
    rw::LibraryVersion libVer;

    // Check whether we should even enable input.
    // This is only if the user selected "Custom".
    bool shouldAllowInput = (this->gameSelectBox->currentIndex() == 0);

    this->versionLineEdit->setDisabled(!shouldAllowInput);
    this->buildLineEdit->setDisabled(!shouldAllowInput);

    bool hasValidVersion = this->GetSelectedVersion(libVer);

    // Alright, set enabled-ness based on valid version.
    if(!hasValidVersion || this->txdName->text().isEmpty() || this->txdName->text().contains(forbPathChars))
        this->applyButton->setDisabled(true);
    else
        this->applyButton->setDisabled(false);
}

void CreateTxdDialog::OnChangeVersion(const QString& newText)
{
    // The version must be validated.
    this->UpdateAccessibility();
}

void CreateTxdDialog::OnChangeSelectedGame(int newIndex)
{
    if (newIndex >= 0) {
        if (newIndex == 0) { // Custom
            this->platSelectBox->setCurrentIndex(-1);
            this->platSelectBox->setDisabled(true);
            QString lastDataTypeName = this->dataTypeSelectBox->currentText();
            this->dataTypeSelectBox->clear();
            for (int i = 1; i <= RwVersionSets::RWVS_DT_NUM_OF_TYPES; i++) {
                const char *dataName = RwVersionSets::dataNameFromId((RwVersionSets::eDataType)i);
                this->dataTypeSelectBox->addItem(dataName);
                if (lastDataTypeName == dataName)
                    this->dataTypeSelectBox->setCurrentIndex(i - 1);
            }
            this->dataTypeSelectBox->setDisabled(false);
        }
        else {
            this->platSelectBox->clear();
            for (unsigned int i = 0; i < this->mainWnd->versionSets.sets[newIndex - 1].availablePlatforms.size(); i++) {
                this->platSelectBox->addItem(RwVersionSets::platformNameFromId(
                    this->mainWnd->versionSets.sets[newIndex - 1].availablePlatforms[i].platformType));
            }
            if (this->mainWnd->versionSets.sets[newIndex - 1].availablePlatforms.size() < 2)
                this->platSelectBox->setDisabled(true);
            else
                this->platSelectBox->setDisabled(false);
            //this->OnChangeSelecteedPlatform( 0 );
        }
    }

    // We want to update the accessibility.
    this->UpdateAccessibility();
}

void CreateTxdDialog::OnChangeSelecteedPlatform(int newIndex) {
    if (newIndex >= 0) {
        this->dataTypeSelectBox->clear();
        unsigned int set = this->gameSelectBox->currentIndex();

        const RwVersionSets::Set& versionSet = this->mainWnd->versionSets.sets[set - 1];
        const RwVersionSets::Set::Platform& platformOfSet = versionSet.availablePlatforms[newIndex];

        for (unsigned int i = 0; i < platformOfSet.availableDataTypes.size(); i++) {
            this->dataTypeSelectBox->addItem(
                RwVersionSets::dataNameFromId(platformOfSet.availableDataTypes[i])
                );
        }
        if (platformOfSet.availableDataTypes.size() < 2)
            this->dataTypeSelectBox->setDisabled(true);
        else
            this->dataTypeSelectBox->setDisabled(false);

        std::string verString =
            std::to_string(platformOfSet.version.rwLibMajor) + "." +
            std::to_string(platformOfSet.version.rwLibMinor) + "." +
            std::to_string(platformOfSet.version.rwRevMajor) + "." +
            std::to_string(platformOfSet.version.rwRevMinor);

        std::string buildString;

        if (platformOfSet.version.buildNumber != 0xFFFF)
        {
            std::stringstream hex_stream;

            hex_stream << std::hex << platformOfSet.version.buildNumber;

            buildString = hex_stream.str();
        }

        this->versionLineEdit->setText(ansi_to_qt(verString));
        this->buildLineEdit->setText(ansi_to_qt(buildString));
    }
}

void CreateTxdDialog::OnRequestAccept(bool clicked)
{
    // Just create an empty TXD.
    rw::TexDictionary *newTXD = NULL;

    try
    {
        newTXD = rw::CreateTexDictionary(this->mainWnd->rwEngine);
    }
    catch (rw::RwException& except)
    {
        this->mainWnd->txdLog->showError(QString("failed to create TXD: ") + ansi_to_qt(except.message));

        // We failed.
        return;
    }

    if (newTXD == NULL)
    {
        this->mainWnd->txdLog->showError("unknown error in TXD creation");

        return;
    }

    this->mainWnd->setCurrentTXD(newTXD);

    this->mainWnd->clearCurrentFilePath();

    // Set the version of the entire TXD.
    this->mainWnd->newTxdName = this->txdName->text();

    rw::LibraryVersion libVer;
    this->GetSelectedVersion(libVer);

    int set = this->gameSelectBox->currentIndex();
    int platform = this->platSelectBox->currentIndex();
    int dataType = this->dataTypeSelectBox->currentIndex();

    RwVersionSets::eDataType dataTypeId;

    if (set == 0) // Custom
        dataTypeId = (RwVersionSets::eDataType)(dataType + 1);
    else
        dataTypeId = this->mainWnd->versionSets.sets[set - 1].availablePlatforms[platform].availableDataTypes[dataType];

    newTXD->SetEngineVersion(libVer);

    char const *currentPlatform = RwVersionSets::dataNameFromId(dataTypeId);

    this->mainWnd->SetRecommendedPlatform(currentPlatform);

    // Update the MainWindow stuff.
    this->mainWnd->updateWindowTitle();

    // Since the version has changed, the friendly icons should have changed.
    this->mainWnd->updateFriendlyIcons();

    this->close();
}

void CreateTxdDialog::OnRequestCancel(bool clicked)
{
    this->close();
}

void CreateTxdDialog::OnUpdateTxdName(const QString& newText)
{
    this->UpdateAccessibility();
}