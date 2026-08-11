/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus_settings_controls.h"

#include <base/math.h>
#include <base/system.h>

#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/localization.h>
#include <engine/textrender.h>

#include <game/client/components/binds.h>
#include <game/client/components/key_binder.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <functional>
#include <string>
#include <vector>

inline constexpr float HEADER_FONT_SIZE = 16.0f;
inline constexpr float FONT_SIZE = 13.0f;
inline constexpr float MARGIN = 10.0f;
inline constexpr float BUTTON_HEIGHT = 20.0f;
inline constexpr float BUTTON_SPACING = 2.0f;
inline constexpr float BIND_OPTION_SPACING = 4.0f;

bool CBindSlotUiElement::operator<(const CBindSlotUiElement &Other) const
{
	if(m_Bind == EMPTY_BIND_SLOT)
	{
		return false;
	}
	if(Other.m_Bind == EMPTY_BIND_SLOT)
	{
		return true;
	}
	return m_Bind.m_ModifierMask < Other.m_Bind.m_ModifierMask ||
	       m_Bind.m_Key < Other.m_Bind.m_Key;
}

std::vector<CBindSlotUiElement>::iterator CBindOption::GetBindSlotElement(const CBindSlot &BindSlot)
{
	return std::find_if(m_vCurrentBinds.begin(), m_vCurrentBinds.end(), [&](const CBindSlotUiElement &BindSlotUiElement) {
		return BindSlotUiElement.m_Bind == BindSlot;
	});
}

bool CBindOption::MatchesSearch(const char *pSearch) const
{
	return (m_Group != EBindOptionGroup::CUSTOM && str_utf8_find_nocase(Localize(m_pLabel), pSearch) != nullptr) ||
	       str_utf8_find_nocase(m_Command.c_str(), pSearch) != nullptr;
}

void CMenusSettingsControls::OnInterfacesInit(CGameClient *pClient)
{
	CComponentInterfaces::OnInterfacesInit(pClient);

	m_vBindOptions = {
		{EBindOptionGroup::MOVEMENT, Localizable("Move left"), "+left"},
		{EBindOptionGroup::MOVEMENT, Localizable("Move right"), "+right"},
		{EBindOptionGroup::MOVEMENT, Localizable("Jump"), "+jump"},
		{EBindOptionGroup::MOVEMENT, Localizable("Fire"), "+fire"},
		{EBindOptionGroup::MOVEMENT, Localizable("Hook"), "+hook"},
		{EBindOptionGroup::MOVEMENT, Localizable("Hook collisions"), "+showhookcoll"},
		{EBindOptionGroup::MOVEMENT, Localizable("Pause"), "say /pause"},
		{EBindOptionGroup::MOVEMENT, Localizable("Kill"), "kill"},
		{EBindOptionGroup::MOVEMENT, Localizable("Zoom in"), "zoom+"},
		{EBindOptionGroup::MOVEMENT, Localizable("Zoom out"), "zoom-"},
		{EBindOptionGroup::MOVEMENT, Localizable("Default zoom"), "zoom"},
		{EBindOptionGroup::MOVEMENT, Localizable("Show others"), "say /showothers"},
		{EBindOptionGroup::MOVEMENT, Localizable("Show all"), "say /showall"},
		{EBindOptionGroup::MOVEMENT, Localizable("Toggle dyncam"), "toggle cl_dyncam 0 1"},
		{EBindOptionGroup::MOVEMENT, Localizable("Toggle ghost"), "toggle cl_race_show_ghost 0 1"},
		{EBindOptionGroup::WEAPON, Localizable("Hammer"), "+weapon1"},
		{EBindOptionGroup::WEAPON, Localizable("Pistol"), "+weapon2"},
		{EBindOptionGroup::WEAPON, Localizable("Shotgun"), "+weapon3"},
		{EBindOptionGroup::WEAPON, Localizable("Grenade"), "+weapon4"},
		{EBindOptionGroup::WEAPON, Localizable("Laser"), "+weapon5"},
		{EBindOptionGroup::WEAPON, Localizable("Next weapon"), "+nextweapon"},
		{EBindOptionGroup::WEAPON, Localizable("Prev. weapon"), "+prevweapon"},
		{EBindOptionGroup::VOTING, Localizable("Vote yes"), "vote yes"},
		{EBindOptionGroup::VOTING, Localizable("Vote no"), "vote no"},
		{EBindOptionGroup::CHAT, Localizable("Chat"), "+show_chat; chat all"},
		{EBindOptionGroup::CHAT, Localizable("Team chat"), "+show_chat; chat team"},
		{EBindOptionGroup::CHAT, Localizable("Converse"), "+show_chat; chat all /c "},
		{EBindOptionGroup::CHAT, Localizable("Chat command"), "+show_chat; chat all /"},
		{EBindOptionGroup::CHAT, Localizable("Show chat"), "+show_chat"},
		{EBindOptionGroup::DUMMY, Localizable("Toggle dummy"), "toggle cl_dummy 0 1"},
		{EBindOptionGroup::DUMMY, Localizable("Dummy copy"), "toggle cl_dummy_copy_moves 0 1"},
		{EBindOptionGroup::DUMMY, Localizable("Hammerfly dummy"), "toggle cl_dummy_hammer 0 1"},
		{EBindOptionGroup::MY_BINDS, Localizable("Dummy left"), "+dummy_left"},
		{EBindOptionGroup::MY_BINDS, Localizable("Dummy right"), "+dummy_right"},
		{EBindOptionGroup::MY_BINDS, Localizable("Dummy jump"), "+dummy_jump"},
		{EBindOptionGroup::MY_BINDS, Localizable("Dummy hammer"), "+dummy_hammer"},
		{EBindOptionGroup::MY_BINDS, Localizable("Dummy copy without hook"), "dummy_copy"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Emoticon"), "+emote"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Spectator mode"), "+spectate"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Spectate next"), "spectate_next"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Spectate previous"), "spectate_previous"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Console"), "toggle_local_console"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Remote console"), "toggle_remote_console"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Screenshot"), "screenshot"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Scoreboard"), "+scoreboard"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Scoreboard cursor"), "toggle_scoreboard_cursor"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Statboard"), "+statboard"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Lock team"), "say /lock"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Show entities"), "toggle cl_overlay_entities 0 100"},
		{EBindOptionGroup::MISCELLANEOUS, Localizable("Show HUD"), "toggle cl_showhud 0 1"},
	};
	m_NumPredefinedBindOptions = m_vBindOptions.size();

	std::fill(std::begin(m_aBindGroupExpanded), std::end(m_aBindGroupExpanded), true);
	m_aBindGroupExpanded[(int)EBindOptionGroup::CUSTOM] = false;

	m_JoystickDropDownState.m_SelectionPopupContext.m_pScrollRegion = &m_JoystickDropDownScrollRegion;
}

void CMenusSettingsControls::Render(CUIRect MainView)
{
	UpdateBindOptions();

	CUIRect QuickSearch, SearchMatches, ResetToDefault;
	MainView.HSplitBottom(BUTTON_HEIGHT, &MainView, &QuickSearch);
	QuickSearch.VSplitRight(200.0f, &QuickSearch, &ResetToDefault);
	QuickSearch.VSplitRight(MARGIN, &QuickSearch, nullptr);
	QuickSearch.VSplitRight(150.0f, &QuickSearch, &SearchMatches);
	QuickSearch.VSplitRight(MARGIN, &QuickSearch, nullptr);
	MainView.HSplitBottom(MARGIN, &MainView, nullptr);

	// Quick search
	if(Ui()->DoEditBox_Search(&m_FilterInput, &QuickSearch, FONT_SIZE, !Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive() && !GameClient()->m_KeyBinder.IsActive()))
	{
		m_CurrentSearchMatch = 0;
		UpdateSearchMatches();
		m_SearchMatchReveal = true;
	}
	else if(!m_vSearchMatches.empty() && (Ui()->ConsumeHotkey(CUi::EHotkey::HOTKEY_ENTER) || Ui()->ConsumeHotkey(CUi::EHotkey::HOTKEY_TAB)))
	{
		UpdateSearchMatches();
		m_CurrentSearchMatch += Input()->ShiftIsPressed() ? -1 : 1;
		if(m_CurrentSearchMatch >= (int)m_vSearchMatches.size())
		{
			m_CurrentSearchMatch = 0;
		}
		if(m_CurrentSearchMatch < 0)
		{
			m_CurrentSearchMatch = m_vSearchMatches.size() - 1;
		}
		m_SearchMatchReveal = true;
	}

	if(!m_FilterInput.IsEmpty())
	{
		if(!m_vSearchMatches.empty())
		{
			char aSearchMatchLabel[64];
			str_format(aSearchMatchLabel, sizeof(aSearchMatchLabel), Localize("Match %d of %d"), m_CurrentSearchMatch + 1, (int)m_vSearchMatches.size());
			Ui()->DoLabel(&SearchMatches, aSearchMatchLabel, FONT_SIZE, TEXTALIGN_MC);
		}
		else
		{
			Ui()->DoLabel(&SearchMatches, Localize("No results"), FONT_SIZE, TEXTALIGN_MC);
		}
	}

	// Reset to default button
	if(GameClient()->m_Menus.DoButton_Menu(&m_ResetToDefaultButton, Localize("Reset to defaults"), 0, &ResetToDefault))
	{
		GameClient()->m_Menus.PopupConfirm(Localize("Reset controls"), Localize("Are you sure that you want to reset the controls to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsControls);
	}

	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 6.0f * BUTTON_HEIGHT;
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	m_SettingsScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.y += ScrollOffset.y;

	CUIRect LeftColumn, RightColumn;
	MainView.VSplitMid(&LeftColumn, &RightColumn, MARGIN);

	// Left column
	RenderSettingsBlock(MeasureSettingsMouseHeight(), &LeftColumn,
		Localize("Mouse"), nullptr, nullptr, std::bind_front(&CMenusSettingsControls::RenderSettingsMouse, this));
	RenderSettingsBlock(MeasureSettingsJoystickHeight(), &LeftColumn,
		Localize("Controller"), nullptr, nullptr, std::bind_front(&CMenusSettingsControls::RenderSettingsJoystick, this));
	RenderSettingsBindsBlock(EBindOptionGroup::MOVEMENT, &LeftColumn, Localize("Movement"));
	RenderSettingsBindsBlock(EBindOptionGroup::WEAPON, &LeftColumn, Localize("Weapon"));

	// Right column
	RenderSettingsBindsBlock(EBindOptionGroup::VOTING, &RightColumn, Localize("Voting"));
	RenderSettingsBindsBlock(EBindOptionGroup::CHAT, &RightColumn, Localize("Chat"));
	RenderSettingsBindsBlock(EBindOptionGroup::DUMMY, &RightColumn, Localize("Dummy"));
	RenderSettingsBindsBlock(EBindOptionGroup::MISCELLANEOUS, &RightColumn, Localize("Miscellaneous"));
	if(std::any_of(m_vBindOptions.begin(), m_vBindOptions.end(), [](const CBindOption &Option) { return Option.m_Group == EBindOptionGroup::CUSTOM; }))
	{
		RenderSettingsBindsBlock(EBindOptionGroup::CUSTOM, &RightColumn, Localize("Custom"));
	}

	m_SettingsScrollRegion.End();
}

void CMenusSettingsControls::RenderMyBinds(CUIRect MainView)
{
	UpdateBindOptions();
	const CUIRect BackgroundView = MainView;
	if(g_Config.m_RcGqAnimatedBackground)
	{
		// A lightweight animated constellation inspired by modern custom clients.
		// Everything is generated from deterministic points, so no external DLS
		// assets or code are required.
		constexpr int NumNetworkPoints = 21;
		constexpr int MaxNetworkLines = NumNetworkPoints * (NumNetworkPoints - 1) / 2;
		constexpr float aNetworkSeedX[NumNetworkPoints] = {0.03f, 0.11f, 0.19f, 0.28f, 0.37f, 0.46f, 0.55f, 0.64f, 0.73f, 0.82f, 0.94f, 0.08f, 0.23f, 0.34f, 0.49f, 0.61f, 0.76f, 0.89f, 0.16f, 0.43f, 0.68f};
		constexpr float aNetworkSeedY[NumNetworkPoints] = {0.18f, 0.62f, 0.33f, 0.84f, 0.09f, 0.55f, 0.25f, 0.77f, 0.42f, 0.91f, 0.14f, 0.95f, 0.48f, 0.69f, 0.04f, 0.88f, 0.18f, 0.59f, 0.73f, 0.36f, 0.66f};
		const float BackgroundTime = std::fmod(Client()->GlobalTime(), 4096.0f);
		const float AnimationSpeed = g_Config.m_RcGqBackgroundCubeSpeed / 100.0f;
		vec2 aNetworkPoints[NumNetworkPoints];
		IGraphics::CLineItem aNetworkLines[MaxNetworkLines];
		IGraphics::CQuadItem aNetworkDots[NumNetworkPoints];
		IGraphics::CFreeformItem aNetworkTriangles[NumNetworkPoints / 3];
		int NumNetworkLines = 0;
		for(int i = 0; i < NumNetworkPoints; ++i)
		{
			aNetworkPoints[i] = vec2(
				BackgroundView.x + aNetworkSeedX[i] * BackgroundView.w + std::sin(BackgroundTime * (0.07f + i * 0.001f) * AnimationSpeed + i * 1.9f) * 24.0f,
				BackgroundView.y + aNetworkSeedY[i] * BackgroundView.h + std::cos(BackgroundTime * (0.06f + i * 0.0015f) * AnimationSpeed + i * 1.4f) * 18.0f);
			const float DotSize = 1.5f + 0.7f * (0.5f + 0.5f * std::sin(BackgroundTime * 0.8f + i));
			aNetworkDots[i] = IGraphics::CQuadItem(aNetworkPoints[i].x - DotSize / 2.0f, aNetworkPoints[i].y - DotSize / 2.0f, DotSize, DotSize);
		}
		const float LinkDistance = g_Config.m_RcGqBackgroundLineDistance;
		for(int i = 0; i < NumNetworkPoints; ++i)
		{
			for(int j = i + 1; j < NumNetworkPoints; ++j)
			{
				const vec2 Delta = aNetworkPoints[i] - aNetworkPoints[j];
				if(dot(Delta, Delta) < LinkDistance * LinkDistance)
					aNetworkLines[NumNetworkLines++] = IGraphics::CLineItem(aNetworkPoints[i], aNetworkPoints[j]);
			}
		}
		for(int i = 0; i < NumNetworkPoints / 3; ++i)
		{
			const vec2 P0 = aNetworkPoints[i * 3];
			const vec2 P1 = aNetworkPoints[i * 3 + 1];
			const vec2 P2 = aNetworkPoints[i * 3 + 2];
			aNetworkTriangles[i] = IGraphics::CFreeformItem(P0, P1, P2, P2);
		}

		Ui()->ClipEnable(&BackgroundView);
		Graphics()->TextureClear();
		if(g_Config.m_RcGqBackgroundTriangles)
		{
			Graphics()->QuadsBegin();
			Graphics()->SetColor(ColorRGBA(0.72f, 0.52f, 0.9f, 0.035f));
			Graphics()->QuadsDrawFreeform(aNetworkTriangles, std::size(aNetworkTriangles));
			Graphics()->QuadsEnd();
		}
		if(g_Config.m_RcGqBackgroundLines)
		{
			Graphics()->LinesBegin();
			Graphics()->SetColor(ColorRGBA(0.82f, 0.84f, 0.88f, 0.105f));
			Graphics()->LinesDraw(aNetworkLines, NumNetworkLines);
			Graphics()->LinesEnd();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(ColorRGBA(0.9f, 0.9f, 0.92f, 0.3f));
			Graphics()->QuadsDrawTL(aNetworkDots, NumNetworkPoints);
			Graphics()->QuadsEnd();
		}

		// Two counter-rotating triangle orbits add depth without textures.
		constexpr int MaxOrbitTriangles = 28;
		const int NumOrbitTriangles = g_Config.m_RcGqBackgroundTriangleCount;
		IGraphics::CFreeformItem aOrbitTriangles[MaxOrbitTriangles];
		const vec2 OrbitCenter(BackgroundView.x + BackgroundView.w * 0.51f, BackgroundView.y + BackgroundView.h * 0.48f);
		for(int i = 0; i < NumOrbitTriangles; ++i)
		{
			const float Direction = i % 2 == 0 ? 1.0f : -1.0f;
			const float Angle = Direction * BackgroundTime * (0.08f + (i % 3) * 0.018f) * AnimationSpeed + i * 2.399f;
			const float RadiusX = 105.0f + (i % 5) * 42.0f;
			const float RadiusY = 45.0f + (i % 4) * 34.0f;
			const vec2 Center(OrbitCenter.x + std::cos(Angle) * RadiusX, OrbitCenter.y + std::sin(Angle) * RadiusY);
			const float Size = 5.0f + (i % 4) * 2.2f;
			const vec2 Tip = Center + vec2(std::cos(Angle), std::sin(Angle)) * Size;
			const vec2 Side(-std::sin(Angle) * Size * 0.65f, std::cos(Angle) * Size * 0.65f);
			aOrbitTriangles[i] = IGraphics::CFreeformItem(Tip, Center + Side, Center - Side, Center - Side);
		}
		if(g_Config.m_RcGqBackgroundTriangles)
		{
			Graphics()->QuadsBegin();
			Graphics()->SetColor(ColorRGBA(0.78f, 0.64f, 0.92f, 0.17f));
			Graphics()->QuadsDrawFreeform(aOrbitTriangles, NumOrbitTriangles);
			Graphics()->QuadsEnd();
		}
		Ui()->ClipDisable();

		constexpr int MaxCubes = 10;
		constexpr int EdgesPerCube = 12;
		constexpr int VerticesPerCube = 8;
		constexpr float aSeedX[MaxCubes] = {0.07f, 0.18f, 0.31f, 0.44f, 0.58f, 0.69f, 0.77f, 0.86f, 0.93f, 0.39f};
		constexpr float aSeedY[MaxCubes] = {0.12f, 0.73f, 0.38f, 0.89f, 0.21f, 0.57f, 0.06f, 0.46f, 0.81f, 0.64f};
		constexpr float aBaseSizes[MaxCubes] = {20.0f, 28.0f, 18.0f, 36.0f, 24.0f, 42.0f, 22.0f, 32.0f, 26.0f, 38.0f};
		constexpr int aEdges[EdgesPerCube][2] = {
			{0, 1}, {1, 3}, {3, 2}, {2, 0},
			{4, 5}, {5, 7}, {7, 6}, {6, 4},
			{0, 4}, {1, 5}, {2, 6}, {3, 7}};
		const int NumCubes = g_Config.m_RcGqBackgroundCubes;
		const float SizeScale = g_Config.m_RcGqBackgroundCubeSize / 100.0f;
		const float SpeedScale = g_Config.m_RcGqBackgroundCubeSpeed / 100.0f;
		const float Time = BackgroundTime;
		IGraphics::CLineItem aLines[MaxCubes * EdgesPerCube];
		IGraphics::CQuadItem aPoints[MaxCubes * VerticesPerCube];
		int NumLines = 0;
		int NumPoints = 0;
		for(int i = 0; i < NumCubes; ++i)
		{
			const float Size = aBaseSizes[i] * SizeScale;
			const float CenterX = BackgroundView.x + aSeedX[i] * BackgroundView.w + std::sin(Time * 0.12f + i * 1.7f) * 18.0f;
			const float CenterY = BackgroundView.y + aSeedY[i] * BackgroundView.h + std::cos(Time * 0.1f + i * 1.3f) * 12.0f;
			const float AngleX = Time * (0.28f + i * 0.013f) * SpeedScale + i * 0.71f;
			const float AngleY = Time * (0.36f + i * 0.017f) * SpeedScale + i * 0.43f;
			const float AngleZ = Time * (0.16f + i * 0.009f) * SpeedScale + i * 0.29f;
			const float SinX = std::sin(AngleX), CosX = std::cos(AngleX);
			const float SinY = std::sin(AngleY), CosY = std::cos(AngleY);
			const float SinZ = std::sin(AngleZ), CosZ = std::cos(AngleZ);
			vec2 aProjected[VerticesPerCube];
			for(int Vertex = 0; Vertex < VerticesPerCube; ++Vertex)
			{
				float X = (Vertex & 1) ? 1.0f : -1.0f;
				float Y = (Vertex & 2) ? 1.0f : -1.0f;
				float Z = (Vertex & 4) ? 1.0f : -1.0f;
				const float RotatedY = Y * CosX - Z * SinX;
				Z = Y * SinX + Z * CosX;
				Y = RotatedY;
				const float RotatedX = X * CosY + Z * SinY;
				Z = -X * SinY + Z * CosY;
				X = RotatedX;
				const float FinalX = X * CosZ - Y * SinZ;
				const float FinalY = X * SinZ + Y * CosZ;
				const float Perspective = 2.8f / (3.8f - Z);
				aProjected[Vertex] = vec2(CenterX + FinalX * Size * Perspective, CenterY + FinalY * Size * Perspective);
				const float PointSize = 2.2f;
				aPoints[NumPoints++] = IGraphics::CQuadItem(aProjected[Vertex].x - PointSize / 2.0f, aProjected[Vertex].y - PointSize / 2.0f, PointSize, PointSize);
			}
			for(const auto &Edge : aEdges)
				aLines[NumLines++] = IGraphics::CLineItem(aProjected[Edge[0]], aProjected[Edge[1]]);
		}
		Ui()->ClipEnable(&BackgroundView);
		Graphics()->TextureClear();
		Graphics()->LinesBegin();
		Graphics()->SetColor(ColorRGBA(1.0f, 1.0f, 1.0f, 0.16f));
		Graphics()->LinesDraw(aLines, NumLines);
		Graphics()->LinesEnd();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(ColorRGBA(1.0f, 1.0f, 1.0f, 0.32f));
		Graphics()->QuadsDrawTL(aPoints, NumPoints);
		Graphics()->QuadsEnd();
		Ui()->ClipDisable();
	}
	const int ShotgunEffectForLayout = g_Config.m_RcShotgunPulse;
	const int LaserEffectForLayout = g_Config.m_RcLaserPulse;
	const char *apPulseNames[] = {Localize("Default"), Localize("Pulse Spiral"), Localize("Prismatic"), Localize("Crystal Glow"), Localize("Enchanted Glow")};
	static CUi::SDropDownState s_ShotgunPulseDropDownState;
	static CUi::SDropDownState s_LaserPulseDropDownState;
	s_ShotgunPulseDropDownState.m_SelectionPopupContext.m_pScrollRegion = &m_ShotgunPulseDropDownScrollRegion;
	s_LaserPulseDropDownState.m_SelectionPopupContext.m_pScrollRegion = &m_LaserPulseDropDownScrollRegion;

	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 6.0f * BUTTON_HEIGHT;
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	m_SettingsScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.y += ScrollOffset.y;
	const float SideMargin = maximum(0.0f, (MainView.w - 720.0f) / 2.0f);
	MainView.VMargin(SideMargin, &MainView);
	const auto MeasureRows = [](int NumRows) {
		return NumRows * BUTTON_HEIGHT + maximum(0, NumRows - 1) * BUTTON_SPACING;
	};
	const auto NextRow = [](CUIRect &View, CUIRect &Row, bool &FirstRow) {
		if(!FirstRow)
			View.HSplitTop(BUTTON_SPACING, nullptr, &View);
		View.HSplitTop(BUTTON_HEIGHT, &Row, &View);
		FirstRow = false;
	};

	CUIRect LeftColumn, RightColumn;
	MainView.VSplitMid(&LeftColumn, &RightColumn, 18.0f);
	m_GqTransparentStyle = true;

	// Left column: the settings that directly affect gameplay presentation.
	const int EffectRows = 2 + (ShotgunEffectForLayout != 0) + (LaserEffectForLayout != 0) + (ShotgunEffectForLayout == 4) + (LaserEffectForLayout == 4);
	RenderSettingsBlock(MeasureRows(EffectRows), &LeftColumn, Localize("Weapon effects"), nullptr, nullptr, [&](CUIRect View) {
		CUIRect Row, Label, Selector;
		bool FirstRow = true;
		NextRow(View, Row, FirstRow);
		Row.VSplitMid(&Label, &Selector, MARGIN);
		Ui()->DoLabel(&Label, Localize("Shotgun"), FONT_SIZE, TEXTALIGN_ML);
		g_Config.m_RcShotgunPulse = Ui()->DoDropDown(&Selector, g_Config.m_RcShotgunPulse, apPulseNames, std::size(apPulseNames), s_ShotgunPulseDropDownState);
		int *pShotgunThickness = ShotgunEffectForLayout == 1 ? &g_Config.m_RcShotgunPulseThickness :
			ShotgunEffectForLayout == 2 ? &g_Config.m_RcShotgunPrismaticThickness :
			ShotgunEffectForLayout == 3 ? &g_Config.m_RcShotgunCrystalGlowThickness :
			ShotgunEffectForLayout == 4 ? &g_Config.m_RcShotgunEnchantedGlowThickness : nullptr;
		if(pShotgunThickness)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(pShotgunThickness, pShotgunThickness, &Row, Localize("Beam thickness"), 25, 300, &CUi::ms_LinearScrollbarScale, 0, "%");
		}
		if(ShotgunEffectForLayout == 4)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcShotgunEnchantedGlowPower, &g_Config.m_RcShotgunEnchantedGlowPower, &Row, Localize("Glow power"), 1, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
		}
		NextRow(View, Row, FirstRow);
		Row.VSplitMid(&Label, &Selector, MARGIN);
		Ui()->DoLabel(&Label, Localize("Laser"), FONT_SIZE, TEXTALIGN_ML);
		g_Config.m_RcLaserPulse = Ui()->DoDropDown(&Selector, g_Config.m_RcLaserPulse, apPulseNames, std::size(apPulseNames), s_LaserPulseDropDownState);
		int *pLaserThickness = LaserEffectForLayout == 1 ? &g_Config.m_RcLaserPulseThickness :
			LaserEffectForLayout == 2 ? &g_Config.m_RcLaserPrismaticThickness :
			LaserEffectForLayout == 3 ? &g_Config.m_RcLaserCrystalGlowThickness :
			LaserEffectForLayout == 4 ? &g_Config.m_RcLaserEnchantedGlowThickness : nullptr;
		if(pLaserThickness)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(pLaserThickness, pLaserThickness, &Row, Localize("Beam thickness"), 25, 300, &CUi::ms_LinearScrollbarScale, 0, "%");
		}
		if(LaserEffectForLayout == 4)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcLaserEnchantedGlowPower, &g_Config.m_RcLaserEnchantedGlowPower, &Row, Localize("Glow power"), 1, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
		}
	});

	RenderSettingsBlock(MeasureRows(2), &LeftColumn, Localize("Trajectory helper"), nullptr, nullptr, [&](CUIRect View) {
		CUIRect Row;
		bool FirstRow = true;
		NextRow(View, Row, FirstRow);
		if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcShowWeaponCollision, Localize("Show weapon trajectories"), g_Config.m_RcShowWeaponCollision, &Row))
			g_Config.m_RcShowWeaponCollision ^= 1;
		NextRow(View, Row, FirstRow);
		Ui()->DoLabel(&Row, Localize("Uses the active hook-collision key"), 11.0f, TEXTALIGN_ML);
	});

	RenderSettingsBindsBlock(EBindOptionGroup::MY_BINDS, &LeftColumn, Localize("Dummy quick binds"));

	// Right column: HUD widgets and the visual style of this page.
	const int NoHookRows = g_Config.m_RcDummyCopyHud ? 5 : 1;
	RenderSettingsBlock(MeasureRows(NoHookRows), &RightColumn, Localize("COPY / NO HOOK HUD"), nullptr, nullptr, [&](CUIRect View) {
		CUIRect Row;
		bool FirstRow = true;
		NextRow(View, Row, FirstRow);
		if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcDummyCopyHud, Localize("Enable widget"), g_Config.m_RcDummyCopyHud, &Row))
			g_Config.m_RcDummyCopyHud ^= 1;
		if(g_Config.m_RcDummyCopyHud)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcDummyCopyHudX, &g_Config.m_RcDummyCopyHudX, &Row, Localize("Horizontal position"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcDummyCopyHudY, &g_Config.m_RcDummyCopyHudY, &Row, Localize("Vertical position"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcDummyCopyHudSize, &g_Config.m_RcDummyCopyHudSize, &Row, Localize("Size"), 20, 300, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcDummyCopyHudSide, Localize("Attach to right side"), g_Config.m_RcDummyCopyHudSide, &Row))
				g_Config.m_RcDummyCopyHudSide ^= 1;
		}
	});

	const int TowerRows = g_Config.m_RcTowerHud ? 4 : 1;
	RenderSettingsBlock(MeasureRows(TowerRows), &RightColumn, Localize("TOWER HUD"), nullptr, nullptr, [&](CUIRect View) {
		CUIRect Row;
		bool FirstRow = true;
		NextRow(View, Row, FirstRow);
		if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcTowerHud, Localize("Enable widget"), g_Config.m_RcTowerHud, &Row))
			g_Config.m_RcTowerHud ^= 1;
		if(g_Config.m_RcTowerHud)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcTowerHudX, &g_Config.m_RcTowerHudX, &Row, Localize("Horizontal position"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcTowerHudY, &g_Config.m_RcTowerHudY, &Row, Localize("Vertical position"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcTowerHudSize, &g_Config.m_RcTowerHudSize, &Row, Localize("Size"), 20, 300, &CUi::ms_LinearScrollbarScale, 0, "%");
		}
	});

	const int AppearanceRows = g_Config.m_RcGqAnimatedBackground ? 6 + g_Config.m_RcGqBackgroundTriangles + g_Config.m_RcGqBackgroundLines : 1;
	RenderSettingsBlock(MeasureRows(AppearanceRows), &RightColumn, Localize("Page atmosphere"), nullptr, nullptr, [&](CUIRect View) {
		CUIRect Row;
		bool FirstRow = true;
		NextRow(View, Row, FirstRow);
		if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcGqAnimatedBackground, Localize("Animated background"), g_Config.m_RcGqAnimatedBackground, &Row))
			g_Config.m_RcGqAnimatedBackground ^= 1;
		if(g_Config.m_RcGqAnimatedBackground)
		{
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcGqBackgroundCubes, &g_Config.m_RcGqBackgroundCubes, &Row, Localize("Cube count"), 1, 10);
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcGqBackgroundCubeSize, &g_Config.m_RcGqBackgroundCubeSize, &Row, Localize("Cube size"), 50, 200, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			Ui()->DoScrollbarOption(&g_Config.m_RcGqBackgroundCubeSpeed, &g_Config.m_RcGqBackgroundCubeSpeed, &Row, Localize("Rotation speed"), 0, 200, &CUi::ms_LinearScrollbarScale, 0, "%");
			NextRow(View, Row, FirstRow);
			if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcGqBackgroundTriangles, Localize("Animated triangles"), g_Config.m_RcGqBackgroundTriangles, &Row))
				g_Config.m_RcGqBackgroundTriangles ^= 1;
			if(g_Config.m_RcGqBackgroundTriangles)
			{
				NextRow(View, Row, FirstRow);
				Ui()->DoScrollbarOption(&g_Config.m_RcGqBackgroundTriangleCount, &g_Config.m_RcGqBackgroundTriangleCount, &Row, Localize("Triangle count"), 2, 28);
			}
			NextRow(View, Row, FirstRow);
			if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_RcGqBackgroundLines, Localize("Constellation lines"), g_Config.m_RcGqBackgroundLines, &Row))
				g_Config.m_RcGqBackgroundLines ^= 1;
			if(g_Config.m_RcGqBackgroundLines)
			{
				NextRow(View, Row, FirstRow);
				Ui()->DoScrollbarOption(&g_Config.m_RcGqBackgroundLineDistance, &g_Config.m_RcGqBackgroundLineDistance, &Row, Localize("Line distance"), 60, 240);
			}
		}
	});

	CUIRect BottomPadding;
	CUIRect &BottomColumn = LeftColumn.y > RightColumn.y ? LeftColumn : RightColumn;
	BottomColumn.HSplitTop(MARGIN, &BottomPadding, &BottomColumn);
	m_SettingsScrollRegion.AddRect(BottomPadding);

	m_SettingsScrollRegion.End();
	m_GqTransparentStyle = false;
}

void CMenusSettingsControls::UpdateBindOptions()
{
	for(CBindOption &Option : m_vBindOptions)
	{
		for(CBindSlotUiElement &BindSlot : Option.m_vCurrentBinds)
		{
			if(BindSlot.m_Bind != EMPTY_BIND_SLOT)
			{
				BindSlot.m_ToBeDeleted = true;
			}
		}
	}

	for(int Mod = KeyModifier::NONE; Mod < KeyModifier::COMBINATION_COUNT; Mod++)
	{
		for(int KeyId = KEY_FIRST; KeyId < KEY_LAST; KeyId++)
		{
			const CBindSlot BindSlot = CBindSlot(KeyId, Mod);
			const char *pBind = GameClient()->m_Binds.Get(BindSlot);
			if(!pBind[0])
			{
				continue;
			}

			auto ExistingOption = std::find_if(m_vBindOptions.begin(), m_vBindOptions.end(), [pBind](const CBindOption &Option) {
				return str_comp(pBind, Option.m_Command.c_str()) == 0;
			});
			if(ExistingOption == m_vBindOptions.end())
			{
				// Bind option not found for command, add custom bind option.
				CBindOption NewOption = {EBindOptionGroup::CUSTOM, nullptr, pBind};
				ExistingOption = m_vBindOptions.insert(
					std::upper_bound(m_vBindOptions.begin() + m_NumPredefinedBindOptions, m_vBindOptions.end(), NewOption, [&](const CBindOption &Option1, const CBindOption &Option2) {
						return str_utf8_comp_nocase(Option1.m_Command.c_str(), Option2.m_Command.c_str()) < 0;
					}),
					NewOption);

				// Update search matches due to new option being added.
				if(!m_FilterInput.IsEmpty())
				{
					const int OptionIndex = ExistingOption - m_vBindOptions.begin();
					for(int &SearchMatch : m_vSearchMatches)
					{
						if(OptionIndex <= SearchMatch)
						{
							++SearchMatch;
						}
					}
					if(ExistingOption->MatchesSearch(m_FilterInput.GetString()))
					{
						const int MatchIndex = m_vSearchMatches.insert(std::upper_bound(m_vSearchMatches.begin(), m_vSearchMatches.end(), OptionIndex), OptionIndex) - m_vSearchMatches.begin();
						if(MatchIndex <= m_CurrentSearchMatch)
						{
							++m_CurrentSearchMatch;
						}
					}
				}
			}
			auto ExistingBindSlot = ExistingOption->GetBindSlotElement(BindSlot);
			if(ExistingBindSlot == ExistingOption->m_vCurrentBinds.end())
			{
				// Remove empty bind slot if one is present because it will be replaced with a bind slot for the new bind.
				auto ExistingEmptyBindSlot = ExistingOption->GetBindSlotElement(EMPTY_BIND_SLOT);
				if(ExistingEmptyBindSlot != ExistingOption->m_vCurrentBinds.end())
				{
					ExistingOption->m_vCurrentBinds.erase(ExistingEmptyBindSlot);
				}

				CBindSlotUiElement BindSlotUiElement = {BindSlot};
				ExistingOption->m_vCurrentBinds.insert(
					std::upper_bound(ExistingOption->m_vCurrentBinds.begin(), ExistingOption->m_vCurrentBinds.end(), BindSlotUiElement),
					BindSlotUiElement);
			}
			else
			{
				ExistingBindSlot->m_ToBeDeleted = false;
			}
		}
	}

	// Remove bind slots that are not bound anymore,
	// mark unused custom bind options for removal.
	for(CBindOption &Option : m_vBindOptions)
	{
		Option.m_vCurrentBinds.erase(std::remove_if(Option.m_vCurrentBinds.begin(), Option.m_vCurrentBinds.end(),
						     [&](const CBindSlotUiElement &BindSlotUiElement) { return BindSlotUiElement.m_ToBeDeleted; }),
			Option.m_vCurrentBinds.end());

		Option.m_ToBeDeleted = Option.m_vCurrentBinds.empty() && Option.m_Group == EBindOptionGroup::CUSTOM;
		if(Option.m_ToBeDeleted)
		{
			continue;
		}

		if(Option.m_vCurrentBinds.empty() ||
			(Option.m_AddNewBind && Option.GetBindSlotElement(EMPTY_BIND_SLOT) == Option.m_vCurrentBinds.end()))
		{
			Option.m_vCurrentBinds.emplace_back(EMPTY_BIND_SLOT);
		}
	}

	// Update search matches when removing bind options.
	for(const CBindOption &Option : m_vBindOptions)
	{
		if(!Option.m_ToBeDeleted)
		{
			continue;
		}
		const int OptionIndex = &Option - m_vBindOptions.data();
		auto ExactSearchMatch = std::find(m_vSearchMatches.begin(), m_vSearchMatches.end(), OptionIndex);
		if(ExactSearchMatch != m_vSearchMatches.end())
		{
			m_vSearchMatches.erase(ExactSearchMatch);
			if((int)(ExactSearchMatch - m_vSearchMatches.begin()) < m_CurrentSearchMatch)
			{
				--m_CurrentSearchMatch;
			}
		}
		for(int &SearchMatch : m_vSearchMatches)
		{
			if(OptionIndex < SearchMatch)
			{
				--SearchMatch;
			}
		}
	}
	if(m_vSearchMatches.empty())
	{
		m_CurrentSearchMatch = 0;
	}
	else if(m_CurrentSearchMatch >= (int)m_vSearchMatches.size())
	{
		m_CurrentSearchMatch = m_vSearchMatches.size() - 1;
	}

	// Remove unused bind options.
	m_vBindOptions.erase(std::remove_if(m_vBindOptions.begin() + m_NumPredefinedBindOptions, m_vBindOptions.end(),
				     [&](const CBindOption &Option) { return Option.m_ToBeDeleted; }),
		m_vBindOptions.end());
}

void CMenusSettingsControls::UpdateSearchMatches()
{
	m_vSearchMatches.clear();

	if(!m_FilterInput.IsEmpty())
	{
		for(CBindOption &Option : m_vBindOptions)
		{
			if(!Option.MatchesSearch(m_FilterInput.GetString()))
			{
				continue;
			}

			m_aBindGroupExpanded[(int)Option.m_Group] = true;
			m_vSearchMatches.emplace_back(&Option - m_vBindOptions.data());
		}
	}

	if(m_vSearchMatches.empty())
	{
		m_CurrentSearchMatch = 0;
	}
	else if(m_CurrentSearchMatch >= (int)m_vSearchMatches.size())
	{
		m_CurrentSearchMatch = m_vSearchMatches.size() - 1;
	}
}

void CMenusSettingsControls::RenderSettingsBlock(float Height, CUIRect *pParentRect, const char *pTitle,
	bool *pExpanded, CButtonContainer *pExpandButton, const std::function<void(CUIRect Rect)> &RenderContentFunction)
{
	const bool WasExpanded = pExpanded == nullptr || *pExpanded;
	float FullHeight = WasExpanded ? Height : 0.0f; // Content
	FullHeight += pTitle == nullptr ? 0.0f : HEADER_FONT_SIZE + (WasExpanded ? MARGIN : 0.0f); // Title and spacing
	FullHeight += 2.0f * MARGIN; // Margin

	CUIRect SettingsBlock;
	pParentRect->HSplitTop(FullHeight, &SettingsBlock, pParentRect);
	pParentRect->HSplitTop(m_GqTransparentStyle ? 16.0f : MARGIN, nullptr, pParentRect);
	if(m_SettingsScrollRegion.AddRect(SettingsBlock) || m_SearchMatchReveal)
	{
		if(m_GqTransparentStyle)
		{
			const bool Hovered = pExpandButton != nullptr && Ui()->HotItem() == pExpandButton;
			SettingsBlock.Draw(ColorRGBA(0.055f, 0.06f, 0.07f, Hovered ? 0.7f : 0.56f), IGraphics::CORNER_ALL, 10.0f);
		}
		else
		{
			SettingsBlock.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, pExpandButton == nullptr || Ui()->HotItem() != pExpandButton ? 0.25f : 0.3f), IGraphics::CORNER_ALL, 10.0f);
		}
		SettingsBlock.Margin(MARGIN, &SettingsBlock);

		if(pTitle != nullptr)
		{
			CUIRect Label;
			SettingsBlock.HSplitTop(HEADER_FONT_SIZE, &Label, &SettingsBlock);
			if(WasExpanded)
			{
				SettingsBlock.HSplitTop(MARGIN, nullptr, &SettingsBlock);
			}

			if(pExpanded != nullptr)
			{
				CUIRect ButtonArea;
				Label.Margin(-MARGIN, &ButtonArea);
				if(Ui()->DoButtonLogic(pExpandButton, 0, &ButtonArea, BUTTONFLAG_LEFT))
				{
					*pExpanded = !*pExpanded;
				}

				CUIRect ExpandButton;
				Label.VSplitRight(20.0f, &Label, &ExpandButton);
				Label.VSplitRight(BUTTON_SPACING, &Label, nullptr);
				if(m_SettingsScrollRegion.AddRect(ExpandButton))
				{
					SLabelProperties Props;
					Props.SetColor(ColorRGBA(1.0f, 1.0f, 1.0f, 0.65f * Ui()->ButtonColorMul(pExpandButton)));
					Props.m_EnableWidthCheck = false;
					TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
					TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
					Ui()->DoLabel(&ExpandButton, *pExpanded ? FontIcon::CHEVRON_UP : FontIcon::CHEVRON_DOWN, HEADER_FONT_SIZE, TEXTALIGN_MR, Props);
					TextRender()->SetRenderFlags(0);
					TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
				}
			}

			if(m_SettingsScrollRegion.AddRect(Label))
			{
				Ui()->DoLabel(&Label, pTitle, HEADER_FONT_SIZE, TEXTALIGN_ML);
			}
		}

		if(WasExpanded)
		{
			RenderContentFunction(SettingsBlock);
		}
	}
}

void CMenusSettingsControls::RenderSettingsBindsBlock(EBindOptionGroup Group, CUIRect *pParentRect, const char *pTitle)
{
	RenderSettingsBlock(MeasureSettingsBindsHeight(Group), pParentRect, pTitle,
		&m_aBindGroupExpanded[(int)Group], &m_aBindGroupExpandButtons[(int)Group],
		[&](CUIRect Rect) { RenderSettingsBinds(Group, Rect); });
}

float CMenusSettingsControls::MeasureSettingsBindsHeight(EBindOptionGroup Group) const
{
	float Height = 0.0f;
	for(const CBindOption &BindOption : m_vBindOptions)
	{
		if(BindOption.m_Group != Group)
		{
			continue;
		}
		if(Height > 0.0f)
		{
			Height += BIND_OPTION_SPACING;
		}
		Height += BUTTON_HEIGHT * BindOption.m_vCurrentBinds.size() + BUTTON_SPACING * (BindOption.m_vCurrentBinds.size() - 1) + BIND_OPTION_SPACING;
	}
	return Height;
}

void CMenusSettingsControls::RenderSettingsBinds(EBindOptionGroup Group, CUIRect View)
{
	bool FirstOption = true;
	for(CBindOption &BindOption : m_vBindOptions)
	{
		if(BindOption.m_Group != Group)
		{
			continue;
		}

		if(!FirstOption)
			View.HSplitTop(BIND_OPTION_SPACING, nullptr, &View);
		FirstOption = false;
		CUIRect KeyReaders;
		View.HSplitTop(BUTTON_HEIGHT * BindOption.m_vCurrentBinds.size() + BUTTON_SPACING * (BindOption.m_vCurrentBinds.size() - 1) + 4.0f, &KeyReaders, &View);
		if(!m_SettingsScrollRegion.AddRect(KeyReaders) && !m_SearchMatchReveal)
		{
			continue;
		}
		KeyReaders.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f), IGraphics::CORNER_ALL, 5.0f);
		KeyReaders.Margin(2.0f, &KeyReaders);

		CUIRect Label, AddButton;
		KeyReaders.VSplitLeft(KeyReaders.w / 3.0f, &Label, &KeyReaders);
		KeyReaders.VSplitLeft(5.0f, nullptr, &KeyReaders);
		KeyReaders.VSplitLeft(BUTTON_HEIGHT, &AddButton, &KeyReaders);
		AddButton.HSplitTop(BUTTON_HEIGHT, &AddButton, nullptr);
		KeyReaders.VSplitLeft(2.0f, nullptr, &KeyReaders);
		Label.HSplitTop(BUTTON_HEIGHT, &Label, nullptr);

		const auto SearchMatch = std::find(m_vSearchMatches.begin(), m_vSearchMatches.end(), &BindOption - m_vBindOptions.data());
		const bool SearchMatchSelected = SearchMatch != m_vSearchMatches.end() && m_CurrentSearchMatch == (int)(SearchMatch - m_vSearchMatches.begin());
		if(SearchMatchSelected && m_SearchMatchReveal)
		{
			m_SearchMatchReveal = false;
			// Scroll to reveal search match
			CUIRect ScrollTarget;
			Label.HMargin(-MARGIN, &ScrollTarget);
			m_SettingsScrollRegion.AddRect(ScrollTarget, true);
		}
		SLabelProperties LabelProps = {.m_MaxWidth = Label.w, .m_EllipsisAtEnd = BindOption.m_Group == EBindOptionGroup::CUSTOM, .m_MinimumFontSize = 9.0f};
		if(SearchMatchSelected)
		{
			LabelProps.SetColor(ColorRGBA(0.1f, 0.1f, 1.0f, 1.0f));
		}
		else if(SearchMatch != m_vSearchMatches.end())
		{
			LabelProps.SetColor(ColorRGBA(0.4f, 0.4f, 0.9f, 1.0f));
		}
		const CLabelResult LabelResult = Ui()->DoLabel(&Label, BindOption.m_Group == EBindOptionGroup::CUSTOM ? BindOption.m_Command.c_str() : Localize(BindOption.m_pLabel),
			FONT_SIZE, TEXTALIGN_ML, LabelProps);
		if(BindOption.m_Group != EBindOptionGroup::CUSTOM || LabelResult.m_Truncated)
		{
			Ui()->DoButtonLogic(&BindOption.m_TooltipButtonId, 0, &Label, BUTTONFLAG_NONE);
			GameClient()->m_Tooltips.DoToolTip(&BindOption.m_TooltipButtonId, &Label, BindOption.m_Command.c_str());
		}

		for(CBindSlotUiElement &CurrentBind : BindOption.m_vCurrentBinds)
		{
			CUIRect KeyReader;
			KeyReaders.HSplitTop(BUTTON_HEIGHT, &KeyReader, &KeyReaders);
			KeyReaders.HSplitTop(BUTTON_SPACING, nullptr, &KeyReaders);
			const bool ActivateKeyReader = BindOption.m_AddNewBindActivate && CurrentBind.m_Bind == EMPTY_BIND_SLOT;
			const CKeyBinder::CKeyReaderResult KeyReaderResult = GameClient()->m_KeyBinder.DoKeyReader(
				&CurrentBind.m_KeyReaderButton, &CurrentBind.m_KeyResetButton,
				&KeyReader, CurrentBind.m_Bind, ActivateKeyReader);
			if(ActivateKeyReader)
			{
				BindOption.m_AddNewBindActivate = false;
				// Scroll to reveal activated key reader
				CUIRect ScrollTarget;
				KeyReader.HMargin(-MARGIN, &ScrollTarget);
				m_SettingsScrollRegion.AddRect(ScrollTarget, true);
			}
			if(KeyReaderResult.m_Aborted)
			{
				BindOption.m_AddNewBind = false;
				if(CurrentBind.m_Bind == EMPTY_BIND_SLOT && (&CurrentBind - BindOption.m_vCurrentBinds.data()) > 0)
				{
					CurrentBind.m_ToBeDeleted = true;
				}
			}
			else if(KeyReaderResult.m_Bind != CurrentBind.m_Bind)
			{
				BindOption.m_AddNewBind = false;
				if(CurrentBind.m_Bind.m_Key != KEY_UNKNOWN || KeyReaderResult.m_Bind.m_Key == KEY_UNKNOWN)
				{
					GameClient()->m_Binds.Bind(CurrentBind.m_Bind.m_Key, "", false, CurrentBind.m_Bind.m_ModifierMask);
				}
				if(KeyReaderResult.m_Bind.m_Key != KEY_UNKNOWN)
				{
					GameClient()->m_Binds.Bind(KeyReaderResult.m_Bind.m_Key, BindOption.m_Command.c_str(), false, KeyReaderResult.m_Bind.m_ModifierMask);
				}
			}
		}

		if(Ui()->DoButton_FontIcon(&BindOption.m_AddBindButtonContainer, FontIcon::PLUS, BindOption.m_AddNewBind ? 1 : 0, &AddButton, BUTTONFLAG_LEFT))
		{
			BindOption.m_AddNewBind = true;
			BindOption.m_AddNewBindActivate = true;
		}
	}
}

float CMenusSettingsControls::MeasureSettingsMouseHeight() const
{
	return 2.0f * BUTTON_HEIGHT + BUTTON_SPACING;
}

void CMenusSettingsControls::RenderSettingsMouse(CUIRect View)
{
	CUIRect Button;
	View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
	Ui()->DoScrollbarOption(&g_Config.m_InpMousesens, &g_Config.m_InpMousesens, &Button, Localize("Ingame mouse sens."), 1, 500,
		&CUi::ms_LogarithmicScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

	View.HSplitTop(BUTTON_SPACING, nullptr, &View);

	View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
	Ui()->DoScrollbarOption(&g_Config.m_UiMousesens, &g_Config.m_UiMousesens, &Button, Localize("UI mouse sens."), 1, 500,
		&CUi::ms_LogarithmicScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE | CUi::SCROLLBAR_OPTION_DELAYUPDATE);
}

float CMenusSettingsControls::MeasureSettingsJoystickHeight() const
{
	int NumOptions = 1; // expandable header
	if(g_Config.m_InpControllerEnable)
	{
		NumOptions++; // message or joystick name/selection
		if(Input()->NumJoysticks() > 0)
		{
			NumOptions += 3; // mode, ui sens, tolerance
			if(!g_Config.m_InpControllerAbsolute)
				NumOptions++; // ingame sens
			NumOptions += Input()->GetActiveJoystick()->GetNumAxes() + 1; // axis selection + header
		}
	}
	return NumOptions * (BUTTON_HEIGHT + BUTTON_SPACING) + (NumOptions == 1 ? 0.0f : BUTTON_SPACING);
}

void CMenusSettingsControls::RenderSettingsJoystick(CUIRect View)
{
	CUIRect Button;
	View.HSplitTop(BUTTON_SPACING, nullptr, &View);
	View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
	const bool WasJoystickEnabled = g_Config.m_InpControllerEnable;
	if(GameClient()->m_Menus.DoButton_CheckBox(&g_Config.m_InpControllerEnable, Localize("Enable controller"), g_Config.m_InpControllerEnable, &Button))
	{
		g_Config.m_InpControllerEnable ^= 1;
	}
	if(!WasJoystickEnabled) // Use old value because this was used to allocate the available height
	{
		return;
	}

	const int NumJoysticks = Input()->NumJoysticks();
	if(NumJoysticks > 0)
	{
		// show joystick device selection if more than one available or just the joystick name if there is only one
		{
			CUIRect JoystickDropDown;
			View.HSplitTop(BUTTON_SPACING, nullptr, &View);
			View.HSplitTop(BUTTON_HEIGHT, &JoystickDropDown, &View);
			if(NumJoysticks > 1)
			{
				std::vector<std::string> vJoystickNames;
				std::vector<const char *> vpJoystickNames;
				vJoystickNames.resize(NumJoysticks);
				vpJoystickNames.resize(NumJoysticks);

				for(int i = 0; i < NumJoysticks; ++i)
				{
					char aJoystickName[256];
					str_format(aJoystickName, sizeof(aJoystickName), "%s %d: %s", Localize("Controller"), i, Input()->GetJoystick(i)->GetName());
					vJoystickNames[i] = aJoystickName;
					vpJoystickNames[i] = vJoystickNames[i].c_str();
				}

				const int CurrentJoystick = Input()->GetActiveJoystick()->GetIndex();
				const int NewJoystick = Ui()->DoDropDown(&JoystickDropDown, CurrentJoystick, vpJoystickNames.data(), vpJoystickNames.size(), m_JoystickDropDownState);
				if(NewJoystick != CurrentJoystick)
				{
					Input()->SetActiveJoystick(NewJoystick);
				}
			}
			else
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "%s 0: %s", Localize("Controller"), Input()->GetJoystick(0)->GetName());
				Ui()->DoLabel(&JoystickDropDown, aBuf, FONT_SIZE, TEXTALIGN_ML);
			}
		}

		const bool WasAbsolute = g_Config.m_InpControllerAbsolute;
		GameClient()->m_Menus.DoLine_RadioMenu(View, Localize("Ingame controller mode"),
			m_vJoystickIngameModeButtonContainers,
			{Localize("Relative", "Ingame controller mode"), Localize("Absolute", "Ingame controller mode")},
			{0, 1},
			g_Config.m_InpControllerAbsolute);

		if(!WasAbsolute) // Use old value because this was used to allocate the available height
		{
			View.HSplitTop(BUTTON_SPACING, nullptr, &View);
			View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
			Ui()->DoScrollbarOption(&g_Config.m_InpControllerSens, &g_Config.m_InpControllerSens, &Button, Localize("Ingame controller sens."), 1, 500,
				&CUi::ms_LogarithmicScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);
		}

		View.HSplitTop(BUTTON_SPACING, nullptr, &View);
		View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
		Ui()->DoScrollbarOption(&g_Config.m_UiControllerSens, &g_Config.m_UiControllerSens, &Button, Localize("UI controller sens."), 1, 500,
			&CUi::ms_LogarithmicScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE);

		View.HSplitTop(BUTTON_SPACING, nullptr, &View);
		View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
		Ui()->DoScrollbarOption(&g_Config.m_InpControllerTolerance, &g_Config.m_InpControllerTolerance, &Button, Localize("Controller jitter tolerance"), 0, 50);

		View.HSplitTop(BUTTON_SPACING, nullptr, &View);
		if(m_SettingsScrollRegion.AddRect(View))
		{
			View.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f), IGraphics::CORNER_ALL, 5.0f);
			RenderJoystickAxisPicker(View);
		}
	}
	else
	{
		View.HSplitTop(View.h - BUTTON_HEIGHT, nullptr, &View);
		View.HSplitTop(BUTTON_HEIGHT, &Button, &View);
		Ui()->DoLabel(&Button, Localize("No controller found. Plug in a controller."), FONT_SIZE, TEXTALIGN_ML);
	}
}

void CMenusSettingsControls::RenderJoystickAxisPicker(CUIRect View)
{
	const float AxisWidth = 0.2f * View.w;
	const float StatusWidth = 0.4f * View.w;
	const float AimBindWidth = 90.0f;
	const float SpacingV = (View.w - AxisWidth - StatusWidth - AimBindWidth) / 2.0f;

	CUIRect Row, Axis, Status, AimBind;
	View.HSplitTop(BUTTON_SPACING, nullptr, &View);
	View.HSplitTop(BUTTON_HEIGHT, &Row, &View);
	Row.VSplitLeft(AxisWidth, &Axis, &Row);
	Row.VSplitLeft(SpacingV, nullptr, &Row);
	Row.VSplitLeft(StatusWidth, &Status, &Row);
	Row.VSplitLeft(SpacingV, nullptr, &Row);
	Row.VSplitLeft(AimBindWidth, &AimBind, &Row);

	Ui()->DoLabel(&Axis, Localize("Axis"), FONT_SIZE, TEXTALIGN_MC);
	Ui()->DoLabel(&Status, Localize("Status"), FONT_SIZE, TEXTALIGN_MC);
	Ui()->DoLabel(&AimBind, Localize("Aim bind"), FONT_SIZE, TEXTALIGN_MC);

	IInput::IJoystick *pJoystick = Input()->GetActiveJoystick();
	for(int i = 0; i < std::min<int>(pJoystick->GetNumAxes(), NUM_JOYSTICK_AXES); i++)
	{
		View.HSplitTop(BUTTON_SPACING, nullptr, &View);
		View.HSplitTop(BUTTON_HEIGHT, &Row, &View);
		if(!m_SettingsScrollRegion.AddRect(Row))
		{
			continue;
		}
		Row.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.1f), IGraphics::CORNER_ALL, 5.0f);
		Row.VSplitLeft(AxisWidth, &Axis, &Row);
		Row.VSplitLeft(SpacingV, nullptr, &Row);
		Row.VSplitLeft(StatusWidth, &Status, &Row);
		Row.VSplitLeft(SpacingV, nullptr, &Row);
		Row.VSplitLeft(AimBindWidth, &AimBind, &Row);

		const bool Active = g_Config.m_InpControllerX == i || g_Config.m_InpControllerY == i;

		// Axis label
		char aLabel[16];
		str_format(aLabel, sizeof(aLabel), "%d", i + 1);
		SLabelProperties LabelProps;
		if(!Active)
		{
			LabelProps.SetColor(ColorRGBA(0.7f, 0.7f, 0.7f, 1.0f));
		}
		Ui()->DoLabel(&Axis, aLabel, FONT_SIZE, TEXTALIGN_MC, LabelProps);

		// Axis status
		Status.HMargin(7.0f, &Status);
		RenderJoystickBar(&Status, (pJoystick->GetAxisValue(i) + 1.0f) / 2.0f, g_Config.m_InpControllerTolerance / 50.0f, Active);

		// Bind to X/Y
		CUIRect AimBindX, AimBindY;
		AimBind.VSplitMid(&AimBindX, &AimBindY);
		if(GameClient()->m_Menus.DoButton_CheckBox(&m_aaJoystickAxisCheckboxIds[i][0], "X", g_Config.m_InpControllerX == i, &AimBindX))
		{
			if(g_Config.m_InpControllerY == i)
				g_Config.m_InpControllerY = g_Config.m_InpControllerX;
			g_Config.m_InpControllerX = i;
		}
		if(GameClient()->m_Menus.DoButton_CheckBox(&m_aaJoystickAxisCheckboxIds[i][1], "Y", g_Config.m_InpControllerY == i, &AimBindY))
		{
			if(g_Config.m_InpControllerX == i)
				g_Config.m_InpControllerX = g_Config.m_InpControllerY;
			g_Config.m_InpControllerY = i;
		}
	}
}

void CMenusSettingsControls::RenderJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active)
{
	CUIRect Handle;
	pRect->VSplitLeft(pRect->h, &Handle, nullptr); // Slider size
	Handle.x += (pRect->w - Handle.w) * Current;

	pRect->Draw(ColorRGBA(1.0f, 1.0f, 1.0f, Active ? 0.25f : 0.125f), IGraphics::CORNER_ALL, pRect->h / 2.0f);

	CUIRect ToleranceArea = *pRect;
	ToleranceArea.w *= Tolerance;
	ToleranceArea.x += (pRect->w - ToleranceArea.w) / 2.0f;
	const ColorRGBA ToleranceColor = Active ? ColorRGBA(0.8f, 0.35f, 0.35f, 1.0f) : ColorRGBA(0.7f, 0.5f, 0.5f, 1.0f);
	ToleranceArea.Draw(ToleranceColor, IGraphics::CORNER_ALL, ToleranceArea.h / 2.0f);

	const ColorRGBA SliderColor = Active ? ColorRGBA(0.95f, 0.95f, 0.95f, 1.0f) : ColorRGBA(0.8f, 0.8f, 0.8f, 1.0f);
	Handle.Draw(SliderColor, IGraphics::CORNER_ALL, Handle.h / 2.0f);
}

void CMenus::ResetSettingsControls()
{
	GameClient()->m_Binds.SetDefaults();

	g_Config.m_InpMousesens = 200;
	g_Config.m_UiMousesens = 200;

	g_Config.m_InpControllerEnable = 0;
	g_Config.m_InpControllerGUID[0] = '\0';
	g_Config.m_InpControllerAbsolute = 0;
	g_Config.m_InpControllerSens = 100;
	g_Config.m_InpControllerX = 0;
	g_Config.m_InpControllerY = 1;
	g_Config.m_InpControllerTolerance = 5;
	g_Config.m_UiControllerSens = 100;
}
