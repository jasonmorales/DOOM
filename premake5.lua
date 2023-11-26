glew_version = '2.1.0'

require('vstudio')
premake.api.register {
	name = 'workspacefiles',
	scope = 'workspace',
	kind = 'list:keyed:list:string',
}

premake.override(premake.vstudio.sln2005, 'projects', function(base, wks)
	for _, folder in ipairs(wks.workspacefiles) do
		for name, pathlist in pairs(folder) do
			premake.push('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "'  .. name .. '", "' .. name .. '", "{' .. os.uuid(name .. wks.name) .. '}"')
			premake.push('ProjectSection(SolutionItems) = preProject')
			for _, path in ipairs(pathlist) do
				for _, file in ipairs(os.matchfiles(path)) do
					premake.w(file .. ' = ' .. file)
				end
			end
			premake.pop('EndProjectSecion')
			premake.pop('EndProject')
		end
	end
	base(wks)
end)

workspace 'Doom'
	editorintegration "On"
	configurations { 'Debug', 'Release' }
	platforms { 'x64' }
	system 'windows'
	architecture 'x86_64'
	startproject 'doom'

	workspacefiles {
		['docs'] = {
			'docs/**',
		},
		['project'] = {
			'premake5.lua',
            '.editorconfig',
            '.gitignore',
		}
	}

	project 'doom'
		--kind 'WindowedApp'
		kind 'ConsoleApp'
		language 'C++'
		cppdialect 'C++latest'
		entrypoint 'WinMainCRTStartup'
		systemversion '10.0'
		characterset 'unicode'
		warnings 'extra'
		rtti 'Off'
		dpiawareness 'HighPerMonitor'
		usestandardpreprocessor 'On'
		flags {}

		location 'project'
		objdir 'intermediate'
		targetdir 'bin'
		debugdir '.'

		filter 'Debug'
			defines { 'DEBUG', '_DEBUG' }
			symbols 'on'
			optimize 'off'
			targetsuffix '_d'
			debugcommand ('bin/doom_d.exe')

		filter 'Release'
			defines { 'NDEBUG' }
			optimize 'full'
			debugcommand ('bin/doom.exe')
			
		filter {}

		files {
            'source/doom/**',
		}

		includedirs {
			'lib/glew-' .. glew_version .. '/include',
		}

		os.mkdir('bin')
		if os.host() == 'windows' then
			os.copyfile('lib/glew-' .. glew_version .. '/bin/Release/x64/glew32.dll', 'bin/glew32.dll')
		end

		libdirs {
			'lib/glew-' .. glew_version .. '/lib/Release/x64',
		}
	
		links {
			'opengl32',
			'glu32',
			'glew32',
			'winmm',
			--'dinput8',
			--'dxguid',
			'ws2_32',
		}

	project 'ipx_driver'
		kind 'None'
		characterset 'unicode'

		location 'project'
		
		files 'source/ipx_driver/**'

	project 'serial_driver'
		kind 'None'
		characterset 'unicode'

		location 'project'
		
		files 'source/serial_driver/**'

	project 'sound_server'
		kind 'None'
		characterset 'unicode'

		location 'project'
		
		files 'source/sound_server/**'
