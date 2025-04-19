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

#include "Logger.h"
#include "version.h"
#include "cIGZCheatCodeManager.h"
#include "cIGZCommandParameterSet.h"
#include "cIGZCommandServer.h"
#include "cIGZCOM.h"
#include "cIGZDBSegmentPackedFile.h"
#include "cIGZFrameWork.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cIGZPersistDBSegment.h"
#include "cIGZVariant.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4Occupant.h"
#include "cISC4Region.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cISCStringDetokenizer.h"
#include "cRZAutoRefCount.h"
#include "cRZBaseString.h"
#include "cRZMessage2COMDirector.h"
#include "DebugUtil.h"
#include "GlobalPointers.h"
#include "GZServPtrs.h"
#include "PropertyUtil.h"
#include "RegionalSupplyLua.h"
#include "RegionalSupplyManager.h"
#include "SC4String.h"
#include "SCLuaUtil.h"

#include <array>
#include <string>
#include <vector>

#include <Windows.h>
#include "wil/resource.h"
#include "wil/result.h"
#include "wil/win32_helpers.h"

static constexpr uint32_t kSC4MessageInsertOccupant = 0x99EF1142;
static constexpr uint32_t kSC4MessageRemoveOccupant = 0x99EF1143;
static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePostCityShutdown = 0x26D31EC3;
static constexpr uint32_t kSC4MessagePostRegionInit = 0xCBB5BB45;

static constexpr std::array<uint32_t, 5> RequiredNotifications =
{
	kSC4MessageInsertOccupant,
	kSC4MessageRemoveOccupant,
	kSC4MessagePostCityInit,
	kSC4MessagePostCityShutdown,
	kSC4MessagePostRegionInit
};

static constexpr uint32_t OccupantTypeBuilding = 0x278128A0;

static constexpr uint32_t kRegionalSupplyDemandDllDirector = 0x21E2B214;

static constexpr uint32_t RegionalSupplyConsumed = 0x16F4C223;
static constexpr uint32_t RegionalSupplyProduced = 0x16F4C224;

static constexpr std::string_view PluginLogFileName = "SC4RegionalSupplyDemand.log";
static constexpr std::string_view RegionalSupplyDataFileName = "RegionalSupplyData.dat";

namespace
{
	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}

	void PathCombine(cRZBaseString& path, const std::string_view& segment)
	{
		uint32_t length = path.Strlen();

		if (length > 0 && segment.size() > 0)
		{
			if (path.ToChar()[length - 1] != '\\')
			{
				// Append a backslash if necessary.
				path.Append("\\", 1);
			}

			path.Append(segment.data(), segment.size());
		}
	}

	void PathCombine(cRZBaseString& path, const cIGZString& segment)
	{
		PathCombine(path, std::string_view(segment.Data(), segment.Strlen()));
	}

	cRZBaseString GetRegionalSupplyDataPath()
	{
		cRZBaseString path;

		cISC4AppPtr sc4App;

		if (sc4App)
		{
			cRZBaseString tempPath;

			sc4App->GetRegionsDirectory(tempPath);

			cISC4Region* pRegion = sc4App->GetRegion();

			if (pRegion)
			{
				PathCombine(tempPath, *pRegion->GetDirectoryName()->AsIGZString());
				PathCombine(tempPath, RegionalSupplyDataFileName);

				path = std::move(tempPath);
			}
		}

		return path;
	}

	struct ResourceEntry
	{
		uint32_t id;
		uint32_t amount;
	};

	bool GetResourceEntries(const cISCPropertyHolder* pPropertyHolder, uint32_t id, std::vector<ResourceEntry>& entries)
	{
		bool result = false;

		entries.clear();

		const cISCProperty* pProperty = pPropertyHolder->GetProperty(id);

		if (pProperty)
		{
			const cIGZVariant* pVariant = pProperty->GetPropertyValue();

			if (pVariant)
			{
				const uint16_t type = pVariant->GetType();

				if (type == cIGZVariant::Uint32Array)
				{
					const uint32_t count = pVariant->GetCount();

					if (count > 0)
					{
						if ((count % 2) == 0)
						{
							const uint32_t* pData = pVariant->RefUint32();

							entries.reserve(count / 2);

							for (uint32_t i = 0; i < count; i += 2)
							{
								uint32_t id = pData[i];
								uint32_t amount = pData[i + 1];

								entries.emplace_back(id,  amount);
							}

							result = true;
						}
						else
						{
							Logger& logger = Logger::GetInstance();

							cRZBaseString displayName;

							if (PropertyUtil::GetDisplayName(pPropertyHolder, displayName))
							{
								logger.WriteLineFormatted(
									LogLevel::Error,
									"%s has an invalid 0x%08X property, the values must be id/amount pair(s).",
									displayName.ToChar(),
									id);
							}
							else
							{
								logger.WriteLineFormatted(
									LogLevel::Error,
									"Invalid 0x%08X property, the values must be id/amount pair(s).",
									id);
							}
						}
					}
				}
			}
		}

		return result;
	}

	void RegisterLuaFunction(
		cISC4AdvisorSystem* pAdvisorSystem,
		const char* tableName,
		const char* functionName,
		lua_CFunction callback)
	{
		Logger& logger = Logger::GetInstance();

		SCLuaUtil::RegisterLuaFunctionStatus status = SCLuaUtil::RegisterLuaFunction(
			pAdvisorSystem,
			tableName,
			functionName,
			callback);

		if (status == SCLuaUtil::RegisterLuaFunctionStatus::Ok)
		{
			logger.WriteLineFormatted(
				LogLevel::Info,
				"Registered the %s.%s function",
				tableName,
				functionName);
		}
		else
		{
			if (status == SCLuaUtil::RegisterLuaFunctionStatus::NullParameter)
			{
				logger.WriteLineFormatted(
					LogLevel::Info,
					"Failed to register the %s.%s function. "
					"One or more SCLuaUtil::RegisterLuaFunction parameters were NULL.",
					tableName,
					functionName);
			}
			else if (status == SCLuaUtil::RegisterLuaFunctionStatus::TableWrongType)
			{
				logger.WriteLineFormatted(
					LogLevel::Info,
					"Failed to register the %s.%s function. The %s object is not a Lua table.",
					tableName,
					functionName,
					tableName);
			}
			else
			{
				logger.WriteLineFormatted(
					LogLevel::Info,
					"Failed to register the %s.%s function. "
					"Is RegionalSupplyDemand.dat in the plugins folder?",
					tableName,
					functionName);
			}
		}
	}

	cRZBaseString Detokenize(cISCStringDetokenizer& detokenizer, const std::string_view& tokenName)
	{
		cRZBaseString result;

		cRZBaseString tokenizedValue("#");
		tokenizedValue.Append(tokenName.data(), tokenName.size());
		tokenizedValue.Append("#", 1);

		detokenizer.Detokenize(tokenizedValue, result);

		return result;
	}

	void DebugTestLuaAPI()
	{
#ifdef _DEBUG
		cISC4AppPtr sc4App;

		cISCStringDetokenizer* pDetokenizer = sc4App->GetStringDetokenizer();

		if (pDetokenizer)
		{
			Detokenize(*pDetokenizer, "regional_supply.add_to_demand(1, 50)");
			Detokenize(*pDetokenizer, "regional_supply.remove_from_demand(1, 25)");
			Detokenize(*pDetokenizer, "regional_supply.add_to_supply(1, 75)");
			Detokenize(*pDetokenizer, "regional_supply.remove_from_supply(1, 25)");

			cRZBaseString quantity = Detokenize(*pDetokenizer, "regional_supply.get_resource_quantity(1)");
			DebugUtil::PrintLineToDebugOutput(quantity.ToChar());
		}
#endif // _DEBUG
	}
}

IRegionalSupplyManager* spRegionalSupplyManager = nullptr;

class RegionalSupplyDemandDllDirector final : public cRZMessage2COMDirector
{
public:

	RegionalSupplyDemandDllDirector()
		: regionalSupplyDataPath(),
		  regionalSupplyManager(),
		  exitedCity(false)
	{
		spRegionalSupplyManager = &regionalSupplyManager;

		std::filesystem::path dllFolderPath = GetDllFolderPath();

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Error);
		logger.WriteLogFileHeader("SC4RegionalSupplyDemand v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kRegionalSupplyDemandDllDirector;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		mpFrameWork->AddHook(this);
		return true;
	}

private:
	bool DoMessage(cIGZMessage2* pMsg)
	{
		switch (pMsg->GetType())
		{
		case kSC4MessageInsertOccupant:
			OccupantInserted(static_cast<cIGZMessage2Standard*>(pMsg));
			break;
		case kSC4MessageRemoveOccupant:
			OccupantRemoved(static_cast<cIGZMessage2Standard*>(pMsg));
			break;
		case kSC4MessagePostCityShutdown:
			exitedCity = true;
			break;
		case kSC4MessagePostCityInit:
			PostCityInit(static_cast<cIGZMessage2Standard*>(pMsg));
			break;
		case kSC4MessagePostRegionInit:
			PostRegionInit();
			break;
		}

		return true;
	}

	void OccupantInserted(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4Occupant* pOccupant = static_cast<cISC4Occupant*>(pStandardMsg->GetVoid1());

		if (pOccupant->GetType() == OccupantTypeBuilding)
		{
			const cISCPropertyHolder* pPropertyHolder = pOccupant->AsPropertyHolder();

			std::vector<ResourceEntry> supplyConsumed;

			if (GetResourceEntries(pPropertyHolder, RegionalSupplyConsumed, supplyConsumed))
			{
				for (const auto& entry : supplyConsumed)
				{
					regionalSupplyManager.AddToDemand(entry.id, entry.amount);
				}
			}

			std::vector<ResourceEntry> supplyProduced;

			if (GetResourceEntries(pPropertyHolder, RegionalSupplyProduced, supplyProduced))
			{
				for (const auto& entry : supplyProduced)
				{
					regionalSupplyManager.AddToSupply(entry.id, entry.amount);
				}
			}
		}
	}

	void OccupantRemoved(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4Occupant* pOccupant = static_cast<cISC4Occupant*>(pStandardMsg->GetVoid1());

		if (pOccupant->GetType() == OccupantTypeBuilding)
		{
			const cISCPropertyHolder* pPropertyHolder = pOccupant->AsPropertyHolder();

			std::vector<ResourceEntry> supplyConsumed;

			if (GetResourceEntries(pPropertyHolder, RegionalSupplyConsumed, supplyConsumed))
			{
				for (const auto& entry : supplyConsumed)
				{
					regionalSupplyManager.RemoveFromDemand(entry.id, entry.amount);
				}
			}

			std::vector<ResourceEntry> supplyProduced;

			if (GetResourceEntries(pPropertyHolder, RegionalSupplyProduced, supplyProduced))
			{
				for (const auto& entry : supplyProduced)
				{
					regionalSupplyManager.RemoveFromSupply(entry.id, entry.amount);
				}
			}
		}
	}

	void PostCityInit(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4City* pCity = static_cast<cISC4City*>(pStandardMsg->GetVoid1());

		if (pCity)
		{
			cISC4AdvisorSystem* pAdvisorSystem = pCity->GetAdvisorSystem();

			if (pAdvisorSystem)
			{
				const char* const tableName = "regional_supply";

				RegisterLuaFunction(
					pAdvisorSystem,
					tableName,
					"add_to_demand",
					RegionalSupplyLua::AddToDemand);
				RegisterLuaFunction(
					pAdvisorSystem,
					tableName,
					"remove_from_demand",
					RegionalSupplyLua::RemoveFromDemand);
				RegisterLuaFunction(
					pAdvisorSystem,
					tableName,
					"add_to_supply",
					RegionalSupplyLua::AddToSupply);
				RegisterLuaFunction(
					pAdvisorSystem,
					tableName,
					"remove_from_supply",
					RegionalSupplyLua::RemoveFromSupply);
				RegisterLuaFunction(
					pAdvisorSystem,
					tableName,
					"get_resource_quantity",
					RegionalSupplyLua::GetResourceQuantity);

#ifdef _DEBUG
				DebugTestLuaAPI();
#endif // _DEBUG
			}
		}
	}

	void PostRegionInit()
	{
		if (exitedCity)
		{
			exitedCity = false;
			SaveRegionData();
		}
		else
		{
			regionalSupplyDataPath = GetRegionalSupplyDataPath();
			LoadRegionData();
		}
	}

	void LoadRegionData()
	{
		if (regionalSupplyDataPath.Strlen() > 0)
		{
			cRZAutoRefCount<cIGZPersistDBSegment> segment;

			if (mpCOM->GetClassObject(
				GZCLSID_cGZDBSegmentPackedFile,
				GZIID_cIGZPersistDBSegment,
				segment.AsPPVoid()))
			{
				if (segment->Init() && segment->SetPath(regionalSupplyDataPath))
				{
					if (segment->Open(true, false))
					{
						regionalSupplyManager.Load(segment);
					}
				}
			}
		}
	}

	void SaveRegionData()
	{
		if (regionalSupplyDataPath.Strlen() > 0)
		{
			cRZAutoRefCount<cIGZPersistDBSegment> segment;

			if (mpCOM->GetClassObject(
				GZCLSID_cGZDBSegmentPackedFile,
				GZIID_cIGZPersistDBSegment,
				segment.AsPPVoid()))
			{
				if (segment->Init() && segment->SetPath(regionalSupplyDataPath))
				{
					if (segment->Open(true, true))
					{
						regionalSupplyManager.Save(segment);
					}
				}
			}
		}
	}

	bool PostAppInit()
	{
		Logger& logger = Logger::GetInstance();

		cIGZMessageServer2Ptr ms2;

		for (uint32_t messageID : RequiredNotifications)
		{
			if (!ms2->AddNotification(this, messageID))
			{
				logger.WriteLine(LogLevel::Error, "Failed to subscribe to the required notifications.");
				return false;
			}
		}

		return true;
	}

	cRZBaseString regionalSupplyDataPath;
	RegionalSupplyManager regionalSupplyManager;
	bool exitedCity;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static RegionalSupplyDemandDllDirector sDirector;
	return &sDirector;
}