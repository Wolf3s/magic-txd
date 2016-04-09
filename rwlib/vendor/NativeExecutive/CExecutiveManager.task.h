/*****************************************************************************
*
*  PROJECT:     Native Executive
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        NativeExecutive/CExecutiveManager.task.h
*  PURPOSE:     Runtime for quick parallel execution sheduling
*  DEVELOPERS:  Martin Turski <quiret@gmx.de>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef _EXECUTIVE_MANAGER_TASKS_
#define _EXECUTIVE_MANAGER_TASKS_

BEGIN_NATIVE_EXECUTIVE

class CExecTask
{
public:
    friend class CExecutiveManager;
    friend class CExecutiveGroup;

    CFiber *runtimeFiber;

    typedef void (__stdcall*taskexec_t)( CExecTask *task, void *userdata );

    taskexec_t callback;

    RwListEntry <CExecTask> node;

    void *finishEvent;

    bool isInitialized;
    bool isOnProcessedList;
    std::atomic <int> usageCount;

    CExecTask( CExecutiveManager *manager, CFiber *runtime );
    ~CExecTask( void );

    void    Execute( void );
    void    WaitForFinish( void );

    AINLINE bool IsFinished( void )
    {
        return ( usageCount == 0 );
    }

    CExecutiveManager *manager;
};

END_NATIVE_EXECUTIVE

#endif //_EXECUTIVE_MANAGER_TASKS_