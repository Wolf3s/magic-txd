#include "mainwindow.h"

void MainWindow::updateFriendlyIcons()
{
    // Decide, based on the currently selected texture, what icons to show.

    this->bShowFriendlyIcons = false;

    if (this->currentTXD)
    {
        // The idea is that games were released with distinctive RenderWare versions and raster configurations.
        // We should decide very smartly what icons we want to show, out of a limited set.

        QString currentPlatform = this->GetCurrentPlatform();
        RwVersionSets::eDataType dataTypeId = RwVersionSets::dataIdFromEnginePlatformName(currentPlatform);

        if (dataTypeId != RwVersionSets::RWVS_DT_NOT_DEFINED)
        {
            rw::LibraryVersion libVersion = this->currentTXD->GetEngineVersion();
            int set, platform, data;

            if (this->versionSets.matchSet(libVersion, dataTypeId, set, platform, data))
            {
                const RwVersionSets::Set& currentSet = this->versionSets.sets[ set ];
                const RwVersionSets::Set::Platform& currentPlatform = currentSet.availablePlatforms[ platform ];

                RwVersionSets::ePlatformType platformType = currentPlatform.platformType;

                if (platformType != RwVersionSets::RWVS_PL_NOT_DEFINED)
                {
                    if (this->showGameIcon)
                    {
                        // Prepare platform icon.
                        QString platIconPath;

                        switch (platformType)
                        {
                        case RwVersionSets::RWVS_PL_PC:
                            platIconPath = makeAppPath("resources\\icons\\pc.png");
                            break;
                        case RwVersionSets::RWVS_PL_PS2:
                            platIconPath = makeAppPath("resources\\icons\\ps2.png");
                            break;
                        case RwVersionSets::RWVS_PL_PSP:
                            platIconPath = makeAppPath("resources\\icons\\psp.png");
                            break;
                        case RwVersionSets::RWVS_PL_XBOX:
                            platIconPath = makeAppPath("resources\\icons\\xbox.png");
                            break;
                        case RwVersionSets::RWVS_PL_GAMECUBE:
                            platIconPath = makeAppPath("resources\\icons\\gamecube.png");
                            break;
                        case RwVersionSets::RWVS_PL_MOBILE:
                            platIconPath = makeAppPath("resources\\icons\\mobile.png");
                            break;
                        }

                        bool doShowPlatIcon = ( platIconPath.isEmpty() == false );

                        if ( doShowPlatIcon )
                        {
                            // Prepare game icon.
                            bool hasGoodGameIcon = false;

                            if (!currentSet.iconName.isEmpty())
                            {
                                QString gameIconPath = makeAppPath("resources\\icons\\") + currentSet.iconName + ".png";

                                this->friendlyIconGame->setPixmap( QPixmap( gameIconPath ) );

                                hasGoodGameIcon = true;
                            }
                            else if ( currentSet.displayName.isEmpty() == false )
                            {
                                this->friendlyIconGame->setText( currentSet.displayName );

                                hasGoodGameIcon = true;
                            }

                            if ( hasGoodGameIcon )
                            {
                                this->friendlyIconPlatform->setPixmap( QPixmap( platIconPath ) );
                                this->bShowFriendlyIcons = true;
                            }
                        }
                    }
                }
                else if (!currentSet.displayName.isEmpty())
                {
                    QString platName = RwVersionSets::platformNameFromId( platformType );

                    if (!platName.isEmpty()) {
                        this->friendlyIconGame->setText(currentSet.displayName);
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