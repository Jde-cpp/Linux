//#include "ApplicationLinux.h"
#include <syslog.h>
#include <execinfo.h>
#include <signal.h>

#include "../../Framework/source/application/Application.h"
#include "../../Framework/source/threading/InterruptibleThread.h"



#define var const auto
namespace Jde
{
	void IApplication::AddApplicationLog( ELogLevel level, const string& value )noexcept //called onterminate, needs to be static.
	{
		auto osLevel = LOG_DEBUG;
		if( level==ELogLevel::Debug )
			osLevel = LOG_INFO;
		else if( level==ELogLevel::Information )
			osLevel = LOG_NOTICE;
		else if( level==ELogLevel::Warning )
			osLevel = LOG_WARNING;
		else if( level==ELogLevel::Error )
			osLevel = LOG_ERR;
		else if( level==ELogLevel::Critical )
			osLevel = LOG_CRIT;
		syslog( osLevel, "%s",  value.c_str() );
	}
	set<string> OSApp::Startup( int argc, char** argv, string_view appName )noexcept(false)
	{
		IApplication::_pInstance = make_shared<OSApp>();
		return IApplication::_pInstance->BaseStartup( argc, argv, appName );
	}

	void OSApp::OSPause()noexcept
	{
		INFON( "Pausing main thread. {}", getpid() );//[*** LOG ERROR ***] [console] [argument index out of range] [2018-04-20 06:47:33]
		auto result = ::pause();
		INFON( "Pause returned - {}.", result );
		IApplication::Wait();
	}

	bool OSApp::AsService()noexcept
	{
		return ::daemon( 1, 0 )==0;
	}


	void IApplication::OnTerminate()noexcept
	{
		void *trace_elems[20];
		auto trace_elem_count( backtrace(trace_elems, 20) );
		char **stack_syms( backtrace_symbols(trace_elems, trace_elem_count) );
		ostringstream os;
		for( auto i = 0; i < trace_elem_count ; ++i )
			os << stack_syms[i] << std::endl;

		IApplication::AddApplicationLog( ELogLevel::Critical, os.str() );
		free( stack_syms );
		exit( EXIT_FAILURE );
	}

	string OSApp::GetEnvironmentVariable( string_view variable )noexcept
	{
		char* pEnv = std::getenv( string{variable}.c_str() );
		return pEnv ? string{pEnv} : string{};

	}
	fs::path OSApp::ProgramDataFolder()noexcept
	{
		return fs::path{ GetEnvironmentVariable("HOME"sv) };
	}

	void OSApp::ExitHandler( int s )
	{
	//	signal( s, SIG_IGN );
	//not supposed to log here...
		lock_guard l{_threadMutex};
		for( auto& pThread : *_pBackgroundThreads )
			pThread->Interrupt();
		//printf( "!!!!!!!!!!!!!!!!!!!!!Caught signal %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",s );
		//pProcessManager->Stop();
		//delete pLogger; pLogger = nullptr;
		//exit( 1 );
	}

	bool OSApp::KillInstance( uint processId )noexcept
	{
		var result = ::kill( processId, 14 );
		if( result )
			ERRN( "kill failed with '{}'.", result );
		else
			INFON( "kill sent to:  '{}'.", processId );
		return result==0;
	}

	void OSApp::AddSignals()noexcept(false)/*noexcept(false) for windows*/
	{
/* 		struct sigaction sigIntHandler;//_XOPEN_SOURCE
		memset( &sigIntHandler, 0, sizeof(sigIntHandler) );
		sigIntHandler.sa_handler = ExitHandler;
		sigemptyset( &sigIntHandler.sa_mask );
		sigIntHandler.sa_flags = 0;*/
		signal( SIGINT, OSApp::ExitHandler );
		signal( SIGSTOP, OSApp::ExitHandler );
		signal( SIGKILL, OSApp::ExitHandler );
		signal( SIGTERM, OSApp::ExitHandler );
		signal( SIGALRM, OSApp::ExitHandler );
		//sigaction( SIGSTOP, &sigIntHandler, nullptr );
		//sigaction( SIGKILL, &sigIntHandler, nullptr );
		//sigaction( SIGTERM, &sigIntHandler, nullptr );
	}

	void OSApp::SetConsoleTitle( string_view title )noexcept
	{
		std::cout << "\033]0;" << title << "\007";
	}
}
#undef var