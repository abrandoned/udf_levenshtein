require 'leven'

describe "Levenshtein" do
  specify{ Levenshtein.leven("hello", "hello").should equal(0) }
  specify{ Levenshtein.leven("", "").should equal(0) }
  specify{ Levenshtein.leven("hello", "jello").should equal(1) }
  specify{ Levenshtein.leven("hello", "jell").should equal(2) }
  specify{ Levenshtein.leven("lo", "jello").should equal(3) }
  specify{ Levenshtein.leven("jello", "lo").should equal(3) }
  specify{ Levenshtein.leven("", "jello").should equal("jello".length) }
  specify{ Levenshtein.leven("jello", "").should equal("jello".length) }
  specify{ Levenshtein.leven("hello"*2, "jello"*2).should equal(2) }
  specify{ Levenshtein.leven("hello"*2, "jelo"*2).should equal(4) }
  specify{ Levenshtein.leven("hello"*2, "jell"*2).should equal(4) }
  specify{ Levenshtein.leven("hello"*4, "jello"*4).should equal(4) }
  specify{ Levenshtein.leven("hello"*8, "jello"*8).should equal(8) }  
  specify{ Levenshtein.leven("hello"*16, "jello"*16).should equal(16) }
  specify{ Levenshtein.leven("hello"*32, "jello"*32).should equal(32) }
  specify{ Levenshtein.leven("hello"*64, "jello"*64).should equal(64) }
  specify{ Levenshtein.leven("hello"*128, "jello"*128).should equal(128) }
  specify{ Levenshtein.leven("hello"*256, "jello"*256).should equal(256) }
  specify{ Levenshtein.leven("hello"*512, "jello"*512).should equal(512) }  
          
  describe "threshold" do 
    specify{ Levenshtein.leven("hello"*100, "jello"*100, 10).should equal(500) }
    specify{ Levenshtein.leven("hello"*100, "jello"*100, 99).should equal(500) }
    specify{ Levenshtein.leven("hello"*100, "jello"*100, 100).should equal(100) }        
    specify{ Levenshtein.leven("hello"*100, "jello"*100, 1000).should equal(100) }    
  end

  describe "long strings" do 
    specify{ Levenshtein.leven("hello"*2000, "jello"*2000).should equal(2000) }
    specify{ Levenshtein.leven("hello"*5000, "jello"*5000).should equal(5000) }
    specify{ Levenshtein.leven("hello"*7500, "jello"*7500).should equal(7500) }
    specify{ Levenshtein.leven("hello"*9500, "jello"*9500).should equal(9500) }    
  end

  describe "threshold long strings" do 
    specify{ Levenshtein.leven("hello"*2000, "jello"*2000, 10).should equal(5*2000) }
    specify{ Levenshtein.leven("hello"*5000, "jello"*5000, 10).should equal(5*5000) }
    specify{ Levenshtein.leven("hello"*7500, "jello"*7500, 10).should equal(5*7500) }
    specify{ Levenshtein.leven("hello"*9500, "jello"*9500, 10).should equal(5*9500) }    
  end

  describe "special chars" do 
    specify{ Levenshtein.leven("*&^%$", "").should equal(5) }
    specify{ Levenshtein.leven("", ",./>?").should equal(5) }
    specify{ Levenshtein.leven('*&^%$+_=-)(*&^%$#@!~123456789', '*&^%$+_=-)(*&^%$#@!~').should equal(9) }
    specify{ Levenshtein.leven('*&^%$+_=-)(*&^%$#@!~', "").should equal(20) }
  end
end
