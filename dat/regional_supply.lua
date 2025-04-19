--#-package:045A0001# -- package signature --
-- Any Lua files that depend on this file must have higher package numbers.

regional_supply = {}
regional_supply.add_to_demand = function(resourceID, demandAdded) end  -- Adds to the existing demand for the specified resource.
regional_supply.remove_from_demand = function(resourceID, demandRemoved) end  -- Subtracts from the existing demand of the specified resource.
regional_supply.add_to_supply = function(resourceID, supplyAdded) end  -- Adds to the existing supply of the specified resource.
regional_supply.remove_from_supply = function(resourceID, supplyRemoved) end  -- Subtracts from the existing supply of the specified resource.
regional_supply.get_resource_quantity = function(resourceID) return 0 end  -- Gets the current quantity of the specified resource.

-- EOF