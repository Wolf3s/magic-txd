#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPlainTextEdit>

#include "languages.h"

#include "progresslogedit.h"

struct MassConvertWindow;

struct MassConvertWindow : public QDialog, public magicTextLocalizationItem
{
    friend struct massconvEnv;

public:
    MassConvertWindow( MainWindow *mainwnd );
    ~MassConvertWindow();

    void postLogMessage( QString msg );

    MainWindow *mainwnd;

public slots:
    void OnRequestConvert( bool checked );
    void OnRequestCancel( bool checked );

protected:
    void updateContent( MainWindow *mainWnd ) override;

    void customEvent( QEvent *evt ) override;

private:
    void serialize( void );

    // Widget pointers.
    MagicLineEdit *editGameRoot;
    MagicLineEdit *editOutputRoot;
    QComboBox *selPlatformBox;
    QComboBox *selGameBox;
    QCheckBox *propClearMipmaps;
    QCheckBox *propGenMipmaps;
    MagicLineEdit *propGenMipmapsMax;
    QCheckBox *propImproveFiltering;
    QCheckBox *propCompressTextures;
    QCheckBox *propReconstructIMG;
    QCheckBox *propCompressedIMG;

    ProgressLogEdit logEditControl;

    QPushButton *buttonConvert;

public:
    volatile rw::thread_t conversionThread;

    rw::rwlock *volatile convConsistencyLock;

    RwListEntry <MassConvertWindow> node;
};