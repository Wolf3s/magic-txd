#pragma once

struct MessageReceiver abstract
{
    virtual void OnMessage( const std::string& msg ) = 0;
    virtual void OnMessage( const std::wstring& msg ) = 0;

    virtual CFile* WrapStreamCodec( CFile *compressed ) = 0;
};

// Shared utilities for human-friendly RenderWare operations.
namespace rwkind
{
    enum eTargetPlatform
    {
        PLATFORM_UNKNOWN,
        PLATFORM_PC,
        PLATFORM_PS2,
        PLATFORM_PSP,
        PLATFORM_XBOX,
        PLATFORM_GC,
        PLATFORM_DXT_MOBILE,
        PLATFORM_PVR,
        PLATFORM_ATC,
        PLATFORM_UNC_MOBILE
    };

    enum eTargetGame
    {
        GAME_GTA3,
        GAME_GTAVC,
        GAME_GTASA,
        GAME_MANHUNT,
        GAME_BULLY,
        GAME_LCS,
        GAME_SHEROES
    };

    static inline bool GetTargetPlatformFromFriendlyString( const char *targetPlatform, rwkind::eTargetPlatform& platOut )
    {
        if ( stricmp( targetPlatform, "PC" ) == 0 )
        {
            platOut = PLATFORM_PC;
            return true;
        }
        else if ( stricmp( targetPlatform, "PS2" ) == 0 ||
                  stricmp( targetPlatform, "Playstation 2" ) == 0 ||
                  stricmp( targetPlatform, "PlayStation2" ) == 0 )
        {
            platOut = PLATFORM_PS2;
            return true;
        }
        else if ( stricmp( targetPlatform, "XBOX" ) == 0 )
        {
            platOut = PLATFORM_XBOX;
            return true;
        }
        else if ( stricmp( targetPlatform, "Gamecube" ) == 0 ||
                  stricmp( targetPlatform, "GCube" ) == 0 ||
                  stricmp( targetPlatform, "GC" ) == 0 )
        {
            platOut = PLATFORM_GC;
            return true;
        }
        else if ( stricmp( targetPlatform, "DXT_MOBILE" ) == 0 ||
                  stricmp( targetPlatform, "S3TC_MOBILE" ) == 0 ||
                  stricmp( targetPlatform, "MOBILE_DXT" ) == 0 ||
                  stricmp( targetPlatform, "MOBILE_S3TC" ) == 0 )
        {
            platOut = PLATFORM_DXT_MOBILE;
            return true;
        }
        else if ( stricmp( targetPlatform, "PVR" ) == 0 ||
                  stricmp( targetPlatform, "PowerVR" ) == 0 ||
                  stricmp( targetPlatform, "PVRTC" ) == 0 )
        {
            platOut = PLATFORM_PVR;
            return true;
        }
        else if ( stricmp( targetPlatform, "ATC" ) == 0 ||
                  stricmp( targetPlatform, "ATI_Compress" ) == 0 ||
                  stricmp( targetPlatform, "ATI" ) == 0 ||
                  stricmp( targetPlatform, "ATITC" ) == 0 ||
                  stricmp( targetPlatform, "ATI TC" ) == 0 )
        {
            platOut = PLATFORM_ATC;
            return true;
        }
        else if ( stricmp( targetPlatform, "UNC" ) == 0 ||
                  stricmp( targetPlatform, "UNCOMPRESSED" ) == 0 ||
                  stricmp( targetPlatform, "unc_mobile" ) == 0 ||
                  stricmp( targetPlatform, "uncompressed_mobile" ) == 0 ||
                  stricmp( targetPlatform, "mobile_unc" ) == 0 ||
                  stricmp( targetPlatform, "mobile_uncompressed" ) == 0 )
        {
            platOut = PLATFORM_UNC_MOBILE;
            return true;
        }

        return false;
    }

    static inline bool GetTargetGameFromFriendlyString( const char *targetVersion, rwkind::eTargetGame& gameOut )
    {
        if ( stricmp( targetVersion, "SA" ) == 0 ||
             stricmp( targetVersion, "SanAndreas" ) == 0 ||
             stricmp( targetVersion, "San Andreas" ) == 0 ||
             stricmp( targetVersion, "GTA SA" ) == 0 ||
             stricmp( targetVersion, "GTASA" ) == 0 )
        {
            gameOut = GAME_GTASA;
            return true;
        }
        else if ( stricmp( targetVersion, "VC" ) == 0 ||
                  stricmp( targetVersion, "ViceCity" ) == 0 ||
                  stricmp( targetVersion, "Vice City" ) == 0 ||
                  stricmp( targetVersion, "GTA VC" ) == 0 ||
                  stricmp( targetVersion, "GTAVC" ) == 0 )
        {
            gameOut = GAME_GTAVC;
            return true;
        }
        else if ( stricmp( targetVersion, "GTAIII" ) == 0 ||
                  stricmp( targetVersion, "III" ) == 0 ||
                  stricmp( targetVersion, "GTA3" ) == 0 ||
                  stricmp( targetVersion, "GTA 3" ) == 0 )
        {
            gameOut = GAME_GTA3;
            return true;
        }
        else if ( stricmp( targetVersion, "MANHUNT" ) == 0 ||
                  stricmp( targetVersion, "MHUNT" ) == 0 ||
                  stricmp( targetVersion, "MH" ) == 0 )
        {
            gameOut = GAME_MANHUNT;
            return true;
        }
        else if ( stricmp( targetVersion, "BULLY" ) == 0 )
        {
            gameOut = GAME_BULLY;
            return true;
        }
        else if ( stricmp( targetVersion, "SHEROES" ) == 0 ||
                  stricmp( targetVersion, "Sonic Heroes" ) == 0 ||
                  stricmp( targetVersion, "SonicHeroes" ) == 0 )
        {
            gameOut = GAME_SHEROES;
            return true;
        }

        return false;
    }

    inline bool GetTargetVersionFromPlatformAndGame(
        rwkind::eTargetPlatform targetPlatform, rwkind::eTargetGame targetGame,
        rw::LibraryVersion& verOut, const char*& strTargetVerOut
    )
    {
        if ( targetGame == GAME_GTA3 )
        {
            if ( targetPlatform == PLATFORM_XBOX )
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::GTA3_XBOX );
            }
            else
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::GTA3_PC );
            }

            strTargetVerOut = "GTA 3";
            return true;
        }
        else if ( targetGame == GAME_GTAVC )
        {
            if ( targetPlatform == PLATFORM_PS2 )
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::VC_PS2 );
            }
            else if ( targetPlatform == PLATFORM_XBOX )
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::VC_XBOX );
            }
            else
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::VC_PC );
            }

            strTargetVerOut = "Vice City";
            return true;
        }
        else if ( targetGame == GAME_GTASA )
        {
            verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::SA );

            strTargetVerOut = "San Andreas";
            return true;
        }
        else if ( targetGame == GAME_MANHUNT )
        {
            if ( targetPlatform == PLATFORM_PS2 )
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::MANHUNT_PS2 );
            }
            else
            {
                verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::MANHUNT_PC );
            }

            strTargetVerOut = "Manhunt";
            return true;
        }
        else if ( targetGame == GAME_BULLY )
        {
            verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::BULLY );

            strTargetVerOut = "Bully";
            return true;
        }
        else if ( targetGame == GAME_LCS )
        {
            verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::LCS_PSP );

            strTargetVerOut = "Liberty City Stories";
            return true;
        }
        else if ( targetGame == GAME_SHEROES )
        {
            verOut = rw::KnownVersions::getGameVersion( rw::KnownVersions::SHEROES_GC );

            strTargetVerOut = "Sonic Heroes";
            return true;
        }

        return false;
    }

    static inline eTargetPlatform GetRasterPlatform( rw::Raster *texRaster )
    {
        eTargetPlatform thePlatform = PLATFORM_UNKNOWN;

        if ( texRaster->hasNativeDataOfType( "Direct3D8" ) || texRaster->hasNativeDataOfType( "Direct3D9" ) )
        {
            thePlatform = PLATFORM_PC;
        }
        else if ( texRaster->hasNativeDataOfType( "XBOX" ) )
        {
            thePlatform = PLATFORM_XBOX;
        }
        else if ( texRaster->hasNativeDataOfType( "PlayStation2" ) )
        {
            thePlatform = PLATFORM_PS2;
        }
        else if ( texRaster->hasNativeDataOfType( "PSP" ) )
        {
            thePlatform = PLATFORM_PSP;
        }
        else if ( texRaster->hasNativeDataOfType( "Gamecube" ) )
        {
            thePlatform = PLATFORM_GC;
        }
        else if ( texRaster->hasNativeDataOfType( "s3tc_mobile" ) )
        {
            thePlatform = PLATFORM_DXT_MOBILE;
        }
        else if ( texRaster->hasNativeDataOfType( "PowerVR" ) )
        {
            thePlatform = PLATFORM_PVR;
        }
        else if ( texRaster->hasNativeDataOfType( "AMDCompress" ) )
        {
            thePlatform = PLATFORM_ATC;
        }
        else if ( texRaster->hasNativeDataOfType( "uncompressed_mobile" ) )
        {
            thePlatform = PLATFORM_UNC_MOBILE;
        }

        return thePlatform;
    }

    static inline double GetPlatformQualityGrade( eTargetPlatform platform )
    {
        double quality = 0.0;

        if ( platform == PLATFORM_PC )
        {
            quality = 1.0;
        }
        else if ( platform == PLATFORM_XBOX )
        {
            quality = 1.0;
        }
        else if ( platform == PLATFORM_PS2 )
        {
            quality = 1.0;
        }
        else if ( platform == PLATFORM_PSP )
        {
            quality = 1.0;
        }
        else if ( platform == PLATFORM_GC )
        {
            quality = 0.95;
        }
        else if ( platform == PLATFORM_DXT_MOBILE )
        {
            quality = 0.7;
        }
        else if ( platform == PLATFORM_PVR )
        {
            quality = 0.4;
        }
        else if ( platform == PLATFORM_ATC )
        {
            quality = 0.8;
        }
        else if ( platform == PLATFORM_UNC_MOBILE )
        {
            quality = 0.9;
        }

        return quality;
    }

    static inline bool ShouldRasterConvertBeforehand( rw::Raster *texRaster, eTargetPlatform targetPlatform )
    {
        bool shouldBeforehand = false;

        if ( targetPlatform != PLATFORM_UNKNOWN )
        {
            eTargetPlatform rasterPlatform = GetRasterPlatform( texRaster );

            if ( rasterPlatform != PLATFORM_UNKNOWN )
            {
                if ( targetPlatform != rasterPlatform )
                {
                    // Decide based on the raster and target platform.
                    // Basically, we want to improve the quality and the conversion speed.
                    // We want to convert beforehand if we convert from a lower quality texture to a higher quality.
                    double sourceQuality = GetPlatformQualityGrade( rasterPlatform );
                    double targetQuality = GetPlatformQualityGrade( targetPlatform );

                    if ( sourceQuality == targetQuality )
                    {
                        // If the quality of the platforms does not change, we do not care.
                        shouldBeforehand = false;
                    }
                    else if ( sourceQuality < targetQuality )
                    {
                        // If the quality of the current raster is worse than the target, we should.
                        shouldBeforehand = true;
                    }
                    else if ( sourceQuality > targetQuality )
                    {
                        // If the quality of the current raster is better than the target, we should not.
                        shouldBeforehand = false;
                    }
                }
            }
        }

        return shouldBeforehand;
    }

    static inline const char* GetTargetNativeFormatName( eTargetPlatform targetPlatform, eTargetGame targetGame )
    {
        if ( targetPlatform == PLATFORM_PS2 )
        {
            return "PlayStation2";
        }
        else if ( targetPlatform == PLATFORM_XBOX )
        {
            return "XBOX";
        }
        else if ( targetPlatform == PLATFORM_PC )
        {
            // Depends on the game.
            if (targetGame == GAME_GTASA)
            {
                return "Direct3D9";
            }
            
            return "Direct3D8";
        }
        else if ( targetPlatform == PLATFORM_PSP )
        {
            return "PSP";
        }
        else if ( targetPlatform == PLATFORM_GC )
        {
            return "Gamecube";
        }
        else if ( targetPlatform == PLATFORM_DXT_MOBILE )
        {
            return "s3tc_mobile";
        }
        else if ( targetPlatform == PLATFORM_PVR )
        {
            return "PowerVR";
        }
        else if ( targetPlatform == PLATFORM_ATC )
        {
            return "AMDCompress";
        }
        else if ( targetPlatform == PLATFORM_UNC_MOBILE )
        {
            return "uncompressed_mobile";
        }
        
        return NULL;
    }

    static inline bool ConvertRasterToPlatform( rw::Raster *texRaster, eTargetPlatform targetPlatform, eTargetGame targetGame )
    {
        bool hasConversionSucceeded = false;

        const char *nativeName = GetTargetNativeFormatName( targetPlatform, targetGame );

        if ( nativeName )
        {
            hasConversionSucceeded = rw::ConvertRasterTo( texRaster, nativeName );
        }
        else
        {
            assert( 0 );
        }

        return hasConversionSucceeded;
    }
};