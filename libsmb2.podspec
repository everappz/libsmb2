Pod::Spec.new do |s|
  s.name             = 'libsmb2'
  s.version          = '6.1.0'
  s.summary          = 'SMB2/SMB3 userspace client library'
  s.description      = <<-DESC
    Libsmb2 is a userspace client library for accessing SMB2/SMB3 shares on a network.
    It is high performance and fully async. It supports both zero-copy for SMB READ/WRITE
    commands as well as compounded commands.
  DESC

  s.homepage         = 'https://github.com/sahlberg/libsmb2'
  s.license          = { :type => 'LGPL-2.1', :file => 'LICENCE-LGPL-2.1.txt' }
  s.author           = { 'Ronnie Sahlberg' => 'ronniesahlberg@gmail.com' }
  s.source           = { :git => 'https://github.com/sahlberg/libsmb2.git', :tag => s.version.to_s }

  s.ios.deployment_target     = '13.0'
  s.osx.deployment_target     = '10.15'
  s.tvos.deployment_target    = '14.0'

  s.source_files = 'lib/*.{c,h}',
                   'include/**/*.h'

  s.public_header_files = 'include/smb2/*.h'

  s.private_header_files = 'include/libsmb2-private.h',
                           'include/asprintf.h',
                           'include/portable-endian.h',
                           'include/slist.h',
                           'include/apple/config.h',
                           'lib/*.h'

  s.exclude_files = 'lib/ps2/**/*',
                    'include/module.modulemap'

  s.header_dir = 'smb2'
  s.module_name = 'SMB2'
  s.module_map = 'support/cocoapods/libsmb2.modulemap'

  s.prefix_header_file = 'include/apple/config.h'

  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => [
      '"${PODS_TARGET_SRCROOT}/include"',
      '"${PODS_TARGET_SRCROOT}/include/apple"',
      '"${PODS_TARGET_SRCROOT}/include/smb2"',
      '"${PODS_TARGET_SRCROOT}/lib"',
    ].join(' '),
    'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) HAVE_CONFIG_H=1 HAVE_STDINT_H=1 HAVE_TIME_H=1 _U_=__attribute__\(\(unused\)\)',
    'CLANG_ENABLE_MODULES' => 'NO',
  }

  s.user_target_xcconfig = {
    'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) HAVE_STDINT_H=1 HAVE_TIME_H=1',
  }

  s.frameworks = 'Security'
  s.library = 'resolv'

  s.requires_arc = false

  s.prepare_command = <<-CMD
    rm -f include/module.modulemap
    /usr/bin/sed -i '' 's/#ifdef HAVE_STDINT_H/#if 1 \\/* HAVE_STDINT_H *\\//' include/smb2/smb2.h
    /usr/bin/sed -i '' 's/#ifdef HAVE_TIME_H/#if 1 \\/* HAVE_TIME_H *\\//' include/smb2/smb2.h
  CMD
end
