/*
 * This file is part of sc4-bulldoze-extensions, a DLL Plugin for
 * SimCity 4 extends the bulldoze tool.
 *
 * Copyright (C) 2024, 2025 Nicholas Hayes
 *
 * sc4-bulldoze-extensions is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * sc4-bulldoze-extensions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SC4ClearPollution.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "version.h"
#include "cGZPersistResourceKey.h"
#include "cSC4ViewInputControlDemolishHooks.h"
#include "FileSystem.h"
#include "Logger.h"
#include "cIGZApp.h"
#include "cIGZCheatCodeManager.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cIGZPersistResourceManager.h"
#include "cIGZWin.h"
#include "cIGZWinKeyAccelerator.h"
#include "cIGZWinKeyAcceleratorRes.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4View3DWin.h"
#include "cISC4ViewInputControl.h"
#include "cRZAutoRefCount.h"
#include "cRZBaseString.h"
#include "cRZMessage2COMDirector.h"
#include "GZServPtrs.h"

static constexpr uint32_t kBulldozeExtensionsDirectorID = 0x5B7D9E30;

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePreCityShutdown = 0x26D31EC2;
static constexpr uint32_t kSC4MessageCityEstablished = 0x26D31EC4;

static constexpr uint32_t BulldozeDiagonalShortcutID = 0x6A935D37;
static constexpr uint32_t BulldozeFloraShortcutID = 0x755C6E40;
static constexpr uint32_t BulldozeFloraDiagonalShortcutID = 0x755C6E41;
static constexpr uint32_t BulldozeNetworkShortcutID = 0x5ECED6AE;
static constexpr uint32_t BulldozeNetworkDiagonalShortcutID = 0x5ECED6AF;


class BulldozeExtensionsDllDirector final : public cRZMessage2COMDirector
{
public:
	BulldozeExtensionsDllDirector()
		: pView3D(nullptr)
	{
		Logger& logger = Logger::GetInstance();
		logger.Init(FileSystem::GetLogFilePath(), LogLevel::Info);
		logger.WriteLogFileHeader("SC4BulldozeExtensions v" PLUGIN_VERSION_STR);
	}

private:
	void ActivateBulldozeTool(cSC4ViewInputControlDemolishHooks::BulldozeCursor cursorID)
	{
		if (pView3D)
		{
			constexpr uint32_t BulldozeControlID = 0x46ddb5f1;

			cISC4ViewInputControl* pCurrentControl = pView3D->GetCurrentViewInputControl();

			if (pCurrentControl == nullptr || pCurrentControl->GetID() != BulldozeControlID)
			{
				cRZAutoRefCount<cISC4ViewInputControl> pControl = cSC4ViewInputControlDemolishHooks::CreateViewInputControl(cursorID);

				pView3D->SetCurrentViewInputControl(pControl, cISC4View3DWin::ViewInputControlStackOperation::RemoveAllControls);
			}
		}
	}

	void RegisterBulldozeShortcuts(cIGZMessageServer2& ms2)
	{
		if (pView3D)
		{
			cIGZPersistResourceManagerPtr pRM;

			if (pRM)
			{
				cRZAutoRefCount<cIGZWinKeyAcceleratorRes> pAcceleratorRes;

				// We use a private KeyConfig file to avoid the conflicts that can come with overriding
				// the city KeyConfig file.

				const cGZPersistResourceKey key(0xA2E3D533, 0x6930B865, 0x3A80C2A5);

				if (pRM->GetPrivateResource(key, kGZIID_cIGZWinKeyAcceleratorRes, pAcceleratorRes.AsPPVoid(), 0, nullptr))
				{
					pAcceleratorRes->RegisterResources(pView3D->GetKeyAccelerator());

					ms2.AddNotification(this, BulldozeDiagonalShortcutID);
					ms2.AddNotification(this, BulldozeFloraShortcutID);
					ms2.AddNotification(this, BulldozeFloraDiagonalShortcutID);
					ms2.AddNotification(this, BulldozeNetworkShortcutID);
					ms2.AddNotification(this, BulldozeNetworkDiagonalShortcutID);
				}
			}
		}
	}

	void UnregisterBulldozeShortcutNotifications()
	{
		cIGZMessageServer2Ptr pMS2;

		if (pMS2)
		{
			pMS2->RemoveNotification(this, BulldozeDiagonalShortcutID);
			pMS2->RemoveNotification(this, BulldozeFloraShortcutID);
			pMS2->RemoveNotification(this, BulldozeFloraDiagonalShortcutID);
			pMS2->RemoveNotification(this, BulldozeNetworkShortcutID);
			pMS2->RemoveNotification(this, BulldozeNetworkDiagonalShortcutID);
		}
	}

	void CityEstablished()
	{
		cIGZMessageServer2Ptr pMS2;

		if (pMS2)
		{
			RegisterBulldozeShortcuts(*pMS2);

			pMS2->RemoveNotification(this, kSC4MessageCityEstablished);
		}
	}

	void PostCityInit()
	{
		cISC4AppPtr pSC4App;
		cIGZMessageServer2Ptr pMS2;

		if (pSC4App && pMS2)
		{
			constexpr uint32_t kGZWin_WinSC4App = 0x6104489A;
			constexpr uint32_t kGZWin_SC4View3DWin = 0x9a47b417;
			constexpr uint32_t kGZIID_cISC4View3DWin = 0xFA47B3F9;

			cIGZWin* pMainWindow = pSC4App->GetMainWindow();

			if (pMainWindow)
			{
				cIGZWin* pWinSC4App = pMainWindow->GetChildWindowFromID(kGZWin_WinSC4App);

				if (pWinSC4App)
				{
					if (pWinSC4App->GetChildAs(
						kGZWin_SC4View3DWin,
						kGZIID_cISC4View3DWin,
						reinterpret_cast<void**>(&pView3D)))
					{
						cISC4City* pCity = pSC4App->GetCity();

						if (pCity)
						{
							if (pCity->GetEstablished())
							{
								RegisterBulldozeShortcuts(*pMS2);
							}
							else
							{
								pMS2->AddNotification(this, kSC4MessageCityEstablished);
							}
						}
					}
				}
			}
		}
	}

	void PreCityShutdown()
	{
		UnregisterBulldozeShortcutNotifications();

		cISC4View3DWin* localView3D = pView3D;
		pView3D = nullptr;

		if (localView3D)
		{
			localView3D->Release();
		}
	}

	bool DoMessage(cIGZMessage2* pMsg)
	{
		switch (pMsg->GetType())
		{
		case kSC4MessageCityEstablished:
			CityEstablished();
			break;
		case kSC4MessagePostCityInit:
			PostCityInit();
			break;
		case kSC4MessagePreCityShutdown:
			PreCityShutdown();
			break;
		case BulldozeDiagonalShortcutID:
			ActivateBulldozeTool(cSC4ViewInputControlDemolishHooks::BulldozeCursorDefaultDiagonal);
			break;
		case BulldozeFloraShortcutID:
			ActivateBulldozeTool(cSC4ViewInputControlDemolishHooks::BulldozeCursorFlora);
			break;
		case BulldozeFloraDiagonalShortcutID:
			ActivateBulldozeTool(cSC4ViewInputControlDemolishHooks::BulldozeCursorFloraDiagonal);
			break;
		case BulldozeNetworkShortcutID:
			ActivateBulldozeTool(cSC4ViewInputControlDemolishHooks::BulldozeCursorNetwork);
			break;
		case BulldozeNetworkDiagonalShortcutID:
			ActivateBulldozeTool(cSC4ViewInputControlDemolishHooks::BulldozeCursorNetworkDiagonal);
		}

		return true;
	}

	uint32_t GetDirectorID() const
	{
		return kBulldozeExtensionsDirectorID;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		mpFrameWork->AddHook(this);
		return true;
	}

	bool PostAppInit()
	{
		if (cSC4ViewInputControlDemolishHooks::Install())
		{
			cIGZMessageServer2Ptr pMS2;

			if (pMS2)
			{
				pMS2->AddNotification(this, kSC4MessagePostCityInit);
				pMS2->AddNotification(this, kSC4MessagePreCityShutdown);
			}
		}

		return true;
	}

	cISC4View3DWin* pView3D;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static BulldozeExtensionsDllDirector sDirector;
	return &sDirector;
}