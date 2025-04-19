/*
 * This file is part of SC4RegionalSupplyDemand, a DLL Plugin for SimCity 4
 * that implements a basic regional supply/demand system.
 *
 * Copyright (C) 2025 Nicholas Hayes
 *
 * SC4RegionalSupplyDemand is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * SC4RegionalSupplyDemand is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SC4RegionalSupplyDemand.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <cstdint>

class IRegionalSupplyManager
{
public:
	virtual void AddToDemand(uint32_t resourceID, uint32_t amount) = 0;
	virtual void RemoveFromDemand(uint32_t resourceID, uint32_t amount) = 0;
	virtual void AddToSupply(uint32_t resourceID, uint32_t amount) = 0;
	virtual void RemoveFromSupply(uint32_t resourceID, uint32_t amount) = 0;

	virtual int64_t GetResourceQuantity(uint32_t resourceID) const = 0;
};