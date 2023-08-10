workspace "BreakOut"
  system "Windows"
  architecture "x86_64"
  defines { "GLOBAL" }
  configurations { "Debug", "Release" }

project "BreakOut"
  kind "ConsoleApp"
  language "C++"
  targetdir "bin/%{cfg.buildcfg}"
  
  files { "src/**.h", "src/**.cpp", "src/**.c" }
  removefiles { }     -- Excluding the useless files. 

  defines { "PROJECT" }

  includedirs { "OpenGL/Include" }

  libdirs { "OpenGL/Libs" }
  links { "glfw3.lib", 
          "opengl32.lib", 
          "irrKlang.lib", 
          "freetyped.lib" }

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
    
  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"