Pod::Spec.new do |s|
  s.name     = 'libsmb2'
  s.version  = '3.0.0.2'
  s.license  =  { :type => 'GNU Lesser General Public License 2.1', :file => 'LICENCE-LGPL-2.1.txt' }
  s.summary  = 'Libsmb2 is a userspace client library for accessing SMB2/SMB3 shares on a network. It is high performance and fully async. It supports both zero-copy for SMB READ/WRITE commands as well as compounded commands.'
  s.homepage = 'https://github.com/leshkoapps/libsmb2.git'
  s.author   = 'Ronnie Sahlberg'
  s.source   = { :git => 'https://github.com/leshkoapps/libsmb2.git', :tag => "#{s.version}" }
  s.platform = :ios, '9.0'
  s.source_files = 
  'include/smb2/*.{h}',
  'include/msvc/poll.h',
  'include/portable-endian.h',
  'include/libsmb2-private.h',
  'include/asprintf.h',
  'include/slist.h',
  'include/esp/config.h',
  'lib/*.{h,c}'
  s.requires_arc = true
  s.xcconfig = { 'HEADER_SEARCH_PATHS' => '"$(PODS_ROOT)/libsmb2/include/..","$(PODS_ROOT)/libsmb2/include/smb2/.."' }
end
