#include "cheat.h"
#include <cmath>

struct iovec g_remote[1024], g_local[1024];
struct cheat::GlowObjectDefinition_t g_glow[1024];

int count = 0;
//int fixBloom = 0;

QAngle RCSLastPunch;

bool tShoot = false;

using namespace std;

static xdo_t *xdo = xdo_new(NULL);

const double pi = 3.1415926535897;

float rainbowVal = 0.f;

void rainbowify(ColorRGBA& color)
{
	color.r = cos(rainbowVal) * .5f + .5f;
	color.g = cos(rainbowVal - 2.f * pi / 3.f) * .5f + .5f;
	color.b = cos(rainbowVal - 4.f * pi / 3.f) * .5f + .5f;
}

void cheat::GlowAndTrigger(double colors[12], bool fullBloom, bool healthBased, bool rainbow, bool visCheck, remote::Handle* csgo, remote::MapModuleMemoryRegion* client) 
{
	if (!csgo || !client)
		return;

	// Reset
	bzero(g_remote, sizeof(g_remote));
	bzero(g_local, sizeof(g_local));
	bzero(g_glow, sizeof(g_glow));

	cheat::CGlowObjectManager manager;
	
	if(rainbowVal < 999999.f)
		rainbowVal += 0.0005f;
	else
		rainbowVal = 0.f;

	if (!csgo->Read((void*) csgo->m_addressOfGlowPointer, &manager, sizeof(cheat::CGlowObjectManager))) 
	{
		Logger::error ("Cannot read Glow Address");
		throw 1;
		return;
	}

	size_t count = manager.m_GlowObjectDefinitions.Count;

	void* data_ptr = (void*) manager.m_GlowObjectDefinitions.DataPtr;

	if (!csgo->Read(data_ptr, g_glow, sizeof(cheat::GlowObjectDefinition_t) * count)) 
	{
		Logger::error ("Cannot read Glow Object");
		throw 1;
		return;
	}

	size_t writeCount = 0;
	unsigned long localPlayer = 0;
	unsigned int teamNumber = 0;

	csgo->Read((void*) csgo->m_addressOfLocalPlayer, &localPlayer, sizeof(long));

	if (localPlayer != 0) 
	{
		csgo->Read((void*) (localPlayer+0x128), &teamNumber, sizeof(int));
	}

	for (unsigned int i = 0; i < count; i++) 
	{
		if (g_glow[i].m_pEntity != NULL) 
		{
			Entity ent;
			bool bSpotted;

			if (csgo->Read(g_glow[i].m_pEntity, &ent, sizeof(Entity))) 
			{
				if (ent.m_iTeamNum != 2 && ent.m_iTeamNum != 3) 
				{
					g_glow[i].m_bRenderWhenOccluded = 0;
					g_glow[i].m_bRenderWhenUnoccluded = 0;
					continue;
				}
				
				if (g_glow[i].m_bRenderWhenOccluded == 1)
					continue;
				
				if(visCheck == 1)
				{
					// yes i know. it's a shit way to do a visibility check but we're external and i cant be arsed to do bsp parsing
					csgo->Read((void*) ((unsigned long) g_glow[i].m_pEntity + ent.m_bSpotted), &bSpotted, sizeof(bSpotted));
					
					if(bSpotted == 0)
						continue;
				}

				if (ent.m_iTeamNum == 2 || ent.m_iTeamNum == 3) 
				{
					if (teamNumber != ent.m_iTeamNum && ent.m_bDormant != 1)
					{	
						g_glow[i].m_bRenderWhenOccluded = 1;
						g_glow[i].m_bRenderWhenUnoccluded = 0;

						if (csgo->TriggerEnabled)
						{
							unsigned int inCrossID;
							unsigned int entityId = 0;
							bool isImmune;
							csgo->Read((void*) (localPlayer+0xBBB8), &inCrossID, sizeof(inCrossID));
							csgo->Read((void*) ((unsigned long) g_glow[i].m_pEntity + ent.m_bGunGameImmunity), &isImmune, sizeof(isImmune));
							if(inCrossID > 0 && !isImmune) // no unnecessary reads
							{
								csgo->Read((void*) ((unsigned long) g_glow[i].m_pEntity + 0x94), &entityId, sizeof(entityId));

								if(inCrossID == entityId)
								{
									xdo_mouse_down(xdo, CURRENTWINDOW, 1);
									xdo_mouse_up(xdo, CURRENTWINDOW, 1);
								}
							}
						}
						
						unsigned long clrRenderOrig;
						unsigned long clrRender = 0xff000000; // 0xAARRGGBB
						csgo->Read((void*) ((unsigned long) g_glow[i].m_pEntity + ent.m_clrRender), &clrRenderOrig, sizeof(clrRenderOrig));
						
						if(clrRender != clrRenderOrig)
							csgo->Write((void*) ((unsigned long) g_glow[i].m_pEntity + ent.m_clrRender), &clrRender, sizeof(clrRender));
						
						//cout << "clrRenderOrig: " << clrRenderOrig << endl;
						
						if(fullBloom == 0 || fullBloom == 1)
                        {
                        	/*if(fixBloom < 1000) // needs to be high-ish so it does it for all entities
                           	{
                            	g_glow[i].m_bFullBloomRender = 0;
                            	fixBloom++;
                            }
                            else*/

                            g_glow[i].m_bFullBloomRender = fullBloom;
                        }
						
						if(healthBased == 1)
						{
							g_glow[i].m_flGlowRed = (255 - 2.55 * ent.m_iHealth) / 255.0f;
							g_glow[i].m_flGlowGreen = (2.55 * ent.m_iHealth) / 255.0f;
							g_glow[i].m_flGlowBlue = 50 / 255;
						}
						else if(rainbow == 1)
						{
							ColorRGBA rainbowClr;
							
							rainbowClr.r = colors[0];
							rainbowClr.g = colors[1];
							rainbowClr.b = colors[2];
							
							rainbowify(rainbowClr);
							
							g_glow[i].m_flGlowRed = rainbowClr.r;
							g_glow[i].m_flGlowGreen = rainbowClr.g;
							g_glow[i].m_flGlowBlue = rainbowClr.b;
						}
						else
						{
							g_glow[i].m_flGlowRed = colors[0];
							g_glow[i].m_flGlowGreen = colors[1];
							g_glow[i].m_flGlowBlue = colors[2];
						}
						
						g_glow[i].m_flGlowAlpha = colors[3];
					}
				}
			}
			/*cout << "Entity ID " << i << " X coord: " << ent.m_vecOrigin.x << endl;
			cout << "Entity ID " << i << " Y coord: " << ent.m_vecOrigin.y << endl;
			cout << "Entity ID " << i << " Z coord: " << ent.m_vecOrigin.z << endl;*/
		}

		if (csgo->GlowEnabled) 
		{
			size_t bytesToCutOffEnd = sizeof(cheat::GlowObjectDefinition_t) - g_glow[i].writeEnd();
			size_t bytesToCutOffBegin = (size_t) g_glow[i].writeStart();
			size_t totalWriteSize = (sizeof(cheat::GlowObjectDefinition_t) - (bytesToCutOffBegin + bytesToCutOffEnd));

			g_remote[writeCount].iov_base =
				((uint8_t*) data_ptr + (sizeof(cheat::GlowObjectDefinition_t) * i)) + bytesToCutOffBegin;
			g_local[writeCount].iov_base = ((uint8_t*) &g_glow[i]) + bytesToCutOffBegin;
			g_remote[writeCount].iov_len = g_local[writeCount].iov_len = totalWriteSize;

			writeCount++;
		}
	}

	process_vm_writev(csgo->GetPid(), g_local, writeCount, g_remote, writeCount, 0);
}

void cheat::RCS(float sensitivity, float m_yaw, float m_pitch, remote::Handle* csgo, remote::MapModuleMemoryRegion* client)
{	
	if(!csgo || !client)
		return;
	
	if(!csgo->RCSEnabled)
		return;
	
	unsigned long localPlayer = 0;
	unsigned int ShotsFired;
	QAngle vecPunch;
	QAngle angle;
	
	csgo->Read((void*) csgo->m_addressOfLocalPlayer, &localPlayer, sizeof(long));
	
	if (localPlayer != 0) 
	{
		csgo->Read((void*) (localPlayer+0x3764), &vecPunch, sizeof(vecPunch));
		csgo->Read((void*) (localPlayer+0xAB90), &ShotsFired, sizeof(ShotsFired));
		//csgo->Read((void*) (localPlayer+0x16C), &oldAngle, sizeof(oldAngle)); // FIXME, need engine pointer to get ViewAngles from engine_client.so
	}
	else
		return;

	if (ShotsFired > 1)
	{
		QAngle NewPunch = { vecPunch.x - RCSLastPunch.x, vecPunch.y - RCSLastPunch.y, 0 };

		angle.x -= NewPunch.x * 1.688f;
		angle.y -= NewPunch.y * 1.895f;
		xdo_move_mouse_relative(xdo, (int) -(angle.y / (m_yaw * sensitivity)), (int) (angle.x / (m_pitch * sensitivity)));
	}

	RCSLastPunch = vecPunch;
}

void cheat::SpoofMusicKit(int MusicID, remote::Handle* csgo, remote::MapModuleMemoryRegion* client)
{
	if(!csgo || !client)
		return;

	if(!csgo->MusicKitChangerEnabled)
		return;
	
	unsigned int LocalPlayerIndex = 1; // TODO: GetLocalPlayerIndex from engine_client.so
	unsigned int originalMusicID;
	unsigned int spoofedMusicID = MusicID;
	
	//csgo->Read((void*) localPlayer, &LocalPlayerIndex, sizeof(LocalPlayerIndex)); // TODO: GetLocalPlayerIndex from engine_client.so
	
	if(!LocalPlayerIndex)
		return;
		
	csgo->Read((void*) (csgo->PlayerResourcesPointer), &csgo->m_addressOfPlayerResource, sizeof(unsigned long));
		
	csgo->Read((void*) (csgo->m_addressOfPlayerResource + 0x4FDC + (LocalPlayerIndex * 4)), &originalMusicID, sizeof(spoofedMusicID));
	if(csgo->m_addressOfPlayerResource)
	{
		if(originalMusicID != spoofedMusicID)
		{
			csgo->Write((void*) (csgo->m_addressOfPlayerResource + 0x4FDC + (LocalPlayerIndex * 4)), &spoofedMusicID, sizeof(spoofedMusicID));
			cout << "Changed music kit ID to " << dec << spoofedMusicID << " on address " << hex << csgo->m_addressOfPlayerResource << endl;
		}
	}
}

void cheat::FovChanger(int fov, remote::Handle* csgo, remote::MapModuleMemoryRegion* client)
{
	if(!csgo || !client)
		return;

	if(!csgo->FovChangerEnabled)
		return;
	
	unsigned long localPlayer = 0;
	bool isScoped;
	float fovRate = 0.000005f;
	
	csgo->Read((void*) csgo->m_addressOfLocalPlayer, &localPlayer, sizeof(long));
	
	if(localPlayer != 0)
	{
		csgo->Read((void*) (localPlayer+0x4144), &isScoped, sizeof(isScoped));
	}
	else
		return;
	
	if(!isScoped)
	{
		csgo->Write((void*) (localPlayer + 0x3738), &fovRate, sizeof(float));
		csgo->Write((void*) (localPlayer + 0x3998), &fov, sizeof(int));
		csgo->Write((void*) (localPlayer + 0x399C), &fov, sizeof(int));
		csgo->Write((void*) (localPlayer + 0x3AF4), &fov, sizeof(int));
	}
}