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

#include "RegionalSupplyManager.h"
#include "cGZPersistResourceKey.h"
#include "cIGZDBSegmentPackedFile.h"
#include "cIGZPersistDBRecord.h"
#include "cIGZPersistDBSegment.h"
#include "cIGZPersistDBSerialRecord.h"
#include "cRZAutoRefCount.h"
#include "Logger.h"

static const cGZPersistResourceKey key(0xA82A8BEC, 0x655AEDB3, 1);

void RegionalSupplyManager::Load(cIGZPersistDBSegment* pSegment)
{
	resources.clear();

	cRZAutoRefCount<cIGZPersistDBRecord> record;

	if (pSegment->OpenRecord(key, record.AsPPObj(), cIGZFile::AccessMode::Read))
	{
		cRZAutoRefCount<cIGZPersistDBSerialRecord> serialRecord;

		if (record->QueryInterface(
			GZIID_cIGZPersistDBSerialRecord,
			serialRecord.AsPPVoid()))
		{
			if (!LoadFromSerialRecord(*serialRecord))
			{
				Logger::GetInstance().WriteLine(
					LogLevel::Error,
					"Failed to load the region resource data.");
				resources.clear();
			}

			pSegment->CloseRecord(serialRecord->AsIGZPersistDBRecord());
		}
	}
}

void RegionalSupplyManager::Save(cIGZPersistDBSegment* pSegment) const
{
	if (!resources.empty())
	{
		cRZAutoRefCount<cIGZPersistDBRecord> record;

		if (pSegment->OpenRecord(key, record.AsPPObj(), cIGZFile::AccessMode::ReadWrite))
		{
			cRZAutoRefCount<cIGZPersistDBSerialRecord> serialRecord;

			if (record->QueryInterface(
				GZIID_cIGZPersistDBSerialRecord,
				serialRecord.AsPPVoid()))
			{
				if (SaveToSerialRecord(*serialRecord))
				{
					pSegment->CloseRecord(serialRecord->AsIGZPersistDBRecord());
				}
				else
				{
					Logger::GetInstance().WriteLine(
						LogLevel::Error,
						"Failed to save the region resource data.");
					pSegment->AbortRecord(serialRecord->AsIGZPersistDBRecord());
				}
			}
		}
	}
}

void RegionalSupplyManager::AddToDemand(uint32_t resourceID, uint32_t amount)
{
	RemoveFromSupply(resourceID, amount);
}

void RegionalSupplyManager::RemoveFromDemand(uint32_t resourceID, uint32_t amount)
{
	AddToSupply(resourceID, amount);
}

void RegionalSupplyManager::AddToSupply(uint32_t resourceID, uint32_t amount)
{
	auto it = resources.find(resourceID);

	if (it != resources.end())
	{
		it->second += amount;
	}
	else
	{
		resources.emplace(resourceID, static_cast<int64_t>(amount));
	}
}

void RegionalSupplyManager::RemoveFromSupply(uint32_t resourceID, uint32_t amount)
{
	auto it = resources.find(resourceID);

	if (it != resources.end())
	{
		it->second -= amount;
	}
	else
	{
		resources.emplace(resourceID, -static_cast<int64_t>(amount));
	}
}

int64_t RegionalSupplyManager::GetResourceQuantity(uint32_t resourceID) const
{
	int64_t supply = 0;

	auto it = resources.find(resourceID);

	if (it != resources.end())
	{
		supply = it->second;
	}

	return supply;
}

bool RegionalSupplyManager::LoadFromSerialRecord(cIGZPersistDBSerialRecord& record)
{
	uint32_t version = 0;

	if (!record.GetFieldUint32(version) || version != 1)
	{
		return false;
	}

	uint32_t itemCount = 0;

	if (!record.GetFieldUint32(itemCount))
	{
		return false;
	}

	for (uint32_t i = 0; i < itemCount; i++)
	{
		uint32_t resourceID = 0;

		if (!record.GetFieldUint32(resourceID))
		{
			return false;
		}

		int64_t resourceQuantity = 0;

		if (!record.GetFieldSint64(resourceQuantity))
		{
			return false;
		}

		resources.emplace(resourceID, resourceQuantity);
	}

	return true;
}

bool RegionalSupplyManager::SaveToSerialRecord(cIGZPersistDBSerialRecord& record) const
{
	if (!record.SetFieldUint32(1)) // version
	{
		return false;
	}

	if (!record.SetFieldUint32(resources.size()))
	{
		return false;
	}

	for (auto& item : resources)
	{
		if (!record.SetFieldUint32(item.first)) // resource id
		{
			return false;
		}

		if (!record.SetFieldSint64(item.second)) // resource quantity
		{
			return false;
		}
	}

	return true;
}
