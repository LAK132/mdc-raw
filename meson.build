project(
	'mdc-raw',
	['c', 'cpp'],
	default_options: {
		'c_std': 'c11',
		'cpp_std': ['c++26', 'c++23', 'c++latest'],
		'b_vscrt': 'static_from_buildtype',
		'warning_level': '3',
		'werror': false,
	},
)

cxx = meson.get_compiler('cpp')

if cxx.get_id() == 'msvc'
	add_project_arguments(
		'-DUNICODE',
		'-DWIN32_LEAN_AND_MEAN',
		'-DNOMINMAX',
		'-wd4315',
		'-wd4366',
		'-wd4250',
		'-Wv:18',
		language: ['c', 'cpp'])

	add_project_arguments(
		'-Zc:__cplusplus',
		'-Zc:rvalueCast',
		'-Zc:wchar_t',
		'-Zc:ternary',
		'-Zc:preprocessor',
		language: ['cpp'])
else
	add_project_arguments(
		'-Wno-unknown-warning-option',
		'-Wno-gnu-zero-variadic-macro-arguments',
		'-Wno-language-extension-token',
		'-Wno-dangling-else',
		'-Wno-microsoft-cast',
		'-Wno-pragma-pack',
		'-Wno-deprecated-declarations',
		'-Wno-unused-private-field',
		'-Wno-unused-variable',
		'-Wno-init-list-lifetime',
		'-Wno-invalid-partial-specialization',
		'-Wno-missing-field-initializers',
		'-Wno-maybe-uninitialized',
		'-Wno-undefined-inline',
		'-Wfatal-errors',
		'-Wno-psabi',
		# '-mavx',
		language: ['c', 'cpp', 'objcpp'])

	if cxx.get_id() == 'gcc'
		if cxx.has_link_argument('-Wl,-lstdc++_libbacktrace')
			# GCC <14
			add_project_link_arguments(
				'-Wl,-lstdc++_libbacktrace',
				language: ['cpp'])
		elif cxx.has_link_argument('-Wl,-lstdc++exp')
			# GCC >=14
			add_project_link_arguments(
				'-Wl,-lstdc++exp',
				language: ['cpp'])
		else
			warning('Unable to determine backtrace library link argument')
		endif
	endif
endif

add_project_arguments(
	'-D_CRT_SECURE_NO_WARNINGS',
	language: ['c', 'cpp'])

if get_option('buildtype') != 'debug'
	add_project_arguments(
		'-DNDEBUG',
		language: ['c', 'cpp'])
endif

binex_subprj = subproject('binex')
binex_core_dep = binex_subprj.get_variable('binex_core_dep')

if host_machine.system() == 'windows'
	git_header = custom_target(
		'generate_git_file',
		output: 'git.hpp',
		command: ['generate_git_file.bat', '@OUTPUT@'] ,
		build_by_default: true,
	)
else
	git_header = custom_target(
		'generate_git_file',
		output: 'git.hpp',
		command: ['./generate_git_file.sh', '@OUTPUT@'] ,
		build_by_default: true,
	)
endif

install_directory = host_machine.cpu_family()

subdir('src')

# if host_machine.system() == 'windows'
#   libgphoto2_dll = configure_file(
#     copy: true,
#     input: libgphoto2_dll,
#     output: 'libgphoto2.dll',
#     install: true,
#     install_dir: install_directory,
#   )
# endif

executable(
	'mdc',
	mdc + [git_header],
	install: true,
	install_dir: install_directory,
	include_directories: include_directories([
		'include',
	]),
	dependencies: [
		# lak_dep,
		binex_core_dep,
		dependency('stb_image'),
		dependency('stb_image_write'),
	],
	link_args: [
		# '/FORCE:MULTIPLE',
		# '/NODEFAULTLIB:library',
		# lak_libs,
	],
)
