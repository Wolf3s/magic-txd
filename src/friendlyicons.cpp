#include "mainwindow.h"

void MainWindow::updateFriendlyIcons()
{
    // Decide, based on the currently selected texture, what icons to show.

    this->bShowFriendlyIcons = false;

    if(this->currentTXD) {
        // The idea is that games were released with distinctive RenderWare versions and raster configurations.
        // We should decide very smartly what icons we want to show, out of a limited set.

        QString currentPlatform = this->GetCurrentPlatform();
        RwVersionSets::eDataType dataTypeId = RwVersionSets::dataIdFromEnginePlatformName(currentPlatform);

        if (dataTypeId != RwVersionSets::RWVS_DT_NOT_DEFINED) {
            rw::LibraryVersion libVersion = this->currentTXD->GetEngineVersion();
            int set, platform, data;
            if (this->versionSets.matchSet(libVersion, dataTypeId, set, platform, data)) {
                RwVersionSets::ePlatformType platformType = this->versionSets.sets[set].availablePlatforms[platform].platformType;
                if (platformType != RwVersionSets::RWVS_PL_NOT_DEFINED) {
                    if (this->showGameIcon) {
                        if (!this->versionSets.sets[set].iconName.isEmpty()) {
                            QString iconPath = makeAppPath("resources\\icons\\") + this->versionSets.sets[set].iconName + ".png";
                            QString platIconPath;
                            switch (platformType) {
                            case RwVersionSets::RWVS_PL_PC:
                                platIconPath = makeAppPath("resources\\icons\\pc.png");
                                break;
                            case RwVersionSets::RWVS_PL_PS2:
                                platIconPath = makeAppPath("resources\\icons\\ps2.png");
                                break;
                            case RwVersionSets::RWVS_PL_XBOX:
                                platIconPath = makeAppPath("resources\\icons\\xbox.png");
                                break;
                            case RwVersionSets::RWVS_PL_MOBILE:
                                platIconPath = makeAppPath("resources\\icons\\mobile.png");
                                break;
                            }
                            if (!platIconPath.isEmpty()) {
                                this->friendlyIconGame->setPixmap(QPixmap(iconPath));
                                this->friendlyIconPlatform->setPixmap(QPixmap(platIconPath));
                                this->bShowFriendlyIcons = true;
                            }
                        }
                    }
                }
                else if (!this->versionSets.sets[set].displayName.isEmpty()) {
                    QString platName;
                    switch (platformType) {
                    case RwVersionSets::RWVS_PL_PC:
                        platName = "PC";
                        break;
                    case RwVersionSets::RWVS_PL_PS2:
                        platName = "PS2";
                        break;
                    case RwVersionSets::RWVS_PL_XBOX:
                        platName = "XBOX";
                        break;
                    case RwVersionSets::RWVS_PL_MOBILE:
                        platName = "Mobile";
                        break;
                    }
                    if (!platName.isEmpty()) {
                        this->friendlyIconGame->setText(this->versionSets.sets[set].displayName);
                        this->friendlyIconPlatform->setText(platName);
                        this->bShowFriendlyIcons = true;
                    }
                }
            }
        }
    }
    this->friendlyIconGame->setVisible(this->bShowFriendlyIcons);
    this->friendlyIconSeparator->setVisible(this->bShowFriendlyIcons);
    this->friendlyIconPlatform->setVisible(this->bShowFriendlyIcons);
}