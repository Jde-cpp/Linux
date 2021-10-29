//#include "ApplicationLinux.h"
#include <syslog.h>
#include <execinfo.h>
#include <signal.h>
#include <dlfcn.h>

#include "LinuxDrive.h"
#include <jde/App.h>
#include "../../Framework/source/threading/InterruptibleThread.h"



#define var const auto
namespace Jde
{
	α OSApp::FreeLibrary( void* p )noexcept->void
	{
		::dlclose( p );
	}

	α OSApp::LoadLibrary( path path )noexcept(false)->void*
	{
		auto p = ::dlopen( path.c_str(), RTLD_LAZY );  THROW_IFX2( !p, IO_EX(path, "Can not load library - '{}'", dlerror()) );
		INFO( "({})Opened"sv, path.string() );
		return p;
	}
	α OSApp::GetProcAddress( void* pModule, str procName )noexcept(false)->void*
	{
		auto p = ::dlsym( pModule, procName.c_str() ); CHECK( p );
		return p;
	}
	α OSApp::Install( str serviceDescription )noexcept(false)->void
	{
		THROW( "Not Implemeented" );
	}

	α OSApp::Uninstall()noexcept(false)->void
	{
		THROW( "Not Implemeented");
	}

	α OSApp::Executable()noexcept->fs::path
	{
		return fs::path{ program_invocation_name };
	}

	α IApplication::AddApplicationLog( ELogLevel level, str value )noexcept->void //called onterminate, needs to be static.
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

	α IApplication::MemorySize()noexcept->size_t//https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
	{
		uint size = 0;
		FILE* fp = fopen( "/proc/self/statm", "r" );
		if( fp!=nullptr )
		{
			long rss = 0L;
			if( fscanf( fp, "%*s%ld", &rss ) == 1 )
				size = (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
			fclose( fp );
		}
		return size;
	}

	α IApplication::Path()noexcept->fs::path
	{
		return std::filesystem::canonical( "/proc/self/exe" ).parent_path();
		return fs::path( program_invocation_name );
	}

	string IApplication::HostName()noexcept
	{
		constexpr uint maxHostName = HOST_NAME_MAX;
		char hostname[maxHostName];
		gethostname( hostname, maxHostName );
		return hostname;
	}

	uint OSApp::ProcessId()noexcept
	{
		return getpid();
	}

	flat_set<string> OSApp::Startup( int argc, char** argv, sv appName, string serviceDescription )noexcept(false)
	{
		IApplication::_pInstance = make_shared<OSApp>();
		return IApplication::_pInstance->BaseStartup( argc, argv, appName, serviceDescription );
	}
	atomic<bool> _workerMutex{false};
	vector<sp<Threading::IWorker>> _workers;

	α OSApp::Pause()noexcept->void
	{
//		DBG( "Pausing main thread. {}", getpid() );
		::pause();
/*		for( ;; )
		{
			break;//not implemented yet.
			auto pWorker = _activeWorkers.WaitAndPop();
			if( pWorker->Poll() )
				AddActiveWorker( pWorker );//make sure doesn't loop forever.
		}*/
//		DBG( "::pause errno={}.", errno );
		//IApplication::Wait();
	}

	bool OSApp::AsService()noexcept
	{
		return ::daemon( 1, 0 )==0;
	}

	α IApplication::OnTerminate()noexcept->void
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

	string OSApp::GetEnvironmentVariable( sv variable )noexcept
	{
		char* pEnv = std::getenv( string{variable}.c_str() );
		return pEnv ? string{pEnv} : string{};

	}
	α OSApp::ProgramDataFolder()noexcept->fs::path
	{
		return fs::path{ GetEnvironmentVariable("HOME") };
	}

	α OSApp::ExitHandler( int s )->void
	{
		Exit( s );
		//ASSERT( false ); //TODO handle
	//	signal( s, SIG_IGN );
	//not supposed to log here...
		//printf( "!!!!!!!!!!!!!!!!!!!!!Caught signal %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",s );
		//pProcessManager->Stop();
		//delete pLogger; pLogger = nullptr;
		//exit( 1 );
	}

	α OSApp::KillInstance( uint processId )noexcept->bool
	{
		var result = ::kill( processId, 14 );
		if( result )
			ERR( "kill failed with '{}'.", result );
		else
			INFO( "kill sent to:  '{}'.", processId );
		return result==0;
	}
	up<flat_map<string,string>> _pArgs;
	α OSApp::Args()noexcept->flat_map<string,string>
	{
		if( !_pArgs )
		{
			_pArgs = make_unique<flat_map<string,string>>();
			std::ifstream file( "/proc/self/cmdline" );
			auto p = _pArgs->try_emplace( {} );
			for( string current; std::getline<char>(file, current, '\0'); )
			{
				if( current.starts_with('-') )
					p = _pArgs->try_emplace( current );
				else
					p.first->second = current;
			}
		}
		return *_pArgs;
	}
	α OSApp::CompanyRootDir()noexcept->fs::path{ return path{ "."+OSApp::CompanyName() }; };

	α OSApp::AddSignals()noexcept(false)->void/*noexcept(false) for windows*/
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
		signal( SIGUSR1, OSApp::ExitHandler );

/*		struct sigaction sa;
	   sa.sa_flags = SA_RESTART | SA_SIGINFO;
		sigemptyset( &sa.sa_mask );
		sa.sa_sigaction = IO::LinuxDriveWorker::AioSigHandler;
		THROW_IF( ::sigaction(SIGUSR1, &sa, nullptr)==-1,  "sigaction(SIGUSR1) returned {}", errno );
*/
		//sigaction( SIGSTOP, &sigIntHandler, nullptr );
		//sigaction( SIGKILL, &sigIntHandler, nullptr );
		//sigaction( SIGTERM, &sigIntHandler, nullptr );
	}

	α OSApp::SetConsoleTitle( sv title )noexcept->void
	{
		std::cout << "\033]0;" << title << "\007";
	}
}
#undef var