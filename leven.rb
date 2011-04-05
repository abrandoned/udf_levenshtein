require 'rubygems'
require 'ffi'

module Levenshtein
  extend FFI::Library
  ffi_lib "./libudf_levenshtein.so"
  attach_function :levenshtein_extern, [:pointer, :pointer, :long_long], :long_long
  
  def self.leven(str1, str2, max=9999)
    self.levenshtein_extern(str1, str2, max)
  end
end
