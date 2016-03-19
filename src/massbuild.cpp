#include "mainwindow.h"
#include "massbuild.h"

#include "taskcompletionwindow.h"

#include "guiserialization.hxx"

#include "qtsharedlogic.h"

#include "toolshared.hxx"

#include "tools/txdbuild.h"

#include "qtutils.h"
#include "languages.h"

using namespace toolshare;

struct massbuildEnv : public magicSerializationProvider
{
    inline void Initialize( MainWindow *mainWnd )
    {
        LIST_CLEAR( this->windows.root );

        RegisterMainWindowSerialization( mainWnd, MAGICSERIALIZE_MASSBUILD, this );

        // Initialize with some defaults.
        this->closeOnCompletion = true;
    }

    inline void Shutdown( MainWindow *mainWnd )
    {
        UnregisterMainWindowSerialization( mainWnd, MAGICSERIALIZE_MASSBUILD );

        // Close all windows.
        while ( !LIST_EMPTY( this->windows.root ) )
        {
            MassBuildWindow *wnd = LIST_GETITEM( MassBuildWindow, this->windows.root.next, node );

            delete wnd;
        }
    }

    struct massbuild_cfg_struct
    {
        endian::little_endian <rwkind::eTargetGame> targetGame;
        endian::little_endian <rwkind::eTargetPlatform> targetPlatform;
        bool generateMipmaps;
        endian::little_endian <std::int32_t> genMipMaxLevel;
        bool closeOnCompletion;
    };

    void Load( MainWindow *mainWnd, rw::BlockProvider& cfgBlock ) override
    {
        // Load our state.
        RwReadUnicodeString( cfgBlock, this->config.gameRoot );
        RwReadUnicodeString( cfgBlock, this->config.outputRoot );

        massbuild_cfg_struct cfgStruct;
        cfgBlock.readStruct( cfgStruct );

        this->config.targetGame = cfgStruct.targetGame;
        this->config.targetPlatform = cfgStruct.targetPlatform;
        this->config.generateMipmaps = cfgStruct.generateMipmaps;
        this->config.curMipMaxLevel = cfgStruct.genMipMaxLevel;
        this->closeOnCompletion = cfgStruct.closeOnCompletion;
    }

    void Save( const MainWindow *mainWnd, rw::BlockProvider& cfgBlock ) const override
    {
        // Save our state.
        RwWriteUnicodeString( cfgBlock, this->config.gameRoot );
        RwWriteUnicodeString( cfgBlock, this->config.outputRoot );

        massbuild_cfg_struct cfgStruct;
        cfgStruct.targetGame = this->config.targetGame;
        cfgStruct.targetPlatform = this->config.targetPlatform;
        cfgStruct.generateMipmaps = this->config.generateMipmaps;
        cfgStruct.genMipMaxLevel = this->config.curMipMaxLevel;
        cfgStruct.closeOnCompletion = this->closeOnCompletion;
        
        cfgBlock.writeStruct( cfgStruct );
    }

    TxdBuildModule::run_config config;
    bool closeOnCompletion;

    RwList <MassBuildWindow> windows;
};

static PluginDependantStructRegister <massbuildEnv, mainWindowFactory_t> massbuildEnvRegister;

MassBuildWindow::MassBuildWindow( MainWindow *mainWnd ) : QDialog( mainWnd )
{
    // We want to create a dialog that spawns a task dialog with a build task with the configuration it set.
    // So you can create this dialog and spawn multiple build tasks from it.

    this->mainWnd = mainWnd;

    massbuildEnv *env = massbuildEnvRegister.GetPluginStruct( mainWnd );

    this->setWindowFlags( this->windowFlags() & ~Qt::WindowContextHelpButtonHint );

    this->setAttribute( Qt::WA_DeleteOnClose );

    // We want a simple vertical dialog that is of fixed size and has buttons at the bottom.
    MagicLayout<QVBoxLayout> layout;
    
    // First the ability to select the paths.
    layout.top->addLayout(
        qtshared::createGameRootInputOutputForm(
            env->config.gameRoot,
            env->config.outputRoot,
            this->editGameRoot,
            this->editOutputRoot
        )
    );

    layout.top->addSpacing( 10 );

    // Then we should have a target configuration.
    createTargetConfigurationComponents(
        layout.top,
        env->config.targetPlatform,
        env->config.targetGame,
        this->selGameBox,
        this->selPlatformBox
    );

    layout.top->addSpacing( 10 );

    // Now some basic properties that the user might want to do globally.
    layout.top->addLayout(
        qtshared::createMipmapGenerationGroup(
            this,
            env->config.generateMipmaps,
            env->config.curMipMaxLevel,
            this->propGenMipmaps,
            this->propGenMipmapsMax
        )
    );

    // Meta-properties.
    QCheckBox *propCloseAfterComplete = CreateCheckBoxL( "Tools.MassBld.CloseOnCmplt" );

    propCloseAfterComplete->setChecked( env->closeOnCompletion );

    layout.top->addWidget( propCloseAfterComplete );

    this->propCloseAfterComplete = propCloseAfterComplete;

    layout.top->addSpacing( 15 );

    // Last thing is the typical button row.
    QPushButton *buttonBuild = CreateButtonL( "Tools.MassBld.Build" );

    connect( buttonBuild, &QPushButton::clicked, this, &MassBuildWindow::OnRequestBuild );

    layout.bottom->addWidget( buttonBuild );

    QPushButton *buttonCancel = CreateButtonL( "Tools.MassBld.Cancel" );

    connect( buttonCancel, &QPushButton::clicked, this, &MassBuildWindow::OnRequestCancel );

    layout.bottom->addWidget( buttonCancel );

    this->setLayout(layout.root);

    RegisterTextLocalizationItem( this );

    // We want to know about all active windows.
    LIST_INSERT( env->windows.root, this->node );
}

MassBuildWindow::~MassBuildWindow( void )
{
    // Remove us from the registry.
    LIST_REMOVE( this->node );

    UnregisterTextLocalizationItem( this );
}

void MassBuildWindow::updateContent( MainWindow *mainWnd )
{
    this->setWindowTitle( MAGIC_TEXT("Tools.MassBld.Desc") );
}

void MassBuildWindow::serialize( void )
{
    massbuildEnv *env = massbuildEnvRegister.GetPluginStruct( this->mainWnd );

    env->config.gameRoot = this->editGameRoot->text().toStdWString();
    env->config.outputRoot = this->editOutputRoot->text().toStdWString();

    platformToNaturalList.getCurrent( this->selPlatformBox, env->config.targetPlatform );
    gameToNaturalList.getCurrent( this->selPlatformBox, env->config.targetGame );

    env->config.generateMipmaps = this->propGenMipmaps->isChecked();
    env->config.curMipMaxLevel = this->propGenMipmapsMax->text().toInt();

    env->closeOnCompletion = this->propCloseAfterComplete->isChecked();
}

struct MassBuildModule : public TxdBuildModule
{
    inline MassBuildModule( TaskCompletionWindow *taskWnd, rw::Interface *rwEngine ) : TxdBuildModule( rwEngine )
    {
        this->taskWnd = taskWnd;
    }

    void OnMessage( const std::string& msg ) override
    {
        taskWnd->updateStatusMessage( QString::fromStdString( msg ) );
    }

    void OnMessage( const std::wstring& msg ) override
    {
        taskWnd->updateStatusMessage( QString::fromStdWString( msg ) );
    }

    // Redirect every file access to the global stream function.
    CFile*  WrapStreamCodec( CFile *compressed ) override
    {
        return CreateDecompressedStream( taskWnd->getMainWindow(), compressed );
    }

    TaskCompletionWindow *taskWnd;
};

struct massbuild_task_params
{
    inline massbuild_task_params( const TxdBuildModule::run_config& config ) : config( config )
    {
        return;
    }

    TxdBuildModule::run_config config;
    TaskCompletionWindow *taskWnd;
    MainWindow *mainWnd;
};

static void massbuild_task_entry( rw::thread_t threadHandle, rw::Interface *engineInterface, void *ud )
{
    massbuild_task_params *params = (massbuild_task_params*)ud;

    try
    {
        // Run the mass build module.
        MassBuildModule module( params->taskWnd, engineInterface );

        module.RunApplication( params->config );
    }
    catch( ... )
    {
        delete params;

        throw;
    }

    // Make sure to free memory.
    delete params;
}

void MassBuildWindow::OnRequestBuild( bool checked )
{   
    this->serialize();

    rw::Interface *rwEngine = this->mainWnd->GetEngine();

    MainWindow *mainWnd = this->mainWnd;

    massbuildEnv *env = massbuildEnvRegister.GetPluginStruct( mainWnd );

    // Create a copy of our configuration.
    massbuild_task_params *params = new massbuild_task_params( env->config );
    params->mainWnd = mainWnd;

    // Create the task.
    rw::thread_t taskThread = rw::MakeThread( rwEngine, massbuild_task_entry, params );

    // Communicate using a task window.
    TaskCompletionWindow *taskWnd = new LogTaskCompletionWindow( mainWnd, taskThread, "Building...", "preparing the build process..." );

    taskWnd->setCloseOnCompletion( env->closeOnCompletion );

    params->taskWnd = taskWnd;

    taskWnd->setVisible( true );

    rw::ResumeThread( rwEngine, taskThread );

    this->close();
}

void MassBuildWindow::OnRequestCancel( bool checked )
{
    this->close();
}

void InitializeMassBuildEnvironment( void )
{
    massbuildEnvRegister.RegisterPlugin( mainWindowFactory );
}