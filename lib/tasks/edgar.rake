desc "get data from edgar"
task :edgar => :environment do

  require 'edgar/edgar'

  stock = Stock.find_by_ticker("IBM")
  ed = Edgar.new(stock)
  puts ed.stock

end
