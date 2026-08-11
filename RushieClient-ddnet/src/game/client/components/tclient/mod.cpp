#include "mod.h"

#include <base/system.h>

#include <engine/shared/config.h>

#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/localization.h>

static constexpr const float MOD_WEAPON_TIME = 1.5f;

void CMod::OnConsoleInit()
{
	Console()->Chain(
		"+fire", [](IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData) {
			pfnCallback(pResult, pCallbackUserData);
			((CMod *)pUserData)->OnFire(pResult->GetInteger(0));
		},
		this);
}

void CMod::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	Graphics()->TextureClear();
	RenderWeaponCollision();

	// Mod Weapon
	if(Client()->State() == IClient::STATE_ONLINE && m_ModWeaponActiveId >= 0 && m_ModWeaponActiveTimeLeft > 0.0f)
	{
		const auto &Player = GameClient()->m_aClients[m_ModWeaponActiveId];
		if(
			GameClient()->m_Menus.IsActive() || // In escape
			GameClient()->m_Chat.IsActive() || // In chat
			GameClient()->m_GameConsole.IsActive() || // In console
			!Player.m_Active || // In spectator
			g_Config.m_TcModWeaponCommand[0] == '\0') // Not active or empty command
		{
			m_ModWeaponActiveId = -1;
		}
		else
		{
			const float Delta = Client()->RenderFrameTime();
			if(Delta < 1.0f / 30.0f) // Don't do anything if lagging
			{
				m_ModWeaponActiveTimeLeft -= Delta;
				if(m_ModWeaponActiveTimeLeft <= 0.0f)
				{
					m_ModWeaponActiveTimeLeft = 0.0f;
					ModWeapon(m_ModWeaponActiveId);
					m_ModWeaponActiveId = -1;
				}
			}
			float Y = Player.m_RenderPos.y + 20.0f;
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), "%.2f", m_ModWeaponActiveTimeLeft);
				STextContainerIndex TextContainer;
				TextContainer.Reset();
				CTextCursor Cursor;
				Cursor.m_FontSize = 25.0f;
				TextRender()->SetRenderFlags(TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | TEXT_RENDER_FLAG_ONE_TIME_USE);
				TextRender()->CreateTextContainer(TextContainer, &Cursor, aBuf);
				TextRender()->SetRenderFlags(0);
				if(TextContainer.Valid())
				{
					const auto Color = color_cast<ColorRGBA>(ColorHSLA(m_ModWeaponActiveTimeLeft / MOD_WEAPON_TIME, 0.5f, 0.5f, 1.0f));
					const auto BoundingBox = TextRender()->GetBoundingBoxTextContainer(TextContainer);
					TextRender()->RenderTextContainer(TextContainer,
						Color, TextRender()->DefaultTextOutlineColor(),
						Player.m_RenderPos.x - BoundingBox.m_W / 2.0f, Y);
					Y += BoundingBox.m_H + 15.0f;
				}
				TextRender()->DeleteTextContainer(TextContainer);
			}
			{
				STextContainerIndex TextContainer;
				TextContainer.Reset();
				CTextCursor Cursor;
				Cursor.m_FontSize = 15.0f;
				TextRender()->SetRenderFlags(TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | TEXT_RENDER_FLAG_ONE_TIME_USE);
				TextRender()->CreateTextContainer(TextContainer, &Cursor, g_Config.m_TcModWeaponCommand);
				TextRender()->SetRenderFlags(0);
				if(TextContainer.Valid())
				{
					const auto BoundingBox = TextRender()->GetBoundingBoxTextContainer(TextContainer);
					TextRender()->RenderTextContainer(TextContainer,
						ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f), TextRender()->DefaultTextOutlineColor(),
						Player.m_RenderPos.x - BoundingBox.m_W / 2.0f, Y);
				}
				TextRender()->DeleteTextContainer(TextContainer);
			}
		}
	}

	// Hitboxes
	if(g_Config.m_TcShowPlayerHitBoxes > 0)
	{
		auto RenderHitbox = [&](vec2 Position, float Alpha) {
			if(Alpha <= 0.0f)
				return;
			const float RadiusInner = 16.0f;
			const float RadiusOuter = 30.0f;
			Graphics()->QuadsBegin();
			Graphics()->SetColor(ColorRGBA(0.0f, 1.0f, 0.0f, 0.2f * Alpha));
			Graphics()->DrawCircle(Position.x, Position.y, RadiusInner, 20);
			Graphics()->DrawCircle(Position.x, Position.y, RadiusOuter, 20);
			Graphics()->QuadsEnd();
			IEngineGraphics::CLineItem aLines[] = {
				{Position.x, Position.y - RadiusOuter, Position.x, Position.y + RadiusOuter},
				{Position.x - RadiusOuter, Position.y, Position.x + RadiusOuter, Position.y},
			};
			Graphics()->LinesBegin();
			Graphics()->SetColor(ColorRGBA(1.0f, 0.0f, 0.0f, 0.8f * Alpha));
			Graphics()->LinesDraw(aLines, std::size(aLines));
			Graphics()->LinesEnd();
		};

		for(const auto &Player : GameClient()->m_aClients)
		{
			const int ClientId = Player.ClientId();
			const auto &Char = GameClient()->m_Snap.m_aCharacters[ClientId];
			if(!Char.m_Active || !Player.m_Active)
				continue;
			if(Player.m_Team < 0)
				continue;

			if(!(in_range(Player.m_RenderPos.x, ScreenX0, ScreenX1) && in_range(Player.m_RenderPos.y, ScreenY0, ScreenY1)))
				continue;

			float Alpha = 1.0f;
			if(GameClient()->IsOtherTeam(ClientId))
				Alpha *= (float)g_Config.m_ClShowOthersAlpha / 100.0f;

			RenderHitbox(Player.m_RenderPos, Alpha);

			if(g_Config.m_TcShowPlayerHitBoxes > 1)
			{
				// From CPlayers::RenderPlayer
				vec2 ShadowPosition = mix(
					vec2(GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev.m_X, GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev.m_Y),
					vec2(GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur.m_X, GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur.m_Y),
					Client()->IntraGameTick(g_Config.m_ClDummy));
				RenderHitbox(ShadowPosition, Alpha * 0.75f);
			}
		}
	}
}

void CMod::RenderWeaponCollision()
{
	if(!g_Config.m_RcShowWeaponCollision || !GameClient()->m_Controls.m_aShowHookColl[g_Config.m_ClDummy] ||
		!GameClient()->m_Snap.m_pLocalCharacter || GameClient()->m_Snap.m_SpecInfo.m_Active)
		return;

	const int OwnerId = GameClient()->m_Snap.m_LocalClientId;
	CCharacter *pOwner = GameClient()->m_PredictedWorld.GetCharacterById(OwnerId);
	const int Weapon = pOwner ? pOwner->GetActiveWeapon() : GameClient()->m_Snap.m_pLocalCharacter->m_Weapon;
	if(Weapon != WEAPON_SHOTGUN && Weapon != WEAPON_GRENADE && Weapon != WEAPON_LASER)
		return;

	const CNetObj_PlayerInput &Input = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy];
	vec2 Aim(Input.m_TargetX, Input.m_TargetY);
	if(Aim == vec2(0.0f, 0.0f))
		Aim = vec2(0.0f, -1.0f);
	const vec2 Direction = normalize(Aim);
	const vec2 CharacterPos = GameClient()->m_LocalCharacterPos;
	const int TuneZone = Collision()->IsTune(Collision()->GetMapIndex(CharacterPos));
	const CTuningParams *pTuning = pOwner ? &pOwner->Core()->m_Tuning : GameClient()->GetTuning(TuneZone);
	std::vector<IGraphics::CLineItem> vLines;

	if(Weapon == WEAPON_GRENADE)
	{
		const vec2 StartPos = CharacterPos + Direction * (CCharacterCore::PhysicalSize() * 0.75f);
		vec2 PreviousPos = StartPos;
		const float Step = 1.0f / Client()->GameTickSpeed();
		for(float Time = Step; Time <= pTuning->m_GrenadeLifetime; Time += Step)
		{
			const vec2 Pos = CalcPos(StartPos, Direction, pTuning->m_GrenadeCurvature, pTuning->m_GrenadeSpeed, Time);
			vec2 CollisionPos = Pos;
			const int HitWall = Collision()->IntersectLine(PreviousPos, Pos, &CollisionPos, nullptr);
			vec2 TeeHitPos;
			CCharacter *pHitCharacter = nullptr;
			if(pOwner && !pOwner->GrenadeHitDisabled())
				pHitCharacter = GameClient()->m_PredictedWorld.IntersectCharacter(PreviousPos, CollisionPos, 6.0f, TeeHitPos, pOwner, OwnerId);
			if(pHitCharacter)
			{
				vLines.emplace_back(PreviousPos.x, PreviousPos.y, TeeHitPos.x, TeeHitPos.y);
				break;
			}
			vLines.emplace_back(PreviousPos.x, PreviousPos.y, CollisionPos.x, CollisionPos.y);
			if(HitWall)
				break;
			PreviousPos = Pos;
		}
	}
	else
	{
		vec2 Pos = CharacterPos;
		vec2 LaserDirection = Direction;
		float Energy = pTuning->m_LaserReach;
		for(int Bounce = 0; Bounce <= pTuning->m_LaserBounceNum && Energy > 0.0f; Bounce++)
		{
			vec2 CollisionTile;
			vec2 EndPos = Pos + LaserDirection * Energy;
			const int Hit = Collision()->IntersectLineTeleWeapon(Pos, EndPos, &CollisionTile, &EndPos);
			vec2 TeeHitPos;
			CCharacter *pHitCharacter = nullptr;
			const bool HitCharacters = pOwner && (Weapon == WEAPON_LASER ? !pOwner->LaserHitDisabled() : !pOwner->ShotgunHitDisabled());
			if(HitCharacters)
				pHitCharacter = GameClient()->m_PredictedWorld.IntersectCharacter(Pos, EndPos, 0.0f, TeeHitPos, pOwner, OwnerId);
			if(pHitCharacter)
			{
				vLines.emplace_back(Pos.x, Pos.y, TeeHitPos.x, TeeHitPos.y);
				break;
			}
			vLines.emplace_back(Pos.x, Pos.y, EndPos.x, EndPos.y);
			if(!Hit)
				break;

			vec2 BouncePos = EndPos;
			vec2 BounceVelocity = LaserDirection * 4.0f;
			Collision()->MovePoint(&BouncePos, &BounceVelocity, 1.0f, nullptr);
			Energy -= distance(Pos, BouncePos) + pTuning->m_LaserBounceCost;
			Pos = BouncePos;
			LaserDirection = normalize(BounceVelocity);
		}
	}

	if(vLines.empty())
		return;

	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(Weapon == WEAPON_GRENADE ? ColorRGBA(1.0f, 0.55f, 0.15f, 0.85f) : Weapon == WEAPON_SHOTGUN ? ColorRGBA(0.2f, 0.85f, 1.0f, 0.85f) : ColorRGBA(1.0f, 0.2f, 0.35f, 0.85f));
	Graphics()->LinesDraw(vLines.data(), vLines.size());
	Graphics()->LinesEnd();
}

void CMod::OnStateChange(int OldState, int NewState)
{
	m_ModWeaponActiveId = -1;
	m_ModWeaponActiveTimeLeft = -1.0f;
}

void CMod::ModWeapon(int Id)
{
	char aBuf[256];

	const auto &Player = GameClient()->m_aClients[Id];
	if(!Player.m_Active)
		return;

	str_format(aBuf, sizeof(aBuf), TCLocalize("Activating mod weapon on %d: %s\n"), Player.ClientId(), Player.m_aName);
	GameClient()->Echo(aBuf);

	str_format(aBuf, sizeof(aBuf), "%s %d", g_Config.m_TcModWeaponCommand, Id);
	Console()->ExecuteLine(aBuf, IConsole::CLIENT_ID_UNSPECIFIED);
}

void CMod::OnFire(bool Pressed)
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return;
	if(!Pressed)
	{
		m_ModWeaponActiveId = -1;
		return;
	}
	if(m_ModWeaponActiveId >= 0)
		return;
	if(g_Config.m_TcModWeapon == 0)
		return;
	if(!Client()->RconAuthed())
		return;
	const auto &Player = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId];
	if(!Player.m_Active)
		return;
	// Find person who we have shot
	const CGameClient::CClientData *pBestClient = nullptr;
	float BestClientScore = -INFINITY;
	if(GameClient()->m_Snap.m_SpecInfo.m_Active || Player.m_Team == TEAM_SPECTATORS)
	{
		const vec2 Pos = GameClient()->m_Camera.m_Center;
		for(const CGameClient::CClientData &Other : GameClient()->m_aClients)
		{
			if(!Other.m_Active || !GameClient()->m_Snap.m_aCharacters[Other.ClientId()].m_Active)
				continue;
			const float PosDelta = distance(Other.m_RenderPos, Pos);
			const float MaxRange = 250.0f;
			if(PosDelta > MaxRange)
				continue;
			const float Score = MaxRange - PosDelta;
			if(Score > BestClientScore)
			{
				BestClientScore = Score;
				pBestClient = &Other;
			}
		}
	}
	else
	{
		const vec2 Pos = Player.m_RenderPos;
		const vec2 Angle = normalize(GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy]);
		for(const CGameClient::CClientData &Other : GameClient()->m_aClients)
		{
			if(!Other.m_Active || !GameClient()->m_Snap.m_aCharacters[Other.ClientId()].m_Active || Player.ClientId() == Other.ClientId() || GameClient()->IsOtherTeam(Other.ClientId()))
				continue;
			const float PosDelta = distance(Other.m_RenderPos, Pos);
			const float MaxRange = 750.0f;
			if(PosDelta > MaxRange)
				continue;
			const float AngleDelta = dot(normalize(Other.m_RenderPos - Pos), Angle);
			if(AngleDelta < 0.9f)
				continue;
			const float Score = (AngleDelta - 1.0f) * 10.0f * MaxRange + (MaxRange - PosDelta);
			if(Score > BestClientScore)
			{
				BestClientScore = Score;
				pBestClient = &Other;
			}
		}
	}
	if(!pBestClient)
		return;

	m_ModWeaponActiveId = pBestClient->ClientId();
	m_ModWeaponActiveTimeLeft = MOD_WEAPON_TIME;
}
