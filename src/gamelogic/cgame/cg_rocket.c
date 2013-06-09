/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "cg_local.h"

rocketInfo_t rocketInfo;

vmCvar_t rocket_menuFiles;

typedef struct
{
	vmCvar_t   *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int        cvarFlags;
} cvarTable_t;

static const cvarTable_t rocketCvarTable[] =
{
	{ &rocket_menuFiles, "rocket_menuFiles", "ui/rocket.txt", CVAR_ARCHIVE }
};

static const size_t rocketCvarTableSize = ARRAY_LEN( rocketCvarTable );

/*
=================
CG_RegisterRocketCvars
=================
*/
void CG_RegisterRocketCvars( void )
{
	int         i;
	const cvarTable_t *cv;

	for ( i = 0, cv = rocketCvarTable; i < rocketCvarTableSize; i++, cv++ )
	{
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
							cv->defaultString, cv->cvarFlags );
	}
}

void CG_Rocket_Init( void )
{
	int i, len;
	char *token, *text_p;
	char text[ 20000 ];
	fileHandle_t f;

	// Version check...
	trap_SyscallABIVersion( SYSCALL_ABI_VERSION_MAJOR, SYSCALL_ABI_VERSION_MINOR );

	// Init Rocket
	trap_Rocket_Init();

	// Dynamic memory
	BG_InitMemory();

	// load overrides
	BG_InitAllConfigs();

	BG_InitAllowedGameElements();


	// rocket cvars
	CG_RegisterRocketCvars();


	// Intialize data sources...
	CG_Rocket_RegisterDataSources();
	CG_Rocket_RegisterDataFormatters();

	// Register elements
	CG_Rocket_RegisterElements();

	rocketInfo.rocketState = IDLE;

	// Preload all the menu files...
	len = trap_FS_FOpenFile( rocket_menuFiles.string, &f, FS_READ );

	if ( len <= 0 )
	{
		Com_Error( ERR_DROP, "Unable to load %s. No rocket menus loaded.", rocket_menuFiles.string );
	}

	if ( len >= sizeof( text ) - 1 )
	{
		trap_FS_FCloseFile( f );
		Com_Error( ERR_DROP, "File %s too long.", rocket_menuFiles.string );
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	text_p = text;
	trap_FS_FCloseFile( f );

	// Parse files to load...
	while ( 1 )
	{
		token = COM_Parse2( &text_p );

		// Closing bracket. EOF
		if ( !*token || *token == '}' )
		{
			break;
		}

		// Ignore opening bracket
		if ( *token == '{' )
		{
			continue;
		}

		// Set the cursor
		if ( !Q_stricmp( token, "cursor" ) )
		{
			token = COM_Parse2( &text_p );

			// Skip non-RML files
			if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
			{
				continue;
			}

			trap_Rocket_LoadCursor( token );
			continue;
		}

		if ( !Q_stricmp( token, "main" ) )
		{
			token = COM_Parse2( &text_p );

			// Skip non-RML files
			if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
			{
				continue;
			}

			trap_Rocket_LoadDocument( token );
			continue;
		}

		if ( !Q_stricmp( token, "root" ) )
		{
			token = COM_Parse( &text_p );

			Q_strncpyz( rocketInfo.rootDir, token, sizeof( rocketInfo.rootDir ) );
			continue;
		}

		if ( !Q_stricmp( token, "human_hud" ) )
		{
			const char *s, *ss;
			token = COM_Parse2( &text_p );

			// Skip non-RML files
			if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
			{
				continue;
			}

			token = COM_Parse2( &text_p );
			s = BG_strdup( token );
			token = COM_Parse2( &text_p );
			ss = BG_strdup( token );

			for ( i = WP_BLASTER; i < WP_GRENADE; ++i )
			{
				rocketInfo.hud[ i ].path = s;
				rocketInfo.hud[ i ].id = ss;
			}

			rocketInfo.hud[ WP_HBUILD ].path = s;
			rocketInfo.hud[ WP_HBUILD ].id = ss;
			continue;
		}

		if ( !Q_stricmp( token, "spectator_hud" ) )
		{
			const char *s, *ss;
			token = COM_Parse2( &text_p );

			// Skip non-RML files
			if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
			{
				continue;
			}

			s = BG_strdup( token );
			token = COM_Parse2( &text_p );
			ss = BG_strdup( token );

			for ( i = WP_NONE; i < WP_NUM_WEAPONS; ++i )
			{
				rocketInfo.hud[ i ].path = s;
				rocketInfo.hud[ i ].id = ss;
			}

			continue;
		}

		if ( !Q_stricmp( token, "alien_hud" ) )
		{
			const char *s, *ss;
			token = COM_Parse2( &text_p );

			// Skip non-RML files
			if ( Q_stricmp( token + strlen( token ) - 4, ".rml" ) )
			{
				continue;
			}

			s = BG_strdup( token );
			token = COM_Parse2( &text_p );
			ss = BG_strdup( token );

			for ( i = WP_ALEVEL0; i < WP_ALEVEL4; ++i )
			{
				rocketInfo.hud[ i ].path = s;
				rocketInfo.hud[ i ].id = ss;
			}

			rocketInfo.hud[ WP_ABUILD ].path = s;
			rocketInfo.hud[ WP_ABUILD2 ].path = s;
			rocketInfo.hud[ WP_ABUILD ].id = ss;
			rocketInfo.hud[ WP_ABUILD2 ].id = ss;
			continue;
		}

		for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; ++i )
		{
			Com_Printf( "%s_hud\n", BG_Weapon( i )->name );

			if ( !Q_stricmp( token, va( "%s_hud", BG_Weapon( i )->humanName ) ) )
			{
				token = COM_Parse( &text_p );
				rocketInfo.hud[ i ].path = BG_strdup( token );
				token = COM_Parse( &text_p );
				rocketInfo.hud[ i ].id = BG_strdup( token );
				continue;
			}
		}
	}

	if ( !rocketInfo.hud[ WP_NONE ].path || !rocketInfo.hud[ WP_NONE ].id )
	{
		Com_Error( ERR_DROP, "Default HUD not set." );
	}

	// Set default HUD for all weapons without a hud
	for ( i = 0; i <  WP_NUM_WEAPONS; ++i )
	{
		if ( !rocketInfo.hud[ i ].path || !rocketInfo.hud[ i ].id )
		{
			rocketInfo.hud[ i ] = rocketInfo.hud[ WP_NONE ];
		}
	}

	trap_Rocket_DocumentAction( "main", "open" );
}

int CG_StringToNetSource( const char *src )
{
	if ( !Q_stricmp( src, "local" ) )
	{
		return AS_LOCAL;
	}
	else if ( !Q_stricmp( src, "favorites" ) )
	{
		return AS_FAVORITES;
	}
	else
	{
		return AS_GLOBAL;
	}
}


void CG_Rocket_Frame( void )
{
	switch ( rocketInfo.rocketState )
	{
		case RETRIEVING_SERVERS:
			if ( trap_LAN_UpdateVisiblePings( rocketInfo.currentNetSrc ) )
			{
			}
			else
			{
				trap_Rocket_SetInnerRML( "serverbrowser", "status", "Updated" );
				rocketInfo.rocketState = IDLE;
			}

			break;

		case BUILDING_SERVER_INFO:
			CG_Rocket_BuildServerInfo();
			break;

		case LOADING:
			CG_Rocket_CleanUpServerList( NULL );
			trap_Rocket_DocumentAction( "", "close" );
			trap_Rocket_DocumentAction( "main", "close" );
			trap_Rocket_LoadDocument( "ui/connecting.rml" );
			trap_Rocket_DocumentAction( "connecting", "show" );
			break;

		case PLAYING:
			trap_Rocket_DocumentAction( "connecting", "close" );
			break;
	}

	CG_Rocket_ProcessEvents();
}

const char *CG_Rocket_GetTag()
{
	static char tag[ 100 ];

	trap_Rocket_GetElementTag( tag, sizeof( tag ) );

	return tag;
}

const char *CG_Rocket_GetAttribute( const char *name, const char *id, const char *attribute )
{
	static char buffer[ 1000 ];

	trap_Rocket_GetAttribute( name, id, attribute, buffer, sizeof( buffer ) );

	return buffer;
}

const char *CG_Rocket_QuakeToRML( const char *in )
{
	static char buffer[ MAX_STRING_CHARS ];
	trap_Rocket_QuakeToRML( in, buffer, sizeof( buffer ) );
	return buffer;
}

qboolean CG_Rocket_IsCommandAllowed( rocketElementType_t type )
{
	playerState_t *ps = &cg.predictedPlayerState;

	switch ( type )
	{
		case ELEMENT_ALL:
			return qtrue;

		case ELEMENT_LOADING:
			if ( rocketInfo.rocketState == LOADING )
			{
				return qtrue;
			}

			return qfalse;

		case ELEMENT_GAME:
			if ( rocketInfo.rocketState == PLAYING )
			{
				return qtrue;
			}

			return qfalse;

		case ELEMENT_ALIENS:
			if ( ps->stats[ STAT_TEAM ] == TEAM_ALIENS && ps->stats[ STAT_HEALTH ] > 0 && ps->weapon != WP_NONE )
			{
				return qtrue;
			}

			return qfalse;

		case ELEMENT_HUMANS:
			if ( ps->stats[ STAT_TEAM ] == TEAM_HUMANS && ps->stats[ STAT_HEALTH ] > 0 && ps->weapon != WP_NONE )
			{
				return qtrue;
			}

			return qfalse;

		case ELEMENT_BOTH:
			if ( ps->stats[ STAT_TEAM ] != TEAM_NONE && ps->stats[ STAT_HEALTH ] > 0 && ps->weapon != WP_NONE )
			{
				return qtrue;
			}

			return qfalse;
	}

	return qfalse;
}
