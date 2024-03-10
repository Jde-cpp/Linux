#pragma once
#include <signal.h>
#include <aio.h>
#include "Exports.h"
#include "../../Framework/source/io/DiskWatcher.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/io/File.h>

using namespace Jde::Coroutine;
namespace Jde::IO
{
	struct LinuxChunk final : IFileChunkArg
	{
		LinuxChunk( FileIOArg& pIOArg, uint index )noexcept;
		//uint StartIndex()const noexcept override;
		//void SetStartIndex( uint i )noexcept override;
		//uint Bytes()const noexcept override{ return _linuxArg.aio_nbytes; } virtual void SetBytes( uint x )noexcept override{ _linuxArg.aio_nbytes=x; }
		//void SetEndIndex( uint i )noexcept override;
		//void SetFileIOArg( FileIOArg* p )noexcept override{ _fileIOArgPtr=p; }
		//HFile Handle()noexcept override{ return _linuxArg.aio_fildes; };
		//void Process( int handle )noexcept override;
		//optional<bool> Complete()noexcept;
	private:
		//aiocb _linuxArg;
	};

/*	struct LinuxDriveWorker final : DriveWorker
	{
		//static void IOHandler( int s )noexcept;
	//	static void AioSigHandler( int sig, siginfo_t* pInfo, void* pContext )noexcept;
	};*/
}
namespace Jde::IO::Drive
{
	struct NativeDrive final: public IDrive
	{
		//void Recursive2( path dir )noexcept(false);
		flat_map<string,IDirEntryPtr> Recursive( const fs::path& dir, SRCE )noexcept(false) override;
		IDirEntryPtr Get( const fs::path& path )noexcept(false) override;
		IDirEntryPtr Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )noexcept(false) override;
		IDirEntryPtr CreateFolder( const fs::path& path, const IDirEntry& dirEntry )noexcept(false) override;
		void Trash( const fs::path& path )noexcept override;
		VectorPtr<char> Load( const IDirEntry& dirEntry )noexcept(false) override;
		void Remove( const fs::path& )noexcept(false) override;
		void TrashDisposal( TimePoint /*latestDate*/ )noexcept(false)override{ THROW("Not Implemented"); };
		void Restore( sv name )noexcept(false)override{ THROW("Not Implemented"); };
		void SoftLink( const fs::path& existingFile, const fs::path& newSymLink )noexcept(false) override;
	};
}