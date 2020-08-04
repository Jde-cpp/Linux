#pragma once
#include "Exports.h"
#include "../../Framework/source/io/DiskWatcher.h"
//#include "io/drive/DriveApi.h"

/*JDE_LINUX_EXPORT*/
//extern "C" Jde::IO::IDrive* LoadDrive();

namespace Jde::IO::Drive
{
	struct NativeDrive final: public IDrive
	{
		//void Recursive2( const fs::path& dir )noexcept(false);
		map<string,IDirEntryPtr> Recursive( const fs::path& dir )noexcept(false) override;
		IDirEntryPtr Get( const fs::path& path )noexcept(false) override;
		IDirEntryPtr Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )noexcept(false) override;
		IDirEntryPtr CreateFolder( const fs::path& path, const IDirEntry& dirEntry )noexcept(false) override;
		void Trash( const fs::path& path )noexcept override;
		VectorPtr<char> Load( const IDirEntry& dirEntry )noexcept(false) override;
		void Remove( const fs::path& )noexcept(false) override;
		void TrashDisposal( TimePoint /*latestDate*/ )noexcept(false)override{ THROW(Exception("Not Implemented")); };
	};
}