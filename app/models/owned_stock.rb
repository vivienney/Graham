# == Schema Information
# Schema version: 20100304214922
#
# Table name: owned_stocks
#
#  id           :integer         not null, primary key
#  portfolio_id :integer
#  stock_id     :integer
#  shares       :integer
#  created_at   :datetime
#  updated_at   :datetime
#

class OwnedStock < ActiveRecord::Base
  belongs_to :stock
  belongs_to :portfolio
  delegate :name, :ticker, :to => :stock









end
