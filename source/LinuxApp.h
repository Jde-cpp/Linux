

namespace Jde
{
	class LinuxApp : IApp
	{
		void SetConsoleTitle( string_view title )noexcept override{ std::cout << "\033]0;" << title << "\007"; }
	}
}
