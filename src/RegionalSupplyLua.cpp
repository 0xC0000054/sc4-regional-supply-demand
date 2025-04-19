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

#include "RegionalSupplyLua.h"
#include "GlobalPointers.h"
#include "SCLuaUtil.h"
#include <limits>

namespace
{
	bool TryGetNumberAsUint32(cISCLua* pLua, int32_t index, uint32_t& value)
	{
		if (pLua->Type(index) != cIGZLua5Thread::LuaTypeNumber)
		{
			value = 0;
			return false;
		}

		double number = pLua->ToNumber(index);

		if (value < std::numeric_limits<uint32_t>::min()
			|| value > std::numeric_limits<uint32_t>::max())
		{
			value = 0;
			return false;
		}

		value = static_cast<uint32_t>(number);
		return true;
	}
}

int32_t RegionalSupplyLua::AddToDemand(lua_State* pState)
{
	cRZAutoRefCount<cISCLua> lua = SCLuaUtil::GetISCLuaFromFunctionState(pState);

	int32_t parameterCount = lua->GetTop();

	if (parameterCount == 2)
	{
		uint32_t amount = 0;
		uint32_t resourceID = 0;

		// Function parameters are popped off the stack in right-to-left order.

		if (TryGetNumberAsUint32(lua, -1, amount)
			&& TryGetNumberAsUint32(lua, -2, resourceID))
		{
			spRegionalSupplyManager->AddToDemand(resourceID, amount);
		}
	}

	return 0;
}

int32_t RegionalSupplyLua::RemoveFromDemand(lua_State* pState)
{
	cRZAutoRefCount<cISCLua> lua = SCLuaUtil::GetISCLuaFromFunctionState(pState);

	int32_t parameterCount = lua->GetTop();

	if (parameterCount == 2)
	{
		uint32_t amount = 0;
		uint32_t resourceID = 0;

		// Function parameters are popped off the stack in right-to-left order.

		if (TryGetNumberAsUint32(lua, -1, amount)
			&& TryGetNumberAsUint32(lua, -2, resourceID))
		{
			spRegionalSupplyManager->RemoveFromDemand(resourceID, amount);
		}
	}

	return 0;
}

int32_t RegionalSupplyLua::AddToSupply(lua_State* pState)
{
	cRZAutoRefCount<cISCLua> lua = SCLuaUtil::GetISCLuaFromFunctionState(pState);

	int32_t parameterCount = lua->GetTop();

	if (parameterCount == 2)
	{
		uint32_t amount = 0;
		uint32_t resourceID = 0;

		// Function parameters are popped off the stack in right-to-left order.

		if (TryGetNumberAsUint32(lua, -1, amount)
			&& TryGetNumberAsUint32(lua, -2, resourceID))
		{
			spRegionalSupplyManager->AddToSupply(resourceID, amount);
		}
	}

	return 0;
}

int32_t RegionalSupplyLua::RemoveFromSupply(lua_State* pState)
{
	cRZAutoRefCount<cISCLua> lua = SCLuaUtil::GetISCLuaFromFunctionState(pState);

	int32_t parameterCount = lua->GetTop();

	if (parameterCount == 2)
	{
		uint32_t amount = 0;
		uint32_t resourceID = 0;

		// Function parameters are popped off the stack in right-to-left order.

		if (TryGetNumberAsUint32(lua, -1, amount)
			&& TryGetNumberAsUint32(lua, -2, resourceID))
		{
			spRegionalSupplyManager->RemoveFromSupply(resourceID, amount);
		}
	}

	return 0;
}

int32_t RegionalSupplyLua::GetResourceQuantity(lua_State* pState)
{
	cRZAutoRefCount<cISCLua> lua = SCLuaUtil::GetISCLuaFromFunctionState(pState);

	int64_t quantity = 0;

	int32_t parameterCount = lua->GetTop();

	if (parameterCount == 1)
	{
		uint32_t resourceID = 0;

		if (TryGetNumberAsUint32(lua, -1, resourceID))
		{
			quantity = spRegionalSupplyManager->GetResourceQuantity(resourceID);
		}
	}

	lua->PushNumber(static_cast<double>(quantity));
	return 1;
}
