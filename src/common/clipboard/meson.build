if platform_clipboard
	clipboard_impl_factories = []
	if host_platform == 'windows'
		powder_files += files('Windows.cpp')
		clipboard_impl_factories += [
			[ 'PlatformSubsystem::Windows', 'WindowsClipboardFactory' ],
		]
	elif host_platform == 'darwin'
		if get_option('build_powder')
			add_languages('objcpp', native: false)
			powder_deps += [
				dependency('Cocoa'),
			]
		endif
		powder_files += files([
			'Cocoa.mm',
		])
		clipboard_impl_factories += [
			[ 'PlatformSubsystem::Cocoa', 'CocoaClipboardFactory' ],
		]
	elif host_platform == 'android'
		# TODO
	elif host_platform == 'emscripten'
		# TODO
	else
		powder_files += files([
			'External.cpp',
		])
		clipboard_impl_factories += [
			[ 'PlatformSubsystem::X11', 'ExternalClipboardFactory' ],
			[ 'PlatformSubsystem::Wayland', 'ExternalClipboardFactory' ],
		]
	endif
	powder_files += files('Dynamic.cpp')
else
	powder_files += files('Local.cpp')
endif
render_files += files('Null.cpp')
font_files += files('Null.cpp')
