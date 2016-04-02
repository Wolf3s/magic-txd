#ifndef _PLUGIN_UTILITIES_
#define _PLUGIN_UTILITIES_

#include <PluginHelpers.h>

namespace rw
{

// Helper to place a RW lock into a StaticPluginClassFactory type.
template <typename factoryType, typename getFactCallbackType>
struct factLockProviderEnv
{
private:
    typedef typename factoryType::hostType_t hostType_t;

    struct dynamic_rwlock
    {
        inline void Initialize( hostType_t *host )
        {
            CreatePlacedReadWriteLock( host->engineInterface, this );
        }

        inline void Shutdown( hostType_t *host )
        {
            ClosePlacedReadWriteLock( host->engineInterface, (rwlock*)this );
        }

        inline void operator = ( const dynamic_rwlock& right )
        {
            // Nothing to do here.
            return;
        }
    };

public:
    inline void Initialize( EngineInterface *intf )
    {
        size_t rwlock_struct_size = GetReadWriteLockStructSize( intf );

        typename factoryType::pluginOffset_t lockPluginOffset = factoryType::INVALID_PLUGIN_OFFSET;

        factoryType *theFact = getFactCallbackType::getFactory( intf );

        if ( theFact )
        {
            lockPluginOffset =
                theFact->RegisterDependantStructPlugin <dynamic_rwlock> ( factoryType::ANONYMOUS_PLUGIN_ID, rwlock_struct_size );
        }

        this->lockPluginOffset = lockPluginOffset;
    }

    inline void Shutdown( EngineInterface *intf )
    {
        if ( factoryType::IsOffsetValid( this->lockPluginOffset ) )
        {
            factoryType *theFact = getFactCallbackType::getFactory( intf );

            theFact->UnregisterPlugin( this->lockPluginOffset );
        }
    }

    inline rwlock* GetLock( const hostType_t *host ) const
    {
        return (rwlock*)factoryType::RESOLVE_STRUCT <rwlock> ( host, this->lockPluginOffset );
    }

private:
    typename factoryType::pluginOffset_t lockPluginOffset;
};

namespace rwFactRegPipes
{

struct rw_fact_pipeline_base
{
    AINLINE static RwTypeSystem& getTypeSystem( EngineInterface *intf )
    {
        return intf->typeSystem;
    }
};

template <typename constrType>
using rw_defconstr_fact_pipeline_base = factRegPipes::defconstr_fact_pipeline_base <EngineInterface, constrType>;

}

}

#endif //_PLUGIN_UTILITIES_