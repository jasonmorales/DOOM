glew_version = '2.1.0'

require('vstudio')
premake.api.register {
	name = 'workspacefiles',
	scope = 'workspace',
	kind = 'list:keyed:list:string',
}

premake.override(premake.vstudio.sln2005, 'projects', function(base, wks)
	for _, folder in ipairs(wks.workspacefiles) do
		for name, files in pairs(folder) do
			premake.push('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "'  .. name .. '", "' .. name .. '", "{' .. os.uuid(name .. wks.name) .. '}"')
			premake.push('ProjectSection(SolutionItems) = preProject')
			for _, file in ipairs(files) do
				premake.w(file .. ' = ' .. file)
			end
			premake.pop('EndProjectSecion')
			premake.pop('EndProject')
		end
	end
	base(wks)
end)

function standard_settings(project_name)
	language 'C++'
    cppdialect 'C++latest'
    cdialect 'C17'
    systemversion '10.0'
	characterset 'unicode'
    warnings 'extra'
	rtti 'Off'
	dpiawareness 'HighPerMonitor'
	flags {}

	location 'project'
	objdir 'intermediate'
	targetdir 'bin'

	filter 'action:vs*'
		buildoptions { '/Zc:preprocessor' }
	filter {}

	defines { 'PROJECT_NAME=' .. project_name }

	filter 'system:windows'
		excludes { '**_mac.*' }
		
	filter 'system:macosx'
		excludes { '**_win32.*', '**_win64.*' }

	filter 'Debug'
		defines { 'DEBUG', '_DEBUG' }
		symbols 'on'
		optimize 'off'
		targetsuffix '_d'
		debugcommand ('bin/' .. project_name .. '_d.exe')

	filter 'Release'
		defines { 'NDEBUG' }
		optimize 'full'
		debugcommand ('bin/' .. project_name .. '.exe')
		
	filter {}
end

workspace 'Doom'
	configurations { 'Debug', 'Release' }
	if os.target() == "windows" then
		platforms { 'x64' }
		system 'windows'
		architecture 'x86_64'
	elseif os.target() == 'linux' then
		platforms { 'ARM' }
		system 'linux'
		architecture 'ARM'
	end
	startproject 'doom'

	workspacefiles {
		['project'] = {
			'premake5.lua',
			'tasks.txt',
            '.editorconfig',
            '.gitignore',
		}
	}

	project 'doom'
		standard_settings('doom')
		--kind 'WindowedApp'
		kind 'ConsoleApp'
		entrypoint 'WinMainCRTStartup'

		files {
			'ipx/**.c',
			'sersrc/**.c',
			'sndserv/**.c',
			'linuxdoom-1.10/**.c',
            'linuxdoom-1.10/**.cpp',
			'linuxdoom-1.10/**.h',
			'**.natvis',
		}

        filter 'files:sersrc/** or files:ipx/** or files:sndserv/**'
            flags { 'ExcludeFromBuild' }

		includedirs {
			'lib/glew-' .. glew_version .. '/include',
		}

		filter 'system:linux'
			includedirs { '/usr/include' }
		filter {}
		
		os.mkdir('bin')
		if os.host() == 'windows' then
			os.copyfile('lib/glew-' .. glew_version .. '/bin/Release/x64/glew32.dll', 'bin/glew32.dll')
		end

		filter 'system:windows'
			libdirs {
				'lib/glew-' .. glew_version .. '/lib/Release/x64',
			}
		
			links {
				'opengl32',
				'glu32',
				--'glew32',
				'winmm',
				'dinput8',
				'dxguid',
				'ws2_32',
			}

		filter 'system:linux'
			libdirs {
				'/opt/vc/lib',
			}
			links {
				'EGL',
				'GLESv2',
			}
		filter {}
