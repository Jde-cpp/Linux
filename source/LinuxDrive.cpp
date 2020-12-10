#include "LinuxDrive.h"
#include <unistd.h>
#include "../../Framework/source/io/File.h"

#define var const auto

namespace Jde::IO
{
	std::shared_ptr<Jde::IO::IDrive> LoadNativeDrive()
	{
		return std::make_shared<Jde::IO::Drive::NativeDrive>();
	}

namespace Drive
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
		if( !fs::exists(dir) )
			THROW( IOException( "'{}' does not exist.", dir) );
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
		THROW_IF( result!=0, Exception("symlink creating '{}' referencing '{}' failed ({}){}.", newSymLink.string(), existingFile.string(), result, errno) );
		DBG( "Created symlink '{}' referencing '{}'."sv, newSymLink.string(), existingFile.string() );
	}
}}