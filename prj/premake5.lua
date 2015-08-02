solution "VideoPoker"
	configurations {"Debug", "Release"}

external "bgfx"
	kind "StaticLib"
	location "../bgfx/.build/projects/vs2015"

external "bx"
	kind "StaticLib"
	location "../bx/.build/projects/vs2015"

external "example-common"
	kind "StaticLib"
	location "../bgfx/.build/projects/vs2015"

project "VideoPoker"
	kind "ConsoleApp"

	flags 
	{
		"StaticRuntime",
	}

	defines 
	{
		"_HAS_EXCEPTIONS=0",
		"_HAS_ITERATOR_DEBUGGING=0",
		"_SCL_SECURE=0",
		"_SECURE_SCL=0",
		"_SCL_SECURE_NO_WARNINGS",
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_SECURE_NO_DEPRECATE", 
	}
	files 
	{
		"../src/**"
	}

	includedirs
	{
		"../bgfx/include/",
		"../bgfx/3rdparty/",
		"../bgfx/examples/common/",
		"../bx/include/",
	}

	libdirs
	{
		"../bgfx/.build/win32_vs2015/bin"
	}

	links 
	{
		"bgfx",
		"example-common"
	}

	links 
	{
		"gdi32",
		"psapi",
	} 

	configuration "Debug"
		defines     "_DEBUG"
		flags       { "Symbols" }
		debugdir    ( "../res" )

	configuration "Release"
		defines     "NDEBUG"
		flags       { "OptimizeSize" }
