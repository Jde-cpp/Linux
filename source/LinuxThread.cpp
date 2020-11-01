#include "../../Framework/source/threading/Thread.cpp"
#include <sys/prctl.h>

namespace Jde
{
		const char* Threading::GetThreadDescription()noexcept
		{
			if( std::strlen(ThreadName)==0 )
			{
				ThreadId = pthread_self();
				//char thread_name[NameLength];
				var rc = pthread_getname_np( ThreadId, ThreadName, NameLength );
    			if (rc != 0)
        			ERR( "pthread_getname_np returned {}"sv, rc );
			}
			return ThreadName;
		}

	void Threading::SetThreadDscrptn( std::thread& thread, std::string_view pszDescription )noexcept
	{
	   pthread_setname_np( thread.native_handle(), string(pszDescription).c_str() );
	}

	void Threading::SetThreadDscrptn( const std::string& description )noexcept
	{
		strncpy( ThreadName, description.c_str(), NameLength );
		prctl( PR_SET_NAME, ThreadName, 0, 0, 0 );
		ThreadId = pthread_self();
	}
	uint Threading::GetThreadId()noexcept{ return ThreadId ? ThreadId : ThreadId = pthread_self(); }
}