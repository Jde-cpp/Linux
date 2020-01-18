//https://www.cise.ufl.edu/~seeger/dist/tzdump.c
#include <arpa/inet.h>
#include "../../Framework/source/Cache.h"
#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/log/Logging.h"
#include <fstream>

#define var const auto
namespace Jde
{
	// auto diff = Timezone::EasternTimezoneDifference( Clock::now() ); ASSERT( diff==-5h );
	// diff = Timezone::EasternTimezoneDifference( DateTime{2020,3,8} ); ASSERT( diff==-5h );
	// diff = Timezone::EasternTimezoneDifference( DateTime{2020,3,9} ); ASSERT( diff==-4h );
	// diff = Timezone::EasternTimezoneDifference( DateTime{2020,11,1} ); ASSERT( diff==-4h );
	// diff = Timezone::EasternTimezoneDifference( DateTime{2020,11,2} ); ASSERT( diff==-5h );

	constexpr string_view Magic = "TZif"sv;
	struct tzhead
	{
		char	tzh_magic[4];		/* TZ_MAGIC */
		char	tzh_version[1];		/* '\0' or '2' or '3' as of 2013 */
		char	tzh_reserved[15];	/* reserved; must be zero */
		char	tzh_ttisutcnt[4];	/* coded number of trans. time flags */
		char	tzh_ttisstdcnt[4];	/* coded number of trans. time flags */
		char	tzh_leapcnt[4];		/* coded number of leap seconds */
		uint32_t tzh_timecnt;		/* coded number of transition times */
		uint32_t tzh_typecnt;		/* coded number of local time types */
		char	tzh_charcnt[4];		/* coded number of abbr. chars */
	};
	typedef map<TimePoint,Duration, std::greater<TimePoint>> CacheType;
	sp<CacheType> LoadGmtOffset( string_view name )noexcept(false)
	{
		const fs::path path{ fs::path{"/usr/share/zoneinfo"}/fs::path{name} };
		if( !fs::exists(path) )
			THROW( IOException("Could not open '{}'", path.string()) );

		std::ifstream is{ path.string() };
		tzhead head;
		is.read(  (char*)&head, sizeof(tzhead) );
		if( string(head.tzh_magic,4)!=string(Magic) )
			THROW( IOException("Magic not equal '{}'", string(head.tzh_magic,4)) );
		if( is.bad() )
			THROW( IOException("after header is.bad()") );

		//unordered_map<string,map<TimePoint,Duration>> timeZones;
		var count = ntohl( head.tzh_timecnt );
		std::vector<TimePoint> transistionTimes; transistionTimes.reserve( count );
		for( uint32_t i=0;i<count; ++i )
		{
			int32_t value;
			is.read( (char*)&value, sizeof(value) );
			transistionTimes.push_back( Clock::from_time_t(ntohl(value)) );
		}
		std::map<TimePoint, uint8> timeTypes;
		for( uint32_t i=0;i<count; ++i )
			timeTypes.try_emplace( timeTypes.end(), transistionTimes[i], is.get() );

		var typeCount = ntohl( head.tzh_typecnt );
		vector<int32_t> typeOffsets; typeOffsets.reserve( typeCount );
		for( uint32_t i = 0; i < typeCount; ++i )
		{
			int32_t value;
			is.read( (char*)&value, sizeof(value) );
			int32_t value2 = ntohl( value );
			typeOffsets.push_back( value2 );
			/*unsigned char isDst =*/ is.get();
			/*unsigned char abbreviationIndex = */ is.get();
		}
		if( is.bad() )
			THROW( IOException("is.bad()") );

		auto pResults = make_shared<CacheType>();
		for( var [time,type] : timeTypes )
		{
			if( type>=typeOffsets.size() )
				THROW( IOException("type>=typeOffsets.size() {}>={}", type, typeOffsets.size()) );
			pResults->emplace( time, std::chrono::seconds(typeOffsets[type]) );
		}
		return pResults;
	}

	Duration Timezone::EasternTimezoneDifference( TimePoint utc )noexcept
	{
		return TryGetGmtOffset( "EST5EDT", utc );
	}

	Duration Timezone::GetGmtOffset( string_view name, TimePoint utc )noexcept(false)
	{
		var key = fmt::format( "GetGmtOffset-{}", name );
		auto pInfo = Cache::Get<CacheType>( key );
		if( !pInfo || !pInfo->size() )
			Cache::Set<CacheType>( key, pInfo = LoadGmtOffset( name ) );
		var pStartDuration = pInfo->lower_bound( utc );
		if( pStartDuration==pInfo->end() )
			THROW( Exception( "No info for '{}'", ToIsoString(utc) ) );
		TimePoint date{ pStartDuration->first };
		const Duration value{ pStartDuration->second };
		return value;
	}
}