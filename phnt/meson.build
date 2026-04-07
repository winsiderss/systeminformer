project(
    'phnt',
    'c',
    meson_version: '>=0.63.0',
)

cc = meson.get_compiler('c')
ntdll_dep = cc.find_library('ntdll')

phnt_inc = include_directories('include')

phnt_srcs = files(
    'include/ntafd.h',
    'include/ntbcd.h',
    'include/ntdbg.h',
    'include/ntexapi.h',
    'include/ntgdi.h',
    'include/ntimage.h',
    'include/ntintsafe.h',
    'include/ntioapi.h',
    'include/ntkeapi.h',
    'include/ntldr.h',
    'include/ntlpcapi.h',
    'include/ntmisc.h',
    'include/ntmmapi.h',
    'include/ntnls.h',
    'include/ntobapi.h',
    'include/ntpebteb.h',
    'include/ntpfapi.h',
    'include/ntpnpapi.h',
    'include/ntpoapi.h',
    'include/ntpsapi.h',
    'include/ntregapi.h',
    'include/ntrtl.h',
    'include/ntsam.h',
    'include/ntseapi.h',
    'include/ntsmss.h',
    'include/ntstrsafe.h',
    'include/ntsxs.h',
    'include/nttmapi.h',
    'include/nttp.h',
    'include/ntuser.h',
    'include/ntwmi.h',
    'include/ntwow64.h',
    'include/ntxcapi.h',
    'include/ntzwapi.h',
    'include/phnt.h',
    'include/phnt_ntdef.h',
    'include/phnt_windows.h',
    'include/smbios.h',
    'include/subprocesstag.h',
    'include/usermgr.h',
    'include/winsta.h',
)

install_headers(
    phnt_srcs,
    subdir: 'phnt',
)

phnt_dep = declare_dependency(
    include_directories: phnt_inc,
    dependencies: ntdll_dep,
)

meson.override_dependency('phnt', phnt_dep)

pkgconfig = import('pkgconfig')
if pkgconfig.found()
    pkgconfig.generate(
        name: 'phnt',
        description: 'Native API header files for the System Informer project.',
        url: 'https://github.com/winsiderss/phnt',
        libraries: ntdll_dep,
        install_dir: get_option('datadir') / 'pkgconfig',
        subdirs: 'phnt',
    )
endif
