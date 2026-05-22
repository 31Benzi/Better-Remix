#pragma once

constexpr auto bProd = false;
inline auto Port = 7777;
inline std::wstring GameserverIP;
inline std::string Region = "EU";
inline std::wstring Playlist = L"/BlastBerry/Playlists/Playlist_SunflowerSolo.Playlist_SunflowerSolo";
inline bool bInit = false;
inline bool bReady = false;
inline bool bEnableZones = true;
inline bool bEvent = false;
inline easywsclient::WebSocket* gsSocket = nullptr;

// GUI Configurations
inline bool bGUI = true;
inline bool bLateGame = false;
inline bool bInfiniteMats = false;
inline bool bInfiniteAmmo = false;
inline bool bKeepInventory = false;
inline bool bNoFallDamage = false;
inline bool bStartBusEarlyRequest = false;
inline int SiphonAmount = 50;
inline int MaxTickRate = 30;

// L"/QuailPlaylist/Playlist/Playlist_Quail.Playlist_Quail" / Remix: The Finale (Live Event)
// L"/BlastBerry/Playlists/Playlist_SunflowerSolo.Playlist_SunflowerSolo" / Fortnite Reload - Solos (Venture Map)
// L"/BlastBerry/Playlists/Playlist_PunchBerrySolo.Playlist_PunchBerrySolo" / Fortnite Reload - Solos (Oasis Map)
// L"/Rumble/Playlists/Playlist_Respawn_24.Playlist_Respawn_24" / Team Rumble
// L"/BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo" / Standard Battle Royale - Solos


