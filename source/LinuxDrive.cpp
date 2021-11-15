#include "LinuxDrive.h"
#include <unistd.h>
#include <jde/io/File.h>

#define var const auto
//Test,
// ignore SigUsr1 on applicable threads
namespace Jde::IO
{
	Jde::IO::Drive::NativeDrive _native;
	Jde::IO::IDrive& Native()noexcept{ return _native; }

	void FileIOArg::Open()noexcept(false)
	{
		Handle = ::open( Path.string().c_str(), O_NONBLOCK | (IsRead ? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC) );
		if( Handle==-1 )
		{
			Handle = 0;
			THROW_IFX( IsRead || errno!=13, IOException(move(Path), errno, "open") );
			THROW_IFX( ::remove(Path.string().c_str())==-1, IOException(move(Path), errno, "remove") );
			Open();
			return;
		}
		if( IsRead )
		{
			struct stat st;
			THROW_IFX( ::fstat( Handle, &st )==-1, IOException(move(Path), errno, "fstat") );
			std::visit( [size=st.st_size](auto&& b){b->resize(size);}, Buffer );
		}
	}

/*	α FileIOArg::CreateChunk( uint i )noexcept->up<IFileChunkArg>
	{
		return make_unique<LinuxChunk>( *this, i );
	}
	α FileIOArg::OSSend()noexcept->void
	{
		CoroutinePool::Resume( move(h) );
	}
*/
	void FileIOArg::Send( coroutine_handle<Task2::promise_type>&& h )noexcept
	{
		CoHandle = h;
		CoroutinePool::Resume( move(CoHandle) );
	}

	α DriveAwaitable::await_resume()noexcept->TaskResult
	{
		base::AwaitResume();
		if( ExceptionPtr )
			return TaskResult{ ExceptionPtr };
		try
		{
			var size = _arg.Size();
			auto pData = std::visit( [](auto&& x){return x->data();}, _arg.Buffer );
			auto pEnd = pData+size;
			var chunkSize = DriveWorker::ChunkSize();
			var count = size/chunkSize+1;
			for( uint32 i=0; i<count; ++i )
			{
				auto pStart = pData+i*chunkSize;
				auto chunkCount = std::min<ptrdiff_t>( chunkSize, pEnd-pStart );
				if( _arg.IsRead )
				{
					THROW_IFX( ::read(_arg.Handle, pStart, chunkCount)==-1, IOException(_arg.Path, (uint)errno, "read()") );
				}
				else
					THROW_IFX( ::write(_arg.Handle, pStart, chunkCount)==-1, IOException(_arg.Path, (uint)errno, "write()") );
			}
			::close( _arg.Handle );
			sp<void> pVoid = std::visit( [](auto&& x){return (sp<void>)x;}, _arg.Buffer );
			return TaskResult{ pVoid };
		}
		catch( IException& e )
		{
			return TaskResult{ e.Clone() };
		}
	}


/*
	void DriveAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		base::await_suspend( h );
		CoroutinePool::Resume( move(h) );
	}
*/
/*	HFile FileIOArg::SubsequentHandle()noexcept
	{
		auto h = Handle;
		if( h )
			Handle = 0;
		else
		{
			h = ::open( Path.string().c_str(), O_ASYNC | O_NONBLOCK | (IsRead ? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC) );
			DBG( "[{}]{}"sv, h, Path.string().c_str() );
		}
		return h;
	}
	*/
/*	up<IFileChunkArg> FileIOArg::CreateChunk( uint startIndex, uint endIndex )noexcept
	{
		return make_unique<LinuxFileChunkArg>( *this, startIndex, endIndex );
	}
*/
/*	void LinuxDriveWorker::AioSigHandler( int sig, siginfo_t* pInfo, void* pContext )noexcept
	{
		auto pChunkArg = (IFileChunkArg*)pInfo->si_value.sival_ptr;
		auto& fileArg = pChunkArg->FileArg();
		DBG( "Processing {}"sv, pChunkArg->index );
		if( fileArg.HandleChunkComplete(pChunkArg) )
		{
		//	if( ::close(pChunkArg->Handle())==-1 )
			fileArg.CoHandle.promise().get_return_object().SetResult( IOException{fileArg.Path, (uint)errno, "close"} );
			CoroutinePool::Resume( move(fileArg.CoHandle) );
			//delete pFileArg;
		}
	}
*/
/*	LinuxFileChunkArg::LinuxFileChunkArg( FileIOArg& ioArg, uint start, uint length )noexcept:
		IFileChunkArg{ ioArg }
	{
		//_linuxArg.aio_fildes = ioArg.FileHandle;
		_linuxArg.aio_lio_opcode = ioArg.IsRead ? LIO_READ : LIO_WRITE;
		_linuxArg.aio_reqprio = 0;
		_linuxArg.aio_buf = ioArg.Data()+start;
		_linuxArg.aio_nbytes = length;
		_linuxArg.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		_linuxArg.aio_sigevent.sigev_signo = DriveWorker::Signal;
		_linuxArg.aio_sigevent.sigev_value.sival_ptr = this;
	}
	void LinuxFileChunkArg::Process()noexcept
	{
		DBG( "Sending chunk '{}' - handle='{}'"sv, index, handle );
		_linuxArg.aio_fildes = handle;
		var result = FileArg().IsRead ? ::aio_read( &_linuxArg ) : ::aio_write( &_linuxArg ); ERR_IF( result == -1, "aio({}) read={} returned false - {}", FileArg().Path.string(), FileArg().IsRead, errno );
		//var result = ::aio_write( &_linuxArg );
		ERR_IF( result == -1, "aio({}) read={} returned false - {}", FileArg().Path.string(), FileArg().IsRead, errno );
		//return result!=-1;
	}
*/
}
namespace Jde::IO::Drive
{
	tuple<TimePoint,TimePoint,uint> GetAttributes( fs::path path )
	{
		struct stat attrib;
		stat( path.string().c_str(), &attrib );
		var size = attrib.st_size;

		const TimePoint modifiedTime = Clock::from_time_t( attrib.st_mtim.tv_sec );//+std::chrono::nanoseconds( attrib.st_mtim.tv_nsec );TODO
		const TimePoint accessedTime = Clock::from_time_t( attrib.st_atim.tv_sec );//+std::chrono::nanoseconds( attrib.st_atim.tv_nsec );
		return make_tuple( modifiedTime,accessedTime, size );
	}
	struct DirEntry : IDirEntry
	{
		DirEntry( path path ):
			DirEntry( fs::directory_entry(path) )
		{
			//LoadNativeDrive();//TODO Remove
		}
		DirEntry( const fs::directory_entry& entry )
		{
			var path = Path = entry.path();
			var status = entry.status();
			if( fs::is_directory(status) )
				Flags = EFileFlags::Directory;

			var& [modified, accessed, size] = GetAttributes( entry );
			Size = size;
			ModifiedTime = modified;
			AccessedTime = accessed;
		}
	};
	IDirEntryPtr NativeDrive::Get( path path )noexcept(false)
	{
		sp<const IDirEntry> pEntry = make_shared<const DirEntry>( path );
		return pEntry;
	}
	map<string,IDirEntryPtr>  NativeDrive::Recursive( path dir )noexcept(false)
	{
		CHECK_PATH( dir );
		var dirString = dir.string();
		map<string,IDirEntryPtr> entries;

		std::function<void(const fs::directory_entry&)> fnctn;
		fnctn = [&dirString, &entries, &fnctn]( const fs::directory_entry& entry )
		{
			var status = entry.status();
			var relativeDir = entry.path().string().substr( dirString.size()+1 );

			sp<DirEntry> pEntry;
			if( fs::is_directory(status) || fs::is_regular_file(status) )
			{
				entries.emplace( relativeDir, make_shared<DirEntry>(entry.path()) );
				if( fs::is_directory(status) )
					FileUtilities::ForEachItem( entry.path(), fnctn );
			}
		};
		FileUtilities::ForEachItem( dir, fnctn );

		return entries;
	}
	timespec to_timespec( const TimePoint& time )
	{
		var sinceEpoch = time.time_since_epoch();
		var total = duration_cast<std::chrono::nanoseconds>( duration_cast<std::chrono::nanoseconds>( sinceEpoch )-duration_cast<std::chrono::seconds>( sinceEpoch ) ).count();

		return { Clock::to_time_t(time), total };
	}
	IDirEntryPtr NativeDrive::CreateFolder( path dir, const IDirEntry& dirEntry )noexcept(false)
	{
		fs::create_directory( dir );
		if( dirEntry.ModifiedTime.time_since_epoch()!=Duration::zero() )
		{
			//var [createTime, modifiedTime, size] = GetTimes( dirEntry );
			var modifiedTime = to_timespec( dirEntry.ModifiedTime );
			var accessedTime = dirEntry.AccessedTime.time_since_epoch()==Duration::zero() ? modifiedTime : to_timespec( dirEntry.AccessedTime );
			timespec values[] = {accessedTime, modifiedTime};
			if( !utimensat(AT_FDCWD, dir.string().c_str(), values, 0) )
				WARN( "utimensat returned {} on {}"sv, errno, dir.string() );
		}
		return make_shared<DirEntry>( dir );
	}
	IDirEntryPtr NativeDrive::Save( path path, const vector<char>& bytes, const IDirEntry& dirEntry )noexcept(false)
	{
		IO::FileUtilities::SaveBinary( path, bytes );
		if( dirEntry.ModifiedTime.time_since_epoch()!=Duration::zero() )
		{
			var modifiedTime = to_timespec( dirEntry.ModifiedTime );
			var accessedTime = dirEntry.AccessedTime.time_since_epoch()==Duration::zero() ? modifiedTime : to_timespec( dirEntry.AccessedTime );
			timespec values[] = {accessedTime, modifiedTime};
			if( utimensat(AT_FDCWD, path.string().c_str(), values, 0) )
				WARN( "utimensat returned {} on {}"sv, errno, path.string() );
		}
		return make_shared<DirEntry>( path );
	}

	//VectorPtr<char> NativeDrive::Load( path path )noexcept(false)
	VectorPtr<char> NativeDrive::Load( const IDirEntry& dirEntry )noexcept(false)//fs::filesystem_error, IOException
	{
		return IO::FileUtilities::LoadBinary( dirEntry.Path );
	}

	void NativeDrive::Remove( path path )noexcept(false)
	{
		DBG( "Removing '{}'."sv, path.string() );
		fs::remove( path );
	}
	void NativeDrive::Trash( path path )noexcept
	{
		DBG( "Trashing '{}'."sv, path.string() );

		var result = system( fmt::format("gio trash {}", path.string()).c_str() );
		DBG( "Trashing '{}' returned '{}'."sv, path.string(), result );
	}
	void NativeDrive::SoftLink( path existingFile, path newSymLink )noexcept(false)
	{
		var result = ::symlink( existingFile.string().c_str(), newSymLink.string().c_str() );
		THROW_IF( result!=0, "symlink creating '{}' referencing '{}' failed ({}){}.", newSymLink.string(), existingFile.string(), result, errno );
		DBG( "Created symlink '{}' referencing '{}'."sv, newSymLink.string(), existingFile.string() );
	}

/*	bool DriveWorker::Poll()noexcept
	{
		return base::Poll() || Args.size();//handle new queue item, from co_await Read() || currently handling item
	}
*/
	// void DriveWorker::HandleRequest( FileIOArg&& arg )noexcept
	// {
	// 	auto pArg = &Args.emplace_back( move(arg) );
	// 	var size = std::visit( [](auto&& x){return x->size();}, pArg->Buffer );
	// 	for( uint i=0; i<size; i+=ChunkSize() )
	// 	{
	// 		auto pChunkArg = make_unique<LinuxFileChunkArg>( pArg, i, std::min(DriveWorker::ChunkSize(), size) );
	// 		//Threading::AtomicGuard l{ pArg->Mutex };
	// 		if( pArg->Overlaps.size()<ThreadCount )
	// 			pArg->Send( move(pChunkArg) );
	// 		else
	// 			pArg->OverlapsOverflow.emplace_back( move(pChunkArg) );
	// 	}
	// }
/*	void FileIOArg::Send( up<IFileChunkArg> pChunkArg )noexcept
	{
		if( pChunkArg->Process() )
			Overlaps.emplace_back( move(pChunkArg) );
	}*/
}