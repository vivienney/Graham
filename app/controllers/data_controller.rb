class DataController < ApplicationController

require 'csv'
  def sppe
    #read file

    @divisor = 8718720000 # Latest divisor 8718.72from 3/31/2016 - retrieved may 2016

    #makret cap and earnings for entire stock list, in Billions
    @total_market_cap = 0
    @total_earnings = 0
    @index = 0 #get from google?
    @comp_data = []
    #lines = lines.first(10) #test first

    @exclude_list = []

    output_file = File.open(Rails.root.join('log','sp_evaluation_out.txt'),"w")
    CSV.foreach(Rails.root.join('utility_scripts','sp500_list_2016-05-01.txt')) do |row|
      ticker = row[0].strip
      ticker = "BRK.A" if ticker == "BRK-B"
      ticker = "BF.B" if ticker == "BF-B"

      #next if ticker.to_s == "GGP" # SCE site is missing the filings!!!
      if ticker.to_s == "CSRA" # NO annual data yet
        output_file.puts"Not enough data yet to include #{ticker}"
        next
      end

      # Acquired - no longer listed:
      next if ticker.to_s == "PCP" # Acquired - no longer listed
      next if ticker.to_s == "BRCM" # Acquired - no longer listed

      stock = Stock.get_from_ticker(ticker)

      if (stock.nil? && ticker)
        ticker = ticker.gsub("-","")
        stock = Stock.joins(:share_classes).where( share_classes: {ticker: ticker}).first
      end

      if stock.nil?
        output_file.puts"Could not get stock object for ticker#{ticker}"
        next
      end

      if stock.has_multiple_share_classes?
        next if @exclude_list.include?(stock.ticker)
        @exclude_list << stock.ticker
      end

      stock.update_price if(stock.updated_at < 1.days.ago)

      price = stock.price

      ep = stock.ttm_earnings_record

      #first filing for HPE is annual for 2015
      ep = stock.annual_eps_newest_first.first if stock.ticker == "HPE"

      if ep.nil?
        output_file.puts"Could not get latest earnings record for #{ticker}"
        next
      end

      spd = Spdata.new(stock.ticker, price, stock.ttm_eps,
                       ep.net_income, stock.shares_float)

      @total_market_cap += spd.market_cap
      @total_earnings += spd.ttm_earnings.to_i

      @comp_data << spd
    end
    output_file.close
    @comp_data.sort_by! { |s| -s.market_cap }
  end


end
