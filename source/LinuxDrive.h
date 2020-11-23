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
		//void Recursive2( path dir )noexcept(false);
		map<string,IDirEntryPtr> Recursive( path dir )noexcept(false) override;
		IDirEntryPtr Get( path path )noexcept(false) override;
		IDirEntryPtr Save( path path, const vector<char>& bytes, const IDirEntry& dirEntry )noexcept(false) override;
		IDirEntryPtr CreateFolder( path path, const IDirEntry& dirEntry )noexcept(false) override;
		void Trash( path path )noexcept override;
		VectorPtr<char> Load( const IDirEntry& dirEntry )noexcept(false) override;
		void Remove( path )noexcept(false) override;
		void TrashDisposal( TimePoint /*latestDate*/ )noexcept(false)override{ THROW(Exception("Not Implemented")); };
		void Restore( sv name )noexcept(false)override{ THROW(Exception("Not Implemented")); };
	};
}