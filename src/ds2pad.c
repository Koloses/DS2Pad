/*
 * DS2Pad - XInput controller support for Dungeon Siege II / Broken World (DungeonSiege2.exe 2.30)
 *
 * Built as system\mss32.dll, a proxy that passes every Miles Sound System call on to the game's
 * original library (renamed mss32o.dll, same folder). Hooks PeekMessageA in the game's import
 * table to get a per-frame callback on the main thread, then drives the game through the
 * engine's own exported functions, a few patched call sites, and synthesized mouse/keyboard input.
 */
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#define BUILD 55

/* ------------------------------------------------------------------ types */
/* A position: coordinates inside one terrain piece plus that piece's identity (24 bytes, of which
   the first 4 are the id the engine compares). Always copied whole. */
typedef struct { float x, y, z, w; unsigned node; unsigned rest[5]; } SiegePos;
typedef struct { float x, y, z; } Vec3;
typedef struct { void **b, **e, **c; void *spare; } GopColl;
typedef struct { int l, t, r, b; } GRect;
typedef struct { int chore, block, stance, subanim, flags; float speed; } AnimReq;
typedef unsigned char bool8;
#define TC __attribute__((thiscall))

enum { JQ_ACTION = 1 };
enum { QP_CLEAR = 5 };
enum { AO_USER = 1 };

/* ------------------------------------------------------------------ XInput */
typedef struct { WORD buttons; BYTE lt, rt; SHORT lx, ly, rx, ry; } XGAMEPAD;
typedef struct { DWORD packet; XGAMEPAD pad; } XSTATE;
static DWORD (WINAPI *XGetState)(DWORD, XSTATE *);
enum {
    B_UP = 0x0001, B_DOWN = 0x0002, B_LEFT = 0x0004, B_RIGHT = 0x0008,
    B_START = 0x0010, B_BACK = 0x0020, B_L3 = 0x0040, B_R3 = 0x0080,
    B_LB = 0x0100, B_RB = 0x0200, B_A = 0x1000, B_B = 0x2000, B_X = 0x4000, B_Y = 0x8000,
    B_LT = 0x10000, B_RT = 0x20000,
    N_UP = 0x100000, N_DOWN = 0x200000, N_LEFT = 0x400000, N_RIGHT = 0x800000 /* left stick as dpad */
};

/* ------------------------------------------------------------------ log */
static char g_dir[MAX_PATH];    /* game folder (where the exe is): ini and log live here */
static char g_modDir[MAX_PATH]; /* folder this library was loaded from */
static char g_ini[MAX_PATH];
static int g_logLevel = 1;
static void logf_(const char *fmt, ...)
{
    /* the file stays open: opening it for every line cost up to a few milliseconds each time (virus
       scanners look at every open), which showed as hitches whenever the mod had something to say */
    static HANDLE h = INVALID_HANDLE_VALUE;
    static int tried;
    if (!g_logLevel) return;
    char line[1024], path[MAX_PATH];
    SYSTEMTIME st; GetLocalTime(&st);
    int n = snprintf(line, sizeof line, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap; va_start(ap, fmt); n += vsnprintf(line + n, sizeof line - n - 3, fmt, ap); va_end(ap);
    if (n > (int)sizeof line - 3) n = sizeof line - 3;
    line[n++] = '\r'; line[n++] = '\n';
    if (h == INVALID_HANDLE_VALUE && tried < 20) {
        tried++;
        for (int pass = 0; pass < 2 && h == INVALID_HANDLE_VALUE; pass++) {
            if (pass == 0) snprintf(path, sizeof path, "%sds2pad.log", g_dir);
            else { DWORD l = GetTempPathA(MAX_PATH - 16, path); lstrcpyA(path + l, "ds2pad.log"); }
            h = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, 0, NULL);
        }
    }
    if (h != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(h, line, n, &w, NULL); }
}

static int rd(const void *p, size_t n) { return p && !IsBadReadPtr(p, n); }

/* ------------------------------------------------------------------ sound library pass-through */
#include "proxy_gen.h"
extern void *px_tbl[PX_COUNT];
static HMODULE g_orig;
static int __cdecl px_missing(void) { return 0; }
void *__cdecl px_resolve(int i)
{
    if (!g_orig) {
        char p[MAX_PATH];
        snprintf(p, sizeof p, "%smss32o.dll", g_modDir);
        g_orig = LoadLibraryA(p);
        if (!g_orig) g_orig = LoadLibraryA("mss32o.dll");
        if (!g_orig) {
            logf_("cannot load the original sound library %s (error %lu)", p, GetLastError());
            MessageBoxA(NULL, "DS2Pad: mss32o.dll (the game's original mss32.dll, renamed) was not found next to mss32.dll.",
                        "DS2Pad", MB_OK | MB_ICONERROR);
            ExitProcess(1);
        }
    }
    void *f = (void *)GetProcAddress(g_orig, px_names[i]);
    if (!f) { logf_("sound library: function %s missing in mss32o.dll", px_names[i]); f = (void *)px_missing; }
    px_tbl[i] = f;
    return f;
}

/* ------------------------------------------------------------------ engine symbols */
static HMODULE g_exe;
static int g_missing;
static void *sym(const char *n)
{
    void *p = (void *)GetProcAddress(g_exe, n);
    if (!p) { g_missing++; logf_("missing export: %s", n); }
    return p;
}

static void *(*Server_Get)(void);
static void *(*GoDb_Get)(void);
static void *(*AIQuery_Get)(void);
static void *(*UIShell_Get)(void);
static void *(*AppModule_Get)(void);
static bool8 (TC *AppModule_IsUserPaused)(void *);
static int (TC *AppModule_GetCursorX)(void *);
static int (TC *AppModule_GetCursorY)(void *);
static void *(*UIGame_Get)(void);
static void *(*UICommands_Get)(void);
static void *(*WorldState_Get)(void);
static void *(*SiegeEngine_Get)(void);
static void *(*WorldPathFinder_Get)(void);
static void *(*MCP_Get)(void);
static void *(*World_Get)(void);

static void *(TC *Server_GetScreenParty)(void *);
static void *(TC *Server_GetScreenHero)(void *);
static void *(TC *GoDb_GetFocusGo)(void *);

static GopColl *(TC *Go_GetChildren)(void *);
static bool8 (TC *Go_IsSelected)(void *);
static void *(TC *Go_GetGoid)(void *);
static void *(TC *Go_GetMind)(void *);
static void *(TC *Go_GetPlacement)(void *);
static void *(TC *Go_GetBody)(void *);
static void *(TC *Go_GetAspect)(void *);
static int (TC *Go_GetLifeState)(void *);
static bool8 (TC *Go_IsItem)(void *);
static bool8 (TC *Go_IsInsideInventory)(void *);
static bool8 (TC *Go_IsActor)(void *);
static bool8 (TC *Go_IsUsable)(void *);
static bool8 (TC *Go_IsBreakable)(void *);
static bool8 (TC *Go_HasConversation)(void *);
static bool8 (TC *Go_HasStore)(void *);
static bool8 (TC *Go_IsScreenPartyMember)(void *);
static const char *(TC *Go_GetTemplateName)(void *);
static bool8 (*LS_IsAlive)(int);

static SiegePos *(TC *GoPlacement_GetPosition)(void *);
static float *(TC *GoPlacement_GetOrientation)(void *);
static void (TC *GoPlacement_RCSetPosition)(void *, const SiegePos *);
static void (TC *GoPlacement_SSetPosition)(void *, const SiegePos *, bool8);
static void (TC *GoPlacement_SSetOrientation)(void *, const float *);
static void (TC *GoPlacement_Get2DDirection)(void *, Vec3 *);
static void (*QuatFromDirection)(float *, const Vec3 *);

static bool8 (TC *GoMind_RSMove)(void *, const SiegePos *, int, int);
static void (TC *GoMind_RSStop)(void *, int);
static void (TC *GoMind_RSGet)(void *, void *, int, int);
static void (TC *GoMind_RSUse)(void *, void *, int, int);
static bool8 (TC *GoMind_RSAttack)(void *, void *, int, int);
static void (TC *GoMind_RSDrinkLife)(void *, int);
static void (TC *GoMind_RSDrinkMana)(void *, int);
static bool8 (TC *GoMind_GetEnemiesInSphere)(void *, float, GopColl *);
static void (TC *GoMind_RSFollow)(void *, void *, int, int, float);
static bool8 (TC *GoMind_IsEnemy)(void *, void *);
static int *(TC *GoMind_GetFrontJob)(void *, int);
static void (TC *GoMind_ClearQ)(void *, int);
static bool8 (TC *GoMind_IsLosClear)(void *, void *);
static float (TC *GoMind_GetWeaponRange)(void *);
static bool8 (TC *AIQuery_GetOccupantsInSphere)(void *, const SiegePos *, float, GopColl *);
static bool8 (TC *GoAspect_GetIsSelectable)(void *);
static bool8 (TC *GoAspect_GetIsVisible)(void *);
static char *(TC *GoAspect_GetAspectPtr)(void *);
static float (TC *GoAspect_GetCurrentLife)(void *);
static float (TC *GoAspect_GetMaxLife)(void *);
static float (TC *GoAspect_GetCurrentMana)(void *);
static float (TC *GoAspect_GetMaxMana)(void *);

static void (TC *UICommands_RSTalk)(void *, void *, void *, int);
static bool8 (TC *UIGame_SelectNextPlayer)(void *);
static bool8 (TC *UIGame_SelectLastPlayer)(void *);

static void *(TC *SiegeEngine_GetCamera)(void *);
static SiegePos *(TC *Cam_GetCameraSiegePos)(void *);
static SiegePos *(TC *Cam_GetTargetSiegePos)(void *);
static Vec3 *(TC *Cam_GetCameraPosition)(void *);
static Vec3 *(TC *Cam_GetTargetPosition)(void *);
static float *(TC *Cam_GetMatrixOrientation)(void *);
static void (*GetSiegeDifference)(Vec3 *, const SiegePos *, const SiegePos *);

static int (TC *WorldState_GetCurrentState)(void *);
static const char *(*WS_ToString)(int);
static const char *(*UIType_ToString)(int);
static const char *(*Jat_ToString)(int);
static const char *(*Chore_ToString)(int);
static const char *(*MCPReq_ToString)(int);
static const char *(*MCPRet_ToString)(int);
static int (TC *UIShell_GetScreenWidth)(void *);
static int (TC *UIShell_GetScreenHeight)(void *);
static int (TC *UIGridbox_GetBoxWidth)(void *);
static int (TC *UIGridbox_GetBoxHeight)(void *);
static int (TC *UIGridbox_GetColumns)(void *);
static int (TC *UIGridbox_GetRows)(void *);

static bool8 (TC *World_IsMultiPlayer)(void *);
static int (TC *MCP_MakeRequest)(void *, void *, int, const SiegePos *, float);
static bool8 (TC *MCP_Flush)(void *, void *, float);
static void (TC *GoBody_SAnimate)(void *, const AnimReq *);
static float (TC *GoBody_GetAvgMoveVelocity)(void *);
static int (TC *GoBody_GetPerm)(void *);
static bool8 (TC *GoBody_IsPositionWalkable)(void *, const SiegePos *);
static bool8 (TC *WPF_FindLinePath)(void *, const SiegePos *, int, const Vec3 *, float, SiegePos *);
static int (TC *Aspect_GetCurrentChore)(void *);
static int (TC *Aspect_GetNextChore)(void *);
static float (TC *Aspect_GetCurrentVelocity)(void *);

static int g_gameOK, g_uiOK, g_directOK;
static int JAT_FOLLOW = -1, JAT_FIDGET = -1, JAT_FACE = -1, JAT_STOP = -1, JAT_NONE = -1, JAT_STARTUP = -1, JAT_BRAIN = -1;
static int CHORE_DEFAULT = -1, CHORE_WALK = -1;
static int g_plApproach = -1;

static int enum_find(const char *(*ts)(int), const char *name)
{
    if (!ts) return -1;
    for (int i = -2; i < 160; i++) {
        const char *s = ts(i);
        if (s && !IsBadStringPtrA(s, 64) && !lstrcmpiA(s, name)) return i;
    }
    logf_("engine value %s not found", name);
    return -1;
}

static void resolve(void)
{
    g_exe = GetModuleHandleA(NULL);
#define S(var, name) var = sym(name)
    g_missing = 0;
    S(Server_Get, "?FUBI_GetClassSingleton@Server@@CAPAV1@XZ");
    S(GoDb_Get, "?FUBI_GetClassSingleton@GoDb@@CAPAV1@XZ");
    S(AIQuery_Get, "?FUBI_GetClassSingleton@AIQuery@@CAPAV1@XZ");
    S(UIGame_Get, "?FUBI_GetClassSingleton@UIGame@@CAPAV1@XZ");
    S(UICommands_Get, "?FUBI_GetClassSingleton@UICommands@@CAPAV1@XZ");
    S(SiegeEngine_Get, "?FUBI_GetClassSingleton@SiegeEngine@siege@@CAPAV12@XZ");
    S(Server_GetScreenParty, "?GetScreenParty@Server@@QAEPAVGo@@XZ");
    S(Server_GetScreenHero, "?GetScreenHero@Server@@QAEPAVGo@@XZ");
    S(GoDb_GetFocusGo, "?GetFocusGo@GoDb@@QBEPBUGoid_@@XZ");
    S(Go_GetChildren, "?GetChildren@Go@@QBEABUGopColl@@XZ");
    S(Go_IsSelected, "?IsSelected@Go@@QBE_NXZ");
    S(Go_GetGoid, "?GetGoid@Go@@QBEPBUGoid_@@XZ");
    S(Go_GetMind, "?GetMind@Go@@QAEPAVGoMind@@XZ");
    S(Go_GetPlacement, "?GetPlacement@Go@@QAEPAVGoPlacement@@XZ");
    S(Go_GetLifeState, "?GetLifeState@Go@@QBE?AW4eLifeState@@XZ");
    S(Go_IsItem, "?IsItem@Go@@QBE_NXZ");
    S(Go_IsInsideInventory, "?IsInsideInventory@Go@@QBE_NXZ");
    S(Go_IsActor, "?IsActor@Go@@QBE_NXZ");
    S(Go_IsUsable, "?IsUsable@Go@@QBE_NXZ");
    S(Go_HasConversation, "?HasConversation@Go@@QBE_NXZ");
    S(Go_HasStore, "?HasStore@Go@@QBE_NXZ");
    S(Go_IsScreenPartyMember, "?IsScreenPartyMember@Go@@QBE_NXZ");
    S(LS_IsAlive, "?IsAlive@@YA_NW4eLifeState@@@Z");
    S(GoPlacement_GetPosition, "?GetPosition@GoPlacement@@QBEABUSiegePos@@XZ");
    S(GoMind_RSMove, "?RSMove@GoMind@@QAE_NABUSiegePos@@W4eQPlace@@W4eActionOrigin@@@Z");
    S(GoMind_RSStop, "?RSStop@GoMind@@QAEXW4eActionOrigin@@@Z");
    S(GoMind_RSGet, "?RSGet@GoMind@@QAEXPBUGoid_@@W4eQPlace@@W4eActionOrigin@@@Z");
    S(GoMind_RSUse, "?RSUse@GoMind@@QAEXPBUGoid_@@W4eQPlace@@W4eActionOrigin@@@Z");
    S(GoMind_RSAttack, "?RSAttack@GoMind@@QAE_NPBUGoid_@@W4eQPlace@@W4eActionOrigin@@@Z");
    S(GoMind_RSDrinkLife, "?RSDrinkLifeHealingPotion@GoMind@@QAEXW4eActionOrigin@@@Z");
    S(GoMind_RSDrinkMana, "?RSDrinkManaHealingPotion@GoMind@@QAEXW4eActionOrigin@@@Z");
    S(GoMind_GetEnemiesInSphere, "?GetEnemiesInSphere@GoMind@@QBE_NMAAUGopColl@@@Z");
    GoMind_RSFollow = (void *)GetProcAddress(g_exe, "?RSFollow@GoMind@@QAEXPBUGoid_@@W4eQPlace@@W4eActionOrigin@@M@Z");
    S(GoMind_IsEnemy, "?IsEnemy@GoMind@@QBE_NPBVGo@@@Z");
    S(GoMind_GetFrontJob, "?GetFrontJob@GoMind@@QBEPAVJob@@W4eJobQ@@@Z");
    S(AIQuery_GetOccupantsInSphere, "?GetOccupantsInSphere@AIQuery@@QAE_NABUSiegePos@@MAAUGopColl@@@Z");
    S(UICommands_RSTalk, "?RSTalk@UICommands@@QAEXPAVGo@@0W4eActionOrigin@@@Z");
    S(UIGame_SelectNextPlayer, "?SelectNextPlayer@UIGame@@QAE_NXZ");
    S(UIGame_SelectLastPlayer, "?SelectLastPlayer@UIGame@@QAE_NXZ");
    S(SiegeEngine_GetCamera, "?GetCamera@SiegeEngine@siege@@QBEABVSiegeCamera@2@XZ");
    S(Cam_GetCameraSiegePos, "?GetCameraSiegePos@SiegeCamera@siege@@QBEABUSiegePos@@XZ");
    S(Cam_GetTargetSiegePos, "?GetTargetSiegePos@SiegeCamera@siege@@QBEABUSiegePos@@XZ");
    S(Cam_GetCameraPosition, "?GetCameraPosition@SiegeCamera@siege@@QBEABUvector_3@@XZ");
    S(Cam_GetTargetPosition, "?GetTargetPosition@SiegeCamera@siege@@QBEABUvector_3@@XZ");
    S(Cam_GetMatrixOrientation, "?GetMatrixOrientation@SiegeCamera@siege@@QAEABUmatrix_3x3@@XZ");
    S(GetSiegeDifference, "?GetSiegeDifference@@YAXAAUvector_3@@ABUSiegePos@@1@Z");
    g_gameOK = (g_missing == 0);

#define O(var, name) var = (void *)GetProcAddress(g_exe, name)
    O(Go_GetBody, "?GetBody@Go@@QAEPAVGoBody@@XZ");
    O(Go_GetAspect, "?GetAspect@Go@@QAEPAVGoAspect@@XZ");
    O(Go_IsBreakable, "?IsBreakable@Go@@QBE_NXZ");
    O(Go_GetTemplateName, "?GetTemplateName@Go@@QBEPBDXZ");
    O(GoPlacement_GetOrientation, "?GetOrientation@GoPlacement@@QBEABUQuat@@XZ");
    O(GoPlacement_RCSetPosition, "?RCSetPosition@GoPlacement@@QAEXABUSiegePos@@@Z");
    O(GoPlacement_SSetPosition, "?SSetPosition@GoPlacement@@QAEXABUSiegePos@@_N@Z");
    O(GoPlacement_SSetOrientation, "?SSetOrientation@GoPlacement@@QAEXABUQuat@@@Z");
    O(GoPlacement_Get2DDirection, "?Get2DDirection@GoPlacement@@QBEXAAUvector_3@@@Z");
    O(QuatFromDirection, "?QuatFromDirection@Math@@YAXAAUQuat@@ABUvector_3@@@Z");
    O(GoMind_ClearQ, "?Clear@GoMind@@QAEXW4eJobQ@@@Z");
    O(GoMind_IsLosClear, "?IsLosClear@GoMind@@QBE_NPAVGo@@@Z");
    O(GoMind_GetWeaponRange, "?GetWeaponRange@GoMind@@QBEMXZ");
    O(GoAspect_GetIsSelectable, "?GetIsSelectable@GoAspect@@QBE_NXZ");
    O(GoAspect_GetIsVisible, "?GetIsVisible@GoAspect@@QBE_NXZ");
    O(GoAspect_GetAspectPtr, "?GetAspectPtr@GoAspect@@QBEPAVAspect@nema@@XZ");
    O(GoAspect_GetCurrentLife, "?GetCurrentLife@GoAspect@@QBEMXZ");
    O(GoAspect_GetMaxLife, "?GetMaxLife@GoAspect@@QBEMXZ");
    O(GoAspect_GetCurrentMana, "?GetCurrentMana@GoAspect@@QBEMXZ");
    O(GoAspect_GetMaxMana, "?GetMaxMana@GoAspect@@QBEMXZ");
    O(World_Get, "?FUBI_GetClassSingleton@World@@CAPAV1@XZ");
    O(World_IsMultiPlayer, "?IsMultiPlayer@World@@QBE_NXZ");
    O(MCP_Get, "?FUBI_GetClassSingleton@Manager@MCP@@CAPAV12@XZ");
    O(MCP_MakeRequest, "?MakeRequest@Manager@MCP@@QAE?AW4eReqRetCode@2@PBUGoid_@@W4eRequest@2@ABUSiegePos@@M@Z");
    O(MCP_Flush, "?Flush@Manager@MCP@@QAE_NPBUGoid_@@M@Z");
    O(MCPReq_ToString, "?ToString@@YAPBDW4eRequest@MCP@@@Z");
    O(MCPRet_ToString, "?ToString@@YAPBDW4eReqRetCode@MCP@@@Z");
    O(Jat_ToString, "?ToString@@YAPBDW4eJobAbstractType@@@Z");
    O(Chore_ToString, "?ToString@@YAPBDW4eAnimChore@@@Z");
    O(GoBody_SAnimate, "?SAnimate@GoBody@@QAEXABUAnimReq@@@Z");
    O(GoBody_GetAvgMoveVelocity, "?GetAvgMoveVelocity@GoBody@@QAEMXZ");
    O(GoBody_GetPerm, "?GetTerrainMovementPermissions@GoBody@@QBE?AW4eLogicalNodeFlags@siege@@XZ");
    O(GoBody_IsPositionWalkable, "?IsPositionWalkable@GoBody@@QBE_NABUSiegePos@@@Z");
    O(WorldPathFinder_Get, "?FUBI_GetClassSingleton@WorldPathFinder@@CAPAV1@XZ");
    O(WPF_FindLinePath, "?FindLinePath@WorldPathFinder@@QAE_NABUSiegePos@@W4eLogicalNodeFlags@siege@@ABUvector_3@@MAAU2@@Z");
    O(Aspect_GetCurrentChore, "?GetCurrentChore@Aspect@nema@@QBE?AW4eAnimChore@@XZ");
    O(Aspect_GetNextChore, "?GetNextChore@Aspect@nema@@QBE?AW4eAnimChore@@XZ");
    O(Aspect_GetCurrentVelocity, "?GetCurrentVelocity@Aspect@nema@@QBEMXZ");
#undef O
    JAT_FOLLOW = enum_find(Jat_ToString, "jat_follow");
    JAT_FIDGET = enum_find(Jat_ToString, "jat_fidget");
    JAT_FACE = enum_find(Jat_ToString, "jat_face");
    JAT_STOP = enum_find(Jat_ToString, "jat_stop");
    JAT_NONE = enum_find(Jat_ToString, "jat_none");
    JAT_STARTUP = enum_find(Jat_ToString, "jat_startup");
    JAT_BRAIN = enum_find(Jat_ToString, "jat_brain");
    CHORE_DEFAULT = enum_find(Chore_ToString, "chore_default");
    CHORE_WALK = enum_find(Chore_ToString, "chore_walk");
    g_plApproach = enum_find(MCPReq_ToString, "pl_approach");
    logf_("engine values: follow %d fidget %d face %d stop %d none %d | chore default %d walk %d | planner approach %d",
          JAT_FOLLOW, JAT_FIDGET, JAT_FACE, JAT_STOP, JAT_NONE, CHORE_DEFAULT, CHORE_WALK, g_plApproach);
    g_directOK = Go_GetBody && Go_GetAspect && GoPlacement_GetOrientation && GoPlacement_RCSetPosition &&
                 GoPlacement_SSetPosition && GoPlacement_SSetOrientation && QuatFromDirection && GoMind_ClearQ &&
                 GoAspect_GetAspectPtr && World_Get && World_IsMultiPlayer && MCP_Get && MCP_MakeRequest && MCP_Flush &&
                 MCPRet_ToString && GoBody_SAnimate && GoBody_GetAvgMoveVelocity && GoBody_GetPerm &&
                 GoBody_IsPositionWalkable && WorldPathFinder_Get && WPF_FindLinePath && Aspect_GetCurrentChore &&
                 Aspect_GetNextChore && Aspect_GetCurrentVelocity && CHORE_WALK >= 0 && CHORE_DEFAULT >= 0;
    logf_("direct control link: %d", g_directOK);

    g_missing = 0;
    S(WorldState_Get, "?FUBI_GetClassSingleton@WorldState@@CAPAV1@XZ");
    S(WorldState_GetCurrentState, "?GetCurrentState@WorldState@@QBE?AW4eWorldState@@XZ");
    S(WS_ToString, "?ToString@@YAPBDW4eWorldState@@@Z");
    S(UIShell_Get, "?FUBI_GetClassSingleton@UIShell@@CAPAV1@XZ");
    S(AppModule_Get, "?FUBI_GetClassSingleton@AppModule@@CAPAV1@XZ");
    AppModule_IsUserPaused = (void *)GetProcAddress(g_exe, "?IsUserPaused@AppModule@@QAE_NXZ");
    S(AppModule_GetCursorX, "?GetCursorX@AppModule@@QBEHXZ");
    S(AppModule_GetCursorY, "?GetCursorY@AppModule@@QBEHXZ");
    S(UIType_ToString, "?ToString@@YAPBDW4UI_CONTROL_TYPE@@@Z");
    S(UIShell_GetScreenWidth, "?GetScreenWidth@UIShell@@QBEHXZ");
    S(UIShell_GetScreenHeight, "?GetScreenHeight@UIShell@@QBEHXZ");
    S(UIGridbox_GetBoxWidth, "?GetBoxWidth@UIGridbox@@QBEHXZ");
    S(UIGridbox_GetBoxHeight, "?GetBoxHeight@UIGridbox@@QBEHXZ");
    S(UIGridbox_GetColumns, "?GetColumns@UIGridbox@@QBEHXZ");
    S(UIGridbox_GetRows, "?GetRows@UIGridbox@@QBEHXZ");
    g_uiOK = (g_missing == 0);
#undef S
    logf_("engine link: gameplay=%d ui=%d", g_gameOK, g_uiOK);
}

/* ------------------------------------------------------------------ config */
static float c_deadzone = 0.24f, c_moveDist = 6.0f, c_attackRadius = 10.0f, c_interactRadius = 6.0f;
static float c_cursorSpeed = 900.0f;
static int c_moveIntervalMs = 120, c_stickXSign = 0, c_navRepeatMs = 170, c_swapSticks = 0;
static int c_pad = -1, c_moveMode = 2;
static float c_speedScale = 1.0f, c_bodyRadius = 0.3f, c_actorSpacing = 1.2f;
static int c_shadowPlan = 1, c_shadowMs = 250;
static int g_walkCheck = 1;
static int c_autoPotion = 1;
static int c_potionShare, c_potionKeep;
static int c_pickGold = 1, c_pickPotions = 1;
static int c_autoRes;
static float c_pickRadius = 2.5f;
static float c_autoPotionPct = 30.0f, c_followDist = 3.5f;
static int c_follow = 1;

static char c_hud[2048];
static int c_pointerWake = 40;
static int c_framePacing = 1;
static float c_framePacingHz;
static int c_camLock = 1, c_hideCursor = 1, c_itemInfo = 1, c_compare = 1, c_infoX = 60, c_infoY = 45;
static int g_hideCursor;
static float c_uiScale = 1.5f;
static int c_uiScaleMinWidth = 1280;
static char c_uiNoScale[512];
static int k_inventory, k_map, k_pause, k_menu, k_loot, k_spellbook, k_journal, k_skills;
static int k_camLeft, k_camRight, k_camUp, k_camDown, k_zoomIn, k_zoomOut, k_power[4], k_mirror, k_rampage;

static float cfg_f(const char *key, float def)
{
    char buf[64], d[64];
    snprintf(d, sizeof d, "%g", def);
    GetPrivateProfileStringA("DS2Pad", key, d, buf, sizeof buf, g_ini);
    return (float)atof(buf);
}
static int cfg_i(const char *key, int def) { return (int)GetPrivateProfileIntA("DS2Pad", key, def, g_ini); }

static int parse_key(const char *s)
{
    static const struct { const char *n; int vk; } tab[] = {
        { "ESC", VK_ESCAPE }, { "ESCAPE", VK_ESCAPE }, { "TAB", VK_TAB }, { "SPACE", VK_SPACE },
        { "ENTER", VK_RETURN }, { "LEFT", VK_LEFT }, { "RIGHT", VK_RIGHT }, { "UP", VK_UP }, { "DOWN", VK_DOWN },
        { "COMMA", VK_OEM_COMMA }, { "PERIOD", VK_OEM_PERIOD }, { "MINUS", VK_OEM_MINUS }, { "EQUALS", VK_OEM_PLUS },
        { "SLASH", VK_OEM_2 }, { "LBRACKET", VK_OEM_4 }, { "RBRACKET", VK_OEM_6 }, { "PAGEUP", VK_PRIOR },
        { "PAGEDOWN", VK_NEXT }, { "HOME", VK_HOME }, { "END", VK_END }, { "NONE", 0 }
    };
    if (!s || !*s) return 0;
    for (unsigned i = 0; i < sizeof tab / sizeof tab[0]; i++)
        if (!lstrcmpiA(s, tab[i].n)) return tab[i].vk;
    if ((s[0] == 'F' || s[0] == 'f') && s[1] >= '1' && s[1] <= '9') return VK_F1 + atoi(s + 1) - 1;
    if (!s[1]) {
        char c = s[0];
        if (c >= 'a' && c <= 'z') c -= 32;
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return c;
    }
    return 0;
}
static int cfg_key(const char *key, const char *def)
{
    char buf[32];
    GetPrivateProfileStringA("Keys", key, def, buf, sizeof buf, g_ini);
    return parse_key(buf);
}

static void load_config(void)
{
    g_logLevel = cfg_i("Log", 1);
    c_deadzone = cfg_f("Deadzone", 0.24f);
    c_moveDist = cfg_f("MoveDistance", 6.0f);
    c_moveIntervalMs = cfg_i("MoveIntervalMs", 120);
    c_attackRadius = cfg_f("AttackRadius", 10.0f);
    c_interactRadius = cfg_f("InteractRadius", 6.0f);
    c_follow = cfg_i("PartyFollow", 1);
    c_followDist = cfg_f("FollowDistance", 3.5f);
    c_autoPotion = cfg_i("PartyAutoPotion", 1);
    c_autoPotionPct = cfg_f("PartyAutoPotionPercent", 30.0f);
    c_potionShare = cfg_i("PotionShare", 1);
    c_potionKeep = cfg_i("PotionShareKeep", 3);
    c_autoRes = cfg_i("AutoResurrect", 1);
    c_pickGold = cfg_i("AutoPickupGold", 1);
    c_pickPotions = cfg_i("AutoPickupPotions", 1);
    c_pickRadius = cfg_f("AutoPickupRadius", 2.5f);
    c_stickXSign = cfg_i("StickXSign", 0);
    c_cursorSpeed = cfg_f("CursorSpeed", 900.0f);
    c_navRepeatMs = cfg_i("NavRepeatMs", 170);
    c_swapSticks = cfg_i("SwapSticks", 0);
    c_pad = cfg_i("Controller", -1);
    c_moveMode = cfg_i("MoveMode", 2);
    c_speedScale = cfg_f("WalkSpeedScale", 1.0f);
    c_bodyRadius = cfg_f("BodyRadius", 0.3f);
    g_walkCheck = cfg_i("WalkCheck", 1);
    c_actorSpacing = cfg_f("EnemySpacing", 1.2f);
    c_shadowPlan = cfg_i("TriggerShadowPath", 1);
    c_shadowMs = cfg_i("TriggerShadowMs", 250);
    c_camLock = cfg_i("CameraLockToLeader", 1);
    c_hideCursor = cfg_i("HideCursorWithPad", 1);
    c_pointerWake = cfg_i("PointerWakePixels", 40);
    c_framePacing = cfg_i("FramePacing", 1);
    c_framePacingHz = cfg_f("FramePacingHz", 0.0f);
    c_itemInfo = cfg_i("GroundItemInfo", 1);
    c_compare = cfg_i("CompareWithEquipped", 1);
    c_infoX = cfg_i("GroundItemInfoX", 60);
    c_infoY = cfg_i("GroundItemInfoY", 45);
    c_uiScale = cfg_f("UIScale", 1.5f);
    c_uiScaleMinWidth = cfg_i("UIScaleMinWidth", 1280);
    GetPrivateProfileStringA("DS2Pad", "UIScaleExclude", "member_labels,tombstone_labels,ui_item_overlay,status_bars",
                             c_uiNoScale + 1, sizeof c_uiNoScale - 3, g_ini);
    c_uiNoScale[0] = ',';
    lstrcatA(c_uiNoScale, ",");
    GetPrivateProfileStringA("DS2Pad", "HudInterfaces",
        "character_awp,data_bar,field_commands,team_portraits,status_bars,power_bar,radar_overlay,console_output,"
        "game_console,game_console_mp,icons,member_labels,rollover_help,world_tip,nis_subtitle,enemy_status,"
        "tombstone_labels,quick_save,game_timer,jip_glass,load_bar,debug,generic_text,world_location_message,"
        "chapter_message,boss_message,teleport_notify,multiplayer_ranks,class_damage,camera_tutorial,tool_tip,"
        "fade_screen,cursors,common,pause,mini_inventories,character_header_bar",
        c_hud + 1, sizeof c_hud - 3, g_ini);
    c_hud[0] = ',';
    lstrcatA(c_hud, ",");
    k_inventory = cfg_key("Inventory", "I");
    k_map = cfg_key("Map", "TAB");
    k_pause = cfg_key("Pause", "SPACE");
    k_menu = cfg_key("Menu", "ESC");
    k_loot = cfg_key("CollectLoot", "Z");
    k_spellbook = cfg_key("SpellBook", "B");
    k_journal = cfg_key("Journal", "J");
    k_skills = cfg_key("Skills", "P");
    k_camLeft = cfg_key("CameraLeft", "LEFT");
    k_camRight = cfg_key("CameraRight", "RIGHT");
    k_camUp = cfg_key("CameraUp", "UP");
    k_camDown = cfg_key("CameraDown", "DOWN");
    k_zoomIn = cfg_key("ZoomIn", "EQUALS");
    k_zoomOut = cfg_key("ZoomOut", "MINUS");
    k_mirror = cfg_key("OrdersMirror", "F");
    k_rampage = cfg_key("OrdersRampage", "G");
    for (int i = 0; i < 4; i++) {
        char n[16], d[4];
        snprintf(n, sizeof n, "Power%d", i + 1);
        snprintf(d, sizeof d, "%d", i + 1);
        k_power[i] = cfg_key(n, d);
    }
}

/* ------------------------------------------------------------------ synthesized input */
static bool8 g_keyHeld[256];
static int g_mouseL, g_mouseR;
static HWND g_hwnd;

/* Synthetic key presses and mouse clicks are handed to a small worker thread. Injecting input can
   take 10 ms and more (every program with a keyboard or mouse hook gets to look at it first), and
   done on the game's own thread that was a visible hitch on every button press. */
static INPUT g_inQ[64];
static volatile LONG g_inHead, g_inTail;
static HANDLE g_inEvent;
static DWORD WINAPI input_worker(LPVOID p)
{
    (void)p;
    for (;;) {
        WaitForSingleObject(g_inEvent, INFINITE);
        while (g_inTail != g_inHead) {
            INPUT in = g_inQ[g_inTail & 63];
            g_inTail++;
            SendInput(1, &in, sizeof in);
        }
    }
    return 0;
}
static void input_post(const INPUT *in)
{
    if (!g_inEvent) {
        g_inEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
        HANDLE t = g_inEvent ? CreateThread(NULL, 0, input_worker, NULL, 0, NULL) : NULL;
        if (t) CloseHandle(t);
        else { g_inEvent = NULL; SendInput(1, (INPUT *)in, sizeof *in); return; }
    }
    if (g_inHead - g_inTail >= 60) { SendInput(1, (INPUT *)in, sizeof *in); return; } /* worker stuck: do it here */
    g_inQ[g_inHead & 63] = *in;
    g_inHead++;
    SetEvent(g_inEvent);
}
static void send_key(int vk, int down)
{
    if (vk <= 0 || vk > 255) return;
    INPUT in;
    memset(&in, 0, sizeof in);
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = (WORD)vk;
    in.ki.wScan = (WORD)MapVirtualKeyA(vk, 0);
    in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN || vk == VK_PRIOR || vk == VK_NEXT ||
        vk == VK_HOME || vk == VK_END)
        in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    input_post(&in);
}
static void hold_key(int vk, int down)
{
    if (vk <= 0 || vk > 255) return;
    if ((g_keyHeld[vk] != 0) == (down != 0)) return;
    g_keyHeld[vk] = (bool8)(down != 0);
    send_key(vk, down);
}
static void tap_key(int vk)
{
    if (vk <= 0 || vk > 255 || g_keyHeld[vk]) return;
    send_key(vk, 1);
    send_key(vk, 0);
}
static void mouse_button(int right, int down)
{
    int *st = right ? &g_mouseR : &g_mouseL;
    if (*st == (down != 0)) return;
    *st = (down != 0);
    INPUT in;
    memset(&in, 0, sizeof in);
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = right ? (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP)
                          : (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
    input_post(&in);
}
static void release_all(void)
{
    for (int i = 1; i < 256; i++)
        if (g_keyHeld[i]) hold_key(i, 0);
    mouse_button(0, 0);
    mouse_button(1, 0);
}

/* ------------------------------------------------------------------ UI scaler
   The interface is laid out for a smaller "virtual" screen (real size / scale), and every 2D quad
   drawn while an interface window is drawing is enlarged by the same factor as it goes into the
   renderer's vertex buffer. The 3D scene, and interfaces pinned to things in the 3D world (name
   labels, health bars over heads), are left alone. */
#define FN_UI_RESIZE  0x742a50 /* shell: resize every interface to a new screen size */
#define FN_UI_SETSIZE 0x7413e0 /* shell: remember the screen size */
#define FN_UI_MOUSE   0x748fe0 /* shell: mouse position update */
#define FN_TV_LOCK    0x510140 /* renderer: reserve room for screen-space vertices */
#define FN_TV_UNLOCK  0x510210 /* renderer: done writing them */
static float g_uiS = 1.0f; /* scale currently in effect (1 = off) */
static int g_inUiDraw;     /* a scaled interface window is drawing right now */
static int g_realW, g_realH, g_uiScaleHooked;
static void *g_tvPtr;
static int g_tvCount;
static bool8 (TC *g_origTvLock)(void *, int, int *, void **);
static void (TC *g_origTvUnlock)(void *);

static int code_is(DWORD addr, const char *hex)
{
    BYTE *p = (BYTE *)addr;
    size_t n = strlen(hex) / 2;
    if (!rd(p, n)) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned v = 0;
        sscanf(hex + i * 2, "%2x", &v);
        if (p[i] != (BYTE)v) return 0;
    }
    return 1;
}
static int write_code(DWORD addr, const BYTE *bytes, int n)
{
    DWORD old;
    if (!VirtualProtect((void *)addr, n, PAGE_EXECUTE_READWRITE, &old)) return 0;
    memcpy((void *)addr, bytes, n);
    VirtualProtect((void *)addr, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void *)addr, n);
    return 1;
}
/* Redirects a whole function: its first instructions (given as hex, whole instructions, at least 5
   bytes, nothing position-dependent) move to a small stub that then jumps on into the function. */
static void *hook_function(DWORD addr, const char *hex, void *hk)
{
    int n = (int)strlen(hex) / 2;
    BYTE jmp[16];
    if (n < 5 || n > 16 || !code_is(addr, hex)) return NULL;
    BYTE *t = VirtualAlloc(NULL, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!t) return NULL;
    memcpy(t, (void *)addr, n);
    t[n] = 0xe9;
    *(int *)(t + n + 1) = (int)(addr + n - (DWORD)(t + n + 5));
    memset(jmp, 0x90, sizeof jmp);
    jmp[0] = 0xe9;
    *(int *)(jmp + 1) = (int)((DWORD)hk - (addr + 5));
    if (!write_code(addr, jmp, n)) return NULL;
    return t;
}
/* The game writes its vertices into a buffer of the mod's (ordinary memory); on "done" they are
   copied, enlarged, into the real vertex buffer. The real buffer is write-only video memory - reading
   it back to change it in place is what made the first version of this slow. */
static DWORD g_tvTemp[0x4000 * 7];
static bool8 TC hk_tv_lock(void *rapi, int count, int *start, void **data)
{
    bool8 r = g_origTvLock(rapi, count, start, data);
    if (r && g_inUiDraw && g_uiS != 1.0f && data && *data && count > 0 && count <= 0x4000) {
        g_tvPtr = *data;
        g_tvCount = count;
        *data = g_tvTemp;
    } else g_tvPtr = NULL;
    return r;
}
static void TC hk_tv_unlock(void *rapi)
{
    if (g_tvPtr) {
        /* screen-space vertex: x, y, z, rhw, colour, u, v (28 bytes) */
        const float *src = (const float *)g_tvTemp;
        float *dst = g_tvPtr, s = g_uiS;
        for (int i = 0; i < g_tvCount; i++, src += 7, dst += 7) {
            dst[0] = (src[0] + 0.5f) * s - 0.5f;
            dst[1] = (src[1] + 0.5f) * s - 0.5f;
            dst[2] = src[2];
            dst[3] = src[3];
            dst[4] = src[4];
            dst[5] = src[5];
            dst[6] = src[6];
        }
        g_tvPtr = NULL;
    }
    g_origTvUnlock(rapi);
}
/* Is this window part of an interface that should stay unscaled? */
static int window_scaled(char *win)
{
    /* asked thousands of times a frame: the answer is remembered per interface name */
    static const char *seen[32];
    static char res[32];
    static int ns;
    const char *name = *(const char **)(win + 0x80);
    if (!name) return 1;
    for (int i = 0; i < 32; i++)
        if (seen[i] == name) return res[i];
    int r = 1;
    if (IsBadStringPtrA(name, 64)) return 1;
    for (const char *p = c_uiNoScale; *p; p++) {
        if (*p != ',') continue;
        const char *a = p + 1, *b = name;
        while (*a && *a != ',' && *a == *b) { a++; b++; }
        if (*a == ',' && *b == 0 && a != p + 1) { r = 0; break; }
    }
    seen[ns % 32] = name;
    res[ns % 32] = (char)r;
    ns++;
    return r;
}
/* Hiding the mouse pointer while the pad is in use in play. The game has three pointers and all three
   have to be dealt with:
   - its own, an ordinary interface window (interface "cursors"): simply not drawn (here);
   - Direct3D's, which the game switches on and off all the time: kept off (hk_D3DShowCursor);
   - Windows', which comes up whenever Windows asks the window what to show: answered with "none"
     (WM_SETCURSOR in the window procedure, and a check in the update). */
static int is_cursor_window(char *win)
{
    static const char *yes, *no[8];
    static int nn;
    const char *name = *(const char **)(win + 0x80);
    if (!name) return 0;
    if (name == yes) return 1;
    for (int i = 0; i < 8; i++) if (no[i] == name) return 0;
    if (!IsBadStringPtrA(name, 64) && !lstrcmpiA(name, "cursors")) { yes = name; return 1; }
    no[nn++ & 7] = name;
    return 0;
}
static void TC hk_window_draw(char *win, float alpha)
{
    if (g_hideCursor && is_cursor_window(win)) return;
    int prev = g_inUiDraw;
    g_inUiDraw = g_uiS != 1.0f && window_scaled(win);
    (*(void (TC **)(void *, float))(*(char **)win + 0x4c))(win, alpha);
    g_inUiDraw = prev;
}
/* The radar (mini map) is drawn by game code, not by an interface window, but inside the rectangle of
   one - so it is in the interface's virtual coordinates and has to be enlarged the same way. */
static void TC hk_radar_draw(void *radar)
{
    int prev = g_inUiDraw;
    g_inUiDraw = g_uiS != 1.0f;
    ((void (TC *)(void *))0x4bef0b)(radar);
    g_inUiDraw = prev;
}
static void ui_virtual_size(int *w, int *h)
{
    g_realW = *w;
    g_realH = *h;
    float sc = (c_uiScale > 1.01f && c_uiScale < 4.0f && *w >= c_uiScaleMinWidth) ? c_uiScale : 1.0f;
    if (sc != g_uiS) logf_("ui scale: %dx%d -> factor %.2f", *w, *h, sc);
    g_uiS = sc;
    if (sc != 1.0f) {
        *w = (int)(*w / sc + 0.5f);
        *h = (int)(*h / sc + 0.5f);
    }
}
static void TC hk_ui_resize(void *shell, int w, int h)
{
    ui_virtual_size(&w, &h);
    ((void (TC *)(void *, int, int))FN_UI_RESIZE)(shell, w, h);
}
static void TC hk_ui_setsize(void *shell, int w, int h)
{
    ui_virtual_size(&w, &h);
    ((void (TC *)(void *, int, int))FN_UI_SETSIZE)(shell, w, h);
}
static bool8 TC hk_ui_mouse(void *shell, int x, int y)
{
    if (g_uiS != 1.0f) {
        x = (int)(x / g_uiS);
        y = (int)(y / g_uiS);
    }
    return ((bool8 (TC *)(void *, int, int))FN_UI_MOUSE)(shell, x, y);
}
/* 3D views inside the interface (the character model in the inventory) are placed with a viewport
   taken from their window; while a scaled window is drawing, such a viewport is enlarged too. */
typedef struct { DWORD x, y, w, h; float minz, maxz; } D3DVP9;
typedef HRESULT (WINAPI *SetVP_t)(void *, const D3DVP9 *);
typedef HRESULT (WINAPI *Present_t)(void *, const void *, const void *, HWND, const void *);
typedef HRESULT (WINAPI *Clear_t)(void *, DWORD, const LONG *, DWORD, DWORD, float, DWORD);
typedef BOOL (WINAPI *ShowCur_t)(void *, BOOL);
typedef ULONG (WINAPI *Rel_t)(void *);
static struct { void **vt; SetVP_t orig; Present_t present; Clear_t clear; ShowCur_t showcur; Rel_t release; } g_devs[6];
static void *g_d3dDev;
static int g_d3dCursorWanted = -1; /* what the game last asked of Direct3D's own pointer (-1 = never asked) */
static int g_hideCursor;
static int g_ndevs;
/* Frame pacing. The pad update used to run whenever the game asked Windows for messages or a timer
   fired - on average once per frame, but not *exactly* once: some frames got two updates and some
   none, so a hero walked with the stick advanced unevenly although the frame rate was steady. The
   moment each frame is handed to the screen is now noted here, and the update runs once per frame,
   with that frame's real length as its time step. */
/* how evenly the game hands frames to the screen (for the log): a steady 60 a second can still be
   made of long and short frames in turn, which is seen as jitter */
static struct { int n, late, early; float sum, min, max, prev, swing; } g_ft;
static int g_shutdown;
static volatile LONG g_frameSeq;
static LONGLONG g_presentQpc;
static DWORD g_presentMs;
static HRESULT WINAPI hk_Present(void *dev, const void *a, const void *b, HWND w, const void *c)
{
    void **vt = *(void ***)dev;
    Present_t orig = g_devs[0].present;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt) orig = g_devs[i].present;
    g_d3dDev = dev;
    HRESULT r = orig(dev, a, b, w, c);
    LARGE_INTEGER q;
    if (c_framePacing && !g_shutdown) {
        /* Frame pacer. The game draws as fast as it can and leaves the timing to the display driver:
           finished frames are queued, and when the queue is full the game is held until it has room.
           On current systems that hold does not come once per frame but in lumps - the log showed two
           frames 5 ms apart followed by a 40 ms wait, over and over. The screen still shows 60 evenly
           spaced pictures a second, but they are snapshots of the game taken 5, 5 and 40 ms apart, so
           everything that moves judders. Here each frame is held until its own slot on an even
           schedule (a hair slower than the display, so the queue never fills). */
        static LONGLONG freq, due, period;
        LARGE_INTEGER f;
        if (!freq) {
            DEVMODEA dm;
            float hz = c_framePacingHz;
            QueryPerformanceFrequency(&f);
            freq = f.QuadPart ? f.QuadPart : 1;
            memset(&dm, 0, sizeof dm);
            dm.dmSize = sizeof dm;
            if (hz < 20.0f) hz = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency >= 30 ? (float)dm.dmDisplayFrequency : 60.0f;
            period = (LONGLONG)((double)freq / hz * 1.0008);
            timeBeginPeriod(1);
            logf_("frame pacing: frames are held to an even %.2f ms schedule (display %.0f Hz)", (double)period * 1000.0 / (double)freq, hz);
        }
        QueryPerformanceCounter(&q);
        /* A frame that comes late restarts the schedule from that moment: the frames after it are not
           allowed to hurry to make up for it, or the short-short-long pattern would simply carry on. */
        if (!due || q.QuadPart >= due || q.QuadPart < due - period * 2) due = q.QuadPart;
        else {
            while (q.QuadPart < due) {
                LONGLONG left = due - q.QuadPart;
                if (left * 1000 > freq * 2) Sleep(1); /* more than 2 ms to go */
                else YieldProcessor();
                QueryPerformanceCounter(&q);
            }
        }
        due += period;
    }
    QueryPerformanceCounter(&q);
    if (g_presentQpc) {
        static LONGLONG freq;
        if (!freq) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); freq = f.QuadPart ? f.QuadPart : 1; }
        float ms = (float)((double)(q.QuadPart - g_presentQpc) * 1000.0 / (double)freq);
        if (ms < 1000.0f) {
            g_ft.n++;
            g_ft.sum += ms;
            if (ms < g_ft.min || g_ft.n == 1) g_ft.min = ms;
            if (ms > g_ft.max) g_ft.max = ms;
            if (ms > 22.0f) g_ft.late++;
            if (ms < 11.0f) g_ft.early++;
            float d = ms - g_ft.prev;
            g_ft.swing += d < 0 ? -d : d;
            g_ft.prev = ms;
        }
    }
    g_presentQpc = q.QuadPart;
    g_presentMs = timeGetTime();
    g_frameSeq++;
    return r;
}
static HRESULT WINAPI hk_SetViewport(void *dev, const D3DVP9 *vp)
{
    void **vt = *(void ***)dev;
    SetVP_t orig = g_devs[0].orig;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt) orig = g_devs[i].orig;
    if (g_inUiDraw && g_uiS != 1.0f && rd(vp, sizeof *vp) && g_realW > 0) {
        D3DVP9 v = *vp;
        float r = (v.x + v.w) * g_uiS, b = (v.y + v.h) * g_uiS;
        if (r <= g_realW + 1.0f && b <= g_realH + 1.0f) {
            v.x = (DWORD)(v.x * g_uiS + 0.5f);
            v.y = (DWORD)(v.y * g_uiS + 0.5f);
            v.w = (DWORD)(v.w * g_uiS + 0.5f);
            v.h = (DWORD)(v.h * g_uiS + 0.5f);
            if (v.x + v.w > (DWORD)g_realW) v.w = g_realW - v.x;
            if (v.y + v.h > (DWORD)g_realH) v.h = g_realH - v.y;
            static int logs;
            if (logs < 6) { logs++; logf_("ui scale: 3D view %lu,%lu %lux%lu enlarged to %lu,%lu %lux%lu", vp->x, vp->y, vp->w, vp->h, v.x, v.y, v.w, v.h); }
            return orig(dev, &v);
        }
    }
    return orig(dev, vp);
}
/* A 3D view inside the interface with no backdrop picture of its own (the journal's map) is prepared
   by clearing its rectangle - depth included. That rectangle comes from the window as well, so while
   a scaled window is drawing it is enlarged like the viewport; otherwise only the top-left part of
   the enlarged view is cleared and the rest of the map is not drawn. */
static HRESULT WINAPI hk_Clear(void *dev, DWORD count, const LONG *rects, DWORD flags, DWORD color, float z, DWORD stencil)
{
    void **vt = *(void ***)dev;
    Clear_t orig = g_devs[0].clear;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt) orig = g_devs[i].clear;
    if (g_inUiDraw && g_uiS != 1.0f && count == 1 && rd(rects, 16) && g_realW > 0) {
        LONG r[4];
        r[0] = (LONG)(rects[0] * g_uiS + 0.5f);
        r[1] = (LONG)(rects[1] * g_uiS + 0.5f);
        r[2] = (LONG)(rects[2] * g_uiS + 0.5f);
        r[3] = (LONG)(rects[3] * g_uiS + 0.5f);
        if (r[2] <= g_realW + 1 && r[3] <= g_realH + 1) {
            static int logs;
            if (r[2] > g_realW) r[2] = g_realW;
            if (r[3] > g_realH) r[3] = g_realH;
            if (logs < 4) { logs++; logf_("ui scale: 3D view clear %ld,%ld-%ld,%ld enlarged to %ld,%ld-%ld,%ld", rects[0], rects[1], rects[2], rects[3], r[0], r[1], r[2], r[3]); }
            return orig(dev, 1, r, flags, color, z, stencil);
        }
    }
    return orig(dev, count, rects, flags, color, z, stencil);
}
/* Direct3D's own pointer: what the game asks for is remembered, and while the pad has the pointer
   hidden the answer is "off" whatever it asks. */
static BOOL WINAPI hk_D3DShowCursor(void *dev, BOOL show)
{
    void **vt = *(void ***)dev;
    ShowCur_t orig = g_devs[0].showcur;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt) orig = g_devs[i].showcur;
    g_d3dCursorWanted = show ? 1 : 0;
    return orig(dev, g_hideCursor ? FALSE : show);
}
/* The game lets go of its device when it changes resolution (it makes a new one). The mod forgets
   the device the moment it is gone, so it can never talk to a dead one. */
static ULONG WINAPI hk_DevRelease(void *dev)
{
    void **vt = *(void ***)dev;
    Rel_t orig = NULL;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt && g_devs[i].release) orig = g_devs[i].release;
    if (!orig) orig = g_devs[0].release;
    int mine = dev == g_d3dDev;
    ULONG r = orig(dev);
    if (mine && r == 0) g_d3dDev = NULL;
    return r;
}
static void d3d_cursor(BOOL show)
{
    if (!g_d3dDev) return;
    void **vt = *(void ***)g_d3dDev;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt && g_devs[i].showcur) g_devs[i].showcur(g_d3dDev, show);
}
/* The renderer's Direct3D device can be recreated, so this is re-checked every tick. */
static void ensure_d3d_hook(void)
{
    char *owner = *(char **)0xd0158c;
    if (!rd(owner, 0x250)) return;
    char *rapi = *(char **)(owner + 0x24c);
    if (!rd(rapi, 0x6c4)) return;
    void ***dev = *(void ****)(rapi + 0x6c0);
    if (!rd(dev, 4)) return;
    void **vt = *dev;
    if (!rd(vt, 0xc4) || vt[47] == (void *)hk_SetViewport || vt[17] == (void *)hk_Present) return;
    int slot = g_ndevs;
    for (int i = 0; i < g_ndevs; i++)
        if (g_devs[i].vt == vt) slot = i;
    if (slot >= 6) { static int next; slot = next++ % 6; } /* the game has made many devices: the oldest entry is reused */
    DWORD old;
    if (!VirtualProtect(&vt[47], 4, PAGE_READWRITE, &old)) {
        static int warned;
        if (!warned++) logf_("ui scale: cannot patch the Direct3D device table");
        return;
    }
    g_devs[slot].vt = vt;
    g_devs[slot].orig = (SetVP_t)vt[47];
    g_devs[slot].present = (Present_t)vt[17];
    g_devs[slot].clear = (Clear_t)vt[43];
    g_devs[slot].showcur = (ShowCur_t)vt[12];
    g_devs[slot].release = (Rel_t)vt[2];
    if (slot == g_ndevs && g_ndevs < 6) g_ndevs++;
    vt[47] = (void *)hk_SetViewport;
    VirtualProtect(&vt[47], 4, old, &old);
    if (VirtualProtect(&vt[17], 4, PAGE_READWRITE, &old)) {
        vt[17] = (void *)hk_Present;
        VirtualProtect(&vt[17], 4, old, &old);
    }
    if (VirtualProtect(&vt[43], 4, PAGE_READWRITE, &old)) {
        vt[43] = (void *)hk_Clear;
        VirtualProtect(&vt[43], 4, old, &old);
    }
    if (VirtualProtect(&vt[12], 4, PAGE_READWRITE, &old)) {
        vt[12] = (void *)hk_D3DShowCursor;
        VirtualProtect(&vt[12], 4, old, &old);
    }
    if (VirtualProtect(&vt[2], 4, PAGE_READWRITE, &old)) {
        vt[2] = (void *)hk_DevRelease;
        VirtualProtect(&vt[2], 4, old, &old);
    }
    logf_("Direct3D hooks installed: viewport, frame pacing %d (device table %p)", vt[17] == (void *)hk_Present, (void *)vt);
}
static int patch_call(DWORD site, DWORD oldTarget, void *newTarget);
static void install_ui_scale(void)
{
    static const struct { DWORD at; const char *expect; int kind; } draw[] = {
        { 0x741742, "8b1150ff524c", 0 },
        { 0x741788, "8b11680000803fff524c", 1 }, { 0x741656, "8b11680000803fff524c", 1 },
        { 0x73f498, "8b11680000803fff524c", 1 }, { 0x748ea7, "8b11680000803fff524c", 1 },
        { 0x74d602, "8b06518bceff504c", 2 }, { 0x74d637, "8b06518bceff504c", 2 },
    };
    static const DWORD calls[][2] = { { 0x42db58, FN_UI_RESIZE }, { 0x45e9e1, FN_UI_RESIZE }, { 0x74be70, FN_UI_SETSIZE }, { 0x74a57f, FN_UI_MOUSE } };
    int wantScale = c_uiScale > 1.01f;
    if ((!wantScale && !c_hideCursor) || !g_uiOK) { logf_("ui scale: off"); return; }
    if (!wantScale) {
        /* no scaling: only the window-draw sites are redirected, for hiding the pointer */
        int ok = 1;
        for (int i = 0; i < 7; i++)
            if (!code_is(draw[i].at, draw[i].expect)) { logf_("cursor hiding: draw site %d not as expected; disabled", i); return; }
        for (int i = 0; i < 7; i++) {
            BYTE b[10];
            int n = 0;
            DWORD at = draw[i].at;
            if (draw[i].kind == 0) b[n++] = 0x50;
            else if (draw[i].kind == 1) { b[n++] = 0x68; b[n++] = 0; b[n++] = 0; b[n++] = 0x80; b[n++] = 0x3f; }
            else { b[n++] = 0x51; b[n++] = 0x8b; b[n++] = 0xce; }
            b[n] = 0xe8;
            *(int *)(b + n + 1) = (int)((DWORD)hk_window_draw - (at + n + 5));
            ok &= write_code(at, b, n + 5);
        }
        logf_("ui scale: off; window draw hooks for hiding the pointer: %d", ok);
        return;
    }
    /* check everything first so the patch set is all-or-nothing */
    for (int i = 0; i < 7; i++)
        if (!code_is(draw[i].at, draw[i].expect)) { logf_("ui scale: draw site %d not as expected; disabled", i); return; }
    for (int i = 0; i < 4; i++) {
        BYTE *p = (BYTE *)calls[i][0];
        if (!rd(p, 5) || p[0] != 0xe8 || (DWORD)(calls[i][0] + 5 + *(int *)(p + 1)) != calls[i][1]) {
            logf_("ui scale: call site %d not as expected; disabled", i);
            return;
        }
    }
    if (!code_is(FN_TV_LOCK, "558bec568bf1") || !code_is(FN_TV_UNLOCK, "568bf18a863c070000") || !rd((void *)0xd0158c, 4)) {
        logf_("ui scale: renderer routines not as expected; disabled");
        return;
    }
    g_origTvLock = hook_function(FN_TV_LOCK, "558bec568bf1", hk_tv_lock);
    g_origTvUnlock = hook_function(FN_TV_UNLOCK, "568bf18a863c070000", hk_tv_unlock);
    if (!g_origTvLock || !g_origTvUnlock) { logf_("ui scale: could not hook the renderer (%p %p); disabled", (void *)g_origTvLock, (void *)g_origTvUnlock); return; }
    int ok = 1;
    for (int i = 0; i < 7; i++) {
        BYTE b[10];
        int n = 0;
        DWORD at = draw[i].at;
        if (draw[i].kind == 0) b[n++] = 0x50;                                              /* push eax */
        else if (draw[i].kind == 1) { b[n++] = 0x68; b[n++] = 0; b[n++] = 0; b[n++] = 0x80; b[n++] = 0x3f; } /* push 1.0f */
        else { b[n++] = 0x51; b[n++] = 0x8b; b[n++] = 0xce; }                              /* push ecx ; mov ecx, esi */
        b[n] = 0xe8;
        *(int *)(b + n + 1) = (int)((DWORD)hk_window_draw - (at + n + 5));
        ok &= write_code(at, b, n + 5);
    }
    ok &= patch_call(0x42db58, FN_UI_RESIZE, hk_ui_resize);
    ok &= patch_call(0x45e9e1, FN_UI_RESIZE, hk_ui_resize);
    ok &= patch_call(0x74be70, FN_UI_SETSIZE, hk_ui_setsize);
    ok &= patch_call(0x74a57f, FN_UI_MOUSE, hk_ui_mouse);
    g_uiScaleHooked = ok;
    logf_("ui scale: radar hook %d", patch_call(0x480f61, 0x4bef0b, hk_radar_draw));
    logf_("ui scale: hooks %s, factor %.2f for widths >= %d", ok ? "installed" : "FAILED", c_uiScale, c_uiScaleMinWidth);
    char *shell = UIShell_Get();
    if (ok && rd(shell, 0x90)) {
        /* the interface already exists: bring it to the scaled layout now if the screen is big enough */
        int w = UIShell_GetScreenWidth(shell), h = UIShell_GetScreenHeight(shell);
        g_realW = w;
        g_realH = h;
        if (w >= c_uiScaleMinWidth && h > 200 && w < 16384) hk_ui_resize(shell, w, h);
    }
}

/* ------------------------------------------------------------------ UI enumeration
   Layout (DungeonSiege2.exe 2.30):
     shell:     [4],[8] begin/end of the interface list, [0x78] screen width
     interface: [0] flags (bit 2: visible), [0xc] name, [0x28] map of its windows (name -> window)
     map:       [4] head; head[4] root; node: [0] left, [8] right, [0x10] window, byte [0x15] "nil"
     window:    [4] control type, [0x60] rectangle, [0x9c] name, byte [0xb0] visible, byte [0xe1] enabled */
enum { UK_NONE = 0, UK_POINT, UK_GRID, UK_LIST };
static signed char g_typeKind[96];

static int type_kind(int t)
{
    if (t < 0 || t >= 96) return UK_NONE;
    if (g_typeKind[t] >= 0) return g_typeKind[t];
    int k = UK_NONE;
    const char *n = UIType_ToString(t);
    if (n && !IsBadStringPtrA(n, 64)) {
        if (strstr(n, "gridbox")) k = UK_GRID;
        else if (strstr(n, "listbox") || strstr(n, "listreport")) k = UK_LIST;
        else if (strstr(n, "itemslot") || strstr(n, "infoslot") || strstr(n, "button") || strstr(n, "checkbox") ||
                 strstr(n, "slider") || strstr(n, "combo_box") || strstr(n, "edit_box") || strstr(n, "popupmenu"))
            k = UK_POINT;
        else {
            size_t l = strlen(n);
            if (l >= 3 && !strcmp(n + l - 3, "tab")) k = UK_POINT;
        }
    }
    g_typeKind[t] = (signed char)k;
    return k;
}

typedef struct { short x, y; const char *type; GRect rc; } NavPt;
#define MAX_PTS 1500
static NavPt g_pts[MAX_PTS];
static int g_npts;
static int g_uiW = 800, g_uiH = 600;

static const char *g_addType = "";
static GRect g_addRect;
static void add_pt(int x, int y)
{
    if (x < 1 || y < 1 || x >= g_uiW - 1 || y >= g_uiH - 1 || g_npts >= MAX_PTS) return;
    for (int i = g_npts - 1; i >= 0 && i > g_npts - 400; i--)
        if (abs(g_pts[i].x - x) < 6 && abs(g_pts[i].y - y) < 6) return;
    g_pts[g_npts].x = (short)x;
    g_pts[g_npts].y = (short)y;
    g_pts[g_npts].type = g_addType;
    g_pts[g_npts].rc = g_addRect;
    g_npts++;
}

static int g_winSeen;
static void add_window(char *w)
{
    if (!rd(w, 0xe4)) return;
    g_winSeen++;
    if (!w[0xb0] || !w[0xe1]) return; /* hidden or disabled */
    GRect *r = (GRect *)(w + 0x60);
    int W = r->r - r->l, H = r->b - r->t;
    if (W <= 3 || H <= 3 || W > g_uiW + 8 || H > g_uiH + 8) return;
    int ty = *(int *)(w + 4);
    int kind = type_kind(ty);
    if (kind == UK_NONE) return;
    g_addType = UIType_ToString(ty);
    g_addRect = *r;
    if (kind == UK_POINT) {
        add_pt((r->l + r->r) / 2, (r->t + r->b) / 2);
    } else if (kind == UK_GRID) {
        int bw = UIGridbox_GetBoxWidth(w), bh = UIGridbox_GetBoxHeight(w);
        int cols = UIGridbox_GetColumns(w), rows = UIGridbox_GetRows(w);
        if (bw < 8 || bw > 256) bw = 32;
        if (bh < 8 || bh > 256) bh = 32;
        if (cols < 1 || cols > 64) cols = W / bw;
        if (rows < 1 || rows > 64) rows = H / bh;
        for (int j = 0; j < rows && j < 64; j++)
            for (int i = 0; i < cols && i < 64; i++)
                add_pt(r->l + i * bw + bw / 2, r->t + j * bh + bh / 2);
    } else if (kind == UK_LIST) {
        int step = g_uiH >= 700 ? 22 : 18, rows = 0;
        void **vt = *(void ***)w;
        if (g_addType && !strcmp(g_addType, "ui_type_listbox") && rd(w, 0x17c) && rd(vt, 0x24)) {
            /* a list box knows its own rows: how many there are and how tall each one is. A short
               list (dialogue answers) may run past the box it was laid out with. */
            int n = ((int (TC *)(void *))vt[4])(w), eh = ((int (TC *)(void *))vt[8])(w);
            if (eh < 8) eh = *(int *)(w + 0x178);
            if (eh >= 8 && eh <= 80 && n > 0 && n < 500) {
                step = eh;
                if (n * eh <= H + 6 * eh) rows = n;
            }
        }
        if (rows) {
            for (int i = 0; i < rows; i++) add_pt((r->l + r->r) / 2, r->t + i * step + step / 2);
        } else {
            for (int y = r->t + step / 2; y < r->b; y += step)
                add_pt((r->l + r->r) / 2, y);
        }
    }
}

static int is_hud(const char *name)
{
    char key[80];
    snprintf(key, sizeof key, ",%s,", name);
    return strstr(c_hud, key) != NULL;
}

/* Collect interactive points. includeHud=0 skips HUD interfaces. Returns the number of non-HUD
   interfaces that are visible and contain something to press. */
static char g_menuNames[256];
static int collect_ui(int includeHud)
{
    g_npts = 0;
    g_menuNames[0] = 0;
    if (!g_uiOK) return 0;
    char *shell = UIShell_Get();
    if (!rd(shell, 0x90)) return 0;
    g_uiW = UIShell_GetScreenWidth(shell);
    g_uiH = UIShell_GetScreenHeight(shell);
    if (g_uiW < 320 || g_uiW > 16384 || g_uiH < 200 || g_uiH > 16384) return 0;
    char **b = *(char ***)(shell + 4), **e = *(char ***)(shell + 8);
    int n = (int)(e - b);
    if (n <= 0 || n > 1024 || !rd(b, n * sizeof(void *))) return 0;
    int menus = 0;
    for (int i = 0; i < n; i++) {
        char *iface = b[i];
        if (!rd(iface, 0x34) || !(iface[0] & 4)) continue;
        const char *name = *(const char **)(iface + 0xc);
        if (!name || IsBadStringPtrA(name, 64)) continue;
        int hud = is_hud(name);
        if (hud && !includeHud) continue;
        int before = g_npts;
        char *head = *(char **)(iface + 0x2c);
        if (!rd(head, 0x18)) continue;
        char *stack[64];
        int sp = 0, guard = 0;
        char *node = *(char **)(head + 4);
#define TREE_OK(p) ((p) && (p) != head && rd((p), 0x18) && !(p)[0x15])
        for (;;) {
            while (TREE_OK(node) && sp < 64) {
                stack[sp++] = node;
                node = *(char **)node;
            }
            if (sp == 0 || ++guard > 6000) break;
            node = stack[--sp];
            add_window(*(char **)(node + 0x10));
            node = *(char **)(node + 8);
        }
#undef TREE_OK
        if (!hud && g_npts > before) {
            menus++;
            if (strlen(g_menuNames) + strlen(name) + 2 < sizeof g_menuNames) {
                lstrcatA(g_menuNames, name);
                lstrcatA(g_menuNames, " ");
            }
        }
    }
    return menus;
}
/* one-off listing of every visible interface, to check the layout above against the real game */
static void dump_interfaces(void)
{
    char *shell = UIShell_Get();
    if (!rd(shell, 0x90)) return;
    char **b = *(char ***)(shell + 4), **e = *(char ***)(shell + 8);
    int n = (int)(e - b);
    logf_("ui: screen %dx%d, %d interfaces", UIShell_GetScreenWidth(shell), UIShell_GetScreenHeight(shell), n);
    if (n <= 0 || n > 1024 || !rd(b, n * sizeof(void *))) return;
    char buf[900];
    int o = 0;
    for (int i = 0; i < n && o < 820; i++) {
        char *iface = b[i];
        if (!rd(iface, 0x34)) continue;
        const char *name = *(const char **)(iface + 0xc);
        if (!name || IsBadStringPtrA(name, 64)) continue;
        if (iface[0] & 4) o += snprintf(buf + o, sizeof buf - o, "%s%s ", name, is_hud(name) ? "(hud)" : "");
    }
    buf[o] = 0;
    logf_("ui: visible interfaces: %s", buf);
}

/* ------------------------------------------------------------------ cursor helpers */
static int cursor_ui(int *x, int *y)
{
    void *app = AppModule_Get();
    if (!rd(app, 0x140)) return 0;
    *x = (int)(AppModule_GetCursorX(app) / g_uiS);
    *y = (int)(AppModule_GetCursorY(app) / g_uiS);
    return 1;
}
static int g_tgtActive, g_tgtX, g_tgtY;
static DWORD g_tgtStart, g_lastNudge;
static POINT g_lastSet;
static void set_cursor_ui(float x, float y)
{
    if (x < 1) x = 1;
    if (y < 1) y = 1;
    if (x > g_uiW - 2) x = (float)(g_uiW - 2);
    if (y > g_uiH - 2) y = (float)(g_uiH - 2);
    g_tgtX = (int)(x + 0.5f);
    g_tgtY = (int)(y + 0.5f);
    g_tgtActive = 1;
    g_tgtStart = timeGetTime();
}
/* where the time of one update goes (milliseconds), for the "slow update" line in the log */
enum { SEC_SCAN, SEC_WALK, SEC_TARGET, SEC_POTION, SEC_FOLLOW, SEC_MENU, SEC_NAV, SEC_CURSOR, SEC_PICKUP, SEC_N };
static float g_secMs[SEC_N];
static LONGLONG g_qpf;
static LONGLONG qpc_now(void) { LARGE_INTEGER q; QueryPerformanceCounter(&q); return q.QuadPart; }
static float qpc_ms(LONGLONG a, LONGLONG b)
{
    if (!g_qpf) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); g_qpf = f.QuadPart ? f.QuadPart : 1; }
    return (float)((double)(b - a) * 1000.0 / (double)g_qpf);
}
#define TIMED(sec, stmt) do { LONGLONG t_a = qpc_now(); stmt; g_secMs[sec] += qpc_ms(t_a, qpc_now()); } while (0)
static void nudge_os(int dx, int dy, DWORD now)
{
    POINT os;
    if (!GetCursorPos(&os)) return;
    TIMED(SEC_CURSOR, SetCursorPos(os.x + dx, os.y + dy));
    g_lastSet.x = os.x + dx;
    g_lastSet.y = os.y + dy;
    g_lastNudge = now;
}
/* The pointer is steered by nudging the OS pointer by the remaining error until the game's own
   cursor reaches the target; that works whether the game reads absolute or relative mouse motion. */
static void drive_cursor(DWORD now)
{
    int cx, cy;
    POINT os;
    if (!g_tgtActive) return;
    if (!cursor_ui(&cx, &cy) || now - g_tgtStart > 700) { g_tgtActive = 0; return; }
    int dx = g_tgtX - cx, dy = g_tgtY - cy;
    if (abs(dx) <= 1 && abs(dy) <= 1) { g_tgtActive = 0; return; }
    if (GetCursorPos(&os) && os.x == g_lastSet.x && os.y == g_lastSet.y && now - g_lastNudge < 120) return;
    nudge_os((int)(dx * g_uiS), (int)(dy * g_uiS), now);
}

static void nav_move(int dx, int dy)
{
    int cx, cy;
    collect_ui(0);
    if (g_npts == 0) collect_ui(1);
    if (!cursor_ui(&cx, &cy) || g_npts == 0) return;
    if (g_tgtActive) { cx = g_tgtX; cy = g_tgtY; }
    int best = -1;
    float bestScore = 1e9f;
    for (int i = 0; i < g_npts; i++) {
        float vx = (float)(g_pts[i].x - cx), vy = (float)(g_pts[i].y - cy);
        float along = vx * dx + vy * dy;
        float perp = fabsf(vx * dy - vy * dx);
        if (along < 4.0f) continue;
        if (perp > along * 3.0f + 8.0f) continue;
        float score = along + perp * 2.5f;
        if (score < bestScore) { bestScore = score; best = i; }
    }
    static int navLog;
    if (best >= 0) set_cursor_ui(g_pts[best].x, g_pts[best].y);
    if (navLog < 40) {
        POINT sp = { 0, 0 };
        GetCursorPos(&sp);
        navLog++;
        logf_("nav dir=(%d,%d) pts=%d from=(%d,%d) -> %s (%d,%d) os=(%ld,%ld)", dx, dy, g_npts, cx, cy,
              best >= 0 ? g_pts[best].type : "none", best >= 0 ? g_pts[best].x : -1, best >= 0 ? g_pts[best].y : -1, sp.x, sp.y);
    }
}
static void dump_ui(void)
{
    static int dumps;
    RECT rc = { 0, 0, 0, 0 };
    POINT sp = { 0, 0 };
    int cx = -1, cy = -1;
    if (dumps++ >= 8) return;
    if (g_hwnd) GetClientRect(g_hwnd, &rc);
    GetCursorPos(&sp);
    cursor_ui(&cx, &cy);
    logf_("ui %dx%d client %ldx%ld cursor screen (%ld,%ld) game (%d,%d) points %d (windows looked at %d)", g_uiW, g_uiH,
          rc.right, rc.bottom, sp.x, sp.y, cx, cy, g_npts, g_winSeen);
    for (int i = 0; i < g_npts && i < 50; i++)
        logf_("  pt %d (%d,%d) %s rect %d,%d,%d,%d", i, g_pts[i].x, g_pts[i].y, g_pts[i].type, g_pts[i].rc.l, g_pts[i].rc.t,
              g_pts[i].rc.r, g_pts[i].rc.b);
}
static void nav_snap_nearest(void)
{
    int cx, cy;
    if (!cursor_ui(&cx, &cy) || g_npts == 0) return;
    int best = -1;
    float bd = 1e12f;
    for (int i = 0; i < g_npts; i++) {
        float vx = (float)(g_pts[i].x - cx), vy = (float)(g_pts[i].y - cy);
        float d = vx * vx + vy * vy;
        if (d < bd) { bd = d; best = i; }
    }
    if (best >= 0) set_cursor_ui(g_pts[best].x, g_pts[best].y);
}

/* ------------------------------------------------------------------ gameplay helpers */
static GopColl g_coll;
static int g_handLogged;

static int go_alive(void *go)
{
    return rd(go, 0x100) && LS_IsAlive(Go_GetLifeState(go));
}

static void *get_party(void)
{
    void *srv = Server_Get();
    if (!rd(srv, 0x20)) return NULL;
    void *party = Server_GetScreenParty(srv);
    return rd(party, 0x100) ? party : NULL;
}

static void *get_leader(void)
{
    void *srv = Server_Get();
    if (!rd(srv, 0x20)) return NULL;
    void *party = get_party();
    void *best = NULL;
    if (party) {
        GopColl *kids = Go_GetChildren(party);
        void *godb = GoDb_Get();
        void *focus = rd(godb, 0x10) ? GoDb_GetFocusGo(godb) : NULL;
        if (rd(kids, 8)) {
            int n = (int)(kids->e - kids->b);
            if (n > 0 && n < 64 && rd(kids->b, n * sizeof(void *))) {
                for (int i = 0; i < n && !best; i++)
                    if (rd(kids->b[i], 0x100) && focus && Go_GetGoid(kids->b[i]) == focus) best = kids->b[i];
                for (int i = 0; i < n && !best; i++)
                    if (rd(kids->b[i], 0x100) && Go_IsSelected(kids->b[i])) best = kids->b[i];
            }
        }
    }
    if (!best) best = Server_GetScreenHero(srv);
    if (!rd(best, 0x100) || !Go_GetMind(best) || !Go_GetPlacement(best)) return NULL;
    return best;
}

static int go_pos(void *go, SiegePos *out)
{
    void *pl = Go_GetPlacement(go);
    if (!rd(pl, 0x40)) return 0;
    SiegePos *p = GoPlacement_GetPosition(pl);
    if (!rd(p, sizeof *p) || !p->node) return 0;
    *out = *p;
    return 1;
}

/* Camera forward/right on the ground plane, expressed in the terrain-piece space of `hp`. */
static SiegePos g_hlPos; /* where the highlighted target stands */
static int g_hlHave;
/* facing diagnostics: what went into the last stick direction */
static struct { float lx, ly, fx, fz, d1x, d1z, d2x, d2z; unsigned camNode, tgtNode; } g_dirDbg, g_dirDbgPrev;
static int cam_basis(const SiegePos *hp, float *fx, float *fz, float *rx, float *rz)
{
    void *eng = SiegeEngine_Get();
    if (!rd(eng, 0x40)) return 0;
    void *cam = SiegeEngine_GetCamera(eng);
    if (!rd(cam, 0x100)) return 0;
    SiegePos *csp = Cam_GetCameraSiegePos(cam), *tsp = Cam_GetTargetSiegePos(cam);
    if (!rd(csp, sizeof *csp) || !rd(tsp, sizeof *tsp) || !csp->node || !tsp->node) return 0;
    Vec3 d1 = { 0, 0, 0 }, d2 = { 0, 0, 0 };
    GetSiegeDifference(&d1, hp, tsp);
    GetSiegeDifference(&d2, hp, csp);
    float x = d1.x - d2.x, z = d1.z - d2.z;
    float len = sqrtf(x * x + z * z);
    g_dirDbg.d1x = d1.x; g_dirDbg.d1z = d1.z; g_dirDbg.d2x = d2.x; g_dirDbg.d2z = d2.z;
    g_dirDbg.camNode = csp->node; g_dirDbg.tgtNode = tsp->node;
    if (len < 0.01f || len != len) return 0;
    x /= len;
    z /= len;

    /* Which side is "right" of the camera is a fixed property of the game's world, not something that
       changes from frame to frame. It used to be re-derived every frame from the camera's matrix, and
       that reading flipped for single frames - the hero then faced the mirror image of the stick's
       direction for that frame. Now it is fixed (StickXSign in the ini swaps it). */
    int sign = c_stickXSign ? c_stickXSign : -1;
    if (!g_handLogged) {
        g_handLogged = 1;
        logf_("camera basis: StickXSign=%d (set StickXSign=1 in the ini if left and right are swapped)", sign);
    }
    *fx = x;
    *fz = z;
    if (sign > 0) { *rx = z; *rz = -x; }
    else { *rx = -z; *rz = x; }
    return 1;
}

static DWORD g_lastMove;
static float g_lastDx, g_lastDz;
static int g_moving;

static char *go_nema(void *go)
{
    if (!Go_GetAspect || !GoAspect_GetAspectPtr) return NULL;
    void *asp = Go_GetAspect(go);
    if (!rd(asp, 0x48)) return NULL;
    char *nema = GoAspect_GetAspectPtr(asp);
    return rd(nema, 0x120) ? nema : NULL;
}

/* ------------------------------------------------------------------ code patches */
static int patch_call(DWORD site, DWORD oldTarget, void *newTarget)
{
    BYTE *p = (BYTE *)site;
    DWORD old;
    if (!rd(p, 5) || p[0] != 0xe8 || (DWORD)(site + 5 + *(int *)(p + 1)) != oldTarget) return 0;
    if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *(int *)(p + 1) = (int)((DWORD)newTarget - (site + 5));
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    return 1;
}

/* Last word on the hero's placement. The character's "follower" component writes position and
   facing through five call sites and the animation through two. Those calls are redirected here,
   and while the stick is driving the hero the values are replaced with the mod's own. */
#define FN_SET_POS    0x805975 /* placement: set position */
#define FN_SET_ROT    0x805a7a /* placement: set facing */
#define FN_SET_POSROT 0x805bc8 /* placement: set both */
#define FN_SET_CHORE  0x560750 /* animation: next chore (chore, stance, sub-animation, flags, speed) */
static void *g_ovrPlacement, *g_ovrNema;
static void *g_directHero, *g_directGoid;
static int g_ovrPos, g_ovrRot, g_ovrAnim, g_ovrHooked, g_choreHooked;
static SiegePos g_ovrPosVal;
static float g_ovrQuat[4];
/* After a walk the planner still has to catch up with where the stick left the hero (it is up to a
   quarter of a second behind). When an order follows the walk at once (attack, pick up, talk), the
   hero is held where he stands until the planner's own position reaches that spot, and only then
   handed over - otherwise he would be seen jumping back to where the planner thinks he is. */
static int g_ovrHandover;
static DWORD g_handT;
static float g_handLast;
static void direct_release(void);
static int handover_done(const SiegePos *p)
{
    Vec3 d = { 0, 0, 0 };
    if (timeGetTime() - g_handT > 700 || !rd(p, sizeof *p) || !p->node) return 1;
    GetSiegeDifference(&d, &g_ovrPosVal, p);
    float dist = sqrtf(d.x * d.x + d.z * d.z);
    if (dist < 0.3f || (dist > g_handLast + 0.02f && g_handLast < 1.5f)) return 1;
    g_handLast = dist;
    return 0;
}
static SiegePos *held_pos(SiegePos *p)
{
    if (g_ovrHandover && handover_done(p)) { direct_release(); return p; }
    return &g_ovrPosVal;
}
/* The same guard on the one routine every position or facing change of any object ends up in, so
   nothing else in the game can turn or move the hero behind the stick's back either. */
#define FN_SET_PLACEMENT 0x804e8d
static void (TC *g_origSetPlacement)(void *, SiegePos *, float *, int);
static int g_ovrSelf; /* the mod itself is placing the hero */
static void TC hk_set_placement(void *pl, SiegePos *p, float *q, int flag)
{
    if (pl == g_ovrPlacement && !g_ovrSelf) {
        if (p && g_ovrPos && p != &g_ovrPosVal) p = held_pos(p);
        if (q && g_ovrRot && q != g_ovrQuat) {
            static void *seen[8];
            static int ns;
            void *ra = __builtin_return_address(0);
            float dot = rd(q, 16) ? q[0] * g_ovrQuat[0] + q[1] * g_ovrQuat[1] + q[2] * g_ovrQuat[2] + q[3] * g_ovrQuat[3] : 1.0f;
            int known = 0;
            for (int i = 0; i < 8; i++) known |= seen[i] == ra;
            if (!known && fabsf(dot) < 0.99f && ns < 8) { seen[ns++] = ra; logf_("facing: a turn asked for by game code at %p was overridden", ra); }
            q = g_ovrQuat;
        }
    }
    g_origSetPlacement(pl, p, q, flag);
}
/* facing diagnostics: how often the game's walker wanted the hero turned some other way */
static int g_turnWanted[3];
static void turn_note(const float *q, int site)
{
    if (!rd(q, 16)) return;
    float dot = q[0] * g_ovrQuat[0] + q[1] * g_ovrQuat[1] + q[2] * g_ovrQuat[2] + q[3] * g_ovrQuat[3];
    if (fabsf(dot) < 0.995f) g_turnWanted[site]++;
}
static void TC hk_follower_set_pos(void *pl, SiegePos *p)
{
    if (g_ovrPos && pl == g_ovrPlacement) p = held_pos(p);
    ((void (TC *)(void *, SiegePos *))FN_SET_POS)(pl, p);
}
static void TC hk_follower_set_rot(void *pl, float *q)
{
    if (g_ovrRot && pl == g_ovrPlacement) { turn_note(q, 1); q = g_ovrQuat; }
    ((void (TC *)(void *, float *))FN_SET_ROT)(pl, q);
}
static void TC hk_follower_set_posrot(void *pl, SiegePos *p, float *q)
{
    if (pl == g_ovrPlacement) {
        if (g_ovrPos) p = held_pos(p);
        if (g_ovrRot) { turn_note(q, 2); q = g_ovrQuat; }
    }
    ((void (TC *)(void *, SiegePos *, float *))FN_SET_POSROT)(pl, p, q);
}
static bool8 TC hk_follower_set_chore(void *aspect, int chore, int stance, int subanim, int flags, float speed)
{
    if (g_ovrAnim && aspect == g_ovrNema) {
        /* a hero that has just died gets its animations from the engine again, at once */
        if (rd(g_directHero, 0x100) && Go_GetGoid(g_directHero) == g_directGoid && !go_alive(g_directHero)) direct_release();
        else return 1;
    }
    return ((bool8 (TC *)(void *, int, int, int, int, float))FN_SET_CHORE)(aspect, chore, stance, subanim, flags, speed);
}
/* Camera lock: the game's tracking camera aims at the party as a whole, which trails the group
   loosely. While the option is on, its tracking update is handed the controlled hero instead. Only
   the tracking modes run this code, so cutscenes and scripted camera moves are untouched. */
static int g_camHooked;
static int g_modeNow;
static void *get_leader(void);
static void *TC hk_cam_party(void *server)
{
    void *party = Server_GetScreenParty(server);
    if (!c_camLock || !party || g_modeNow == 0 || g_modeNow == 3) return party; /* 0 none, 3 cutscene */
    void *hero = get_leader();
    if (!hero || !rd(Go_GetPlacement(hero), 0x60)) return party;
    return hero;
}
/* Frame rate fix. About 2,400 small functions of this executable do not run directly: each call goes
   through the copy protection's interpreter, and that interpreter asks the processor to identify
   itself (CPUID) eight times per call, then throws the answer away and uses a fixed one. On current
   systems each CPUID is very slow (with Windows' virtualization-based security it leaves to the
   hypervisor and back), and the game makes thousands of such calls a frame. The 171 CPUID instructions
   are replaced by no-ops; the fixed answer the interpreter loads right after is untouched. */
#include "cpuid_sites.h"
static int c_fpsFix = 1;
static void install_fps_fix(void)
{
    int n = (int)(sizeof cpuid_sites / sizeof cpuid_sites[0]), done = 0, other = 0;
    static const BYTE nops[2] = { 0x90, 0x90 };
    if (!c_fpsFix) { logf_("frame rate fix: off"); return; }
    for (int i = 0; i < n; i++) {
        BYTE *p = (BYTE *)cpuid_sites[i];
        if (rd(p, 2) && p[0] == 0x0f && p[1] == 0xa2) done += write_code(cpuid_sites[i], nops, 2);
        else other++;
    }
    logf_("frame rate fix: %d of %d processor-identification calls removed from the copy protection's interpreter (%d not as expected)", done, n, other);
}
/* Target highlight. Every frame the game works out which objects the mouse pointer is over and gives
   them its "hovered" look (the outline, and whatever the interface shows for a hovered object). That
   routine reads a list of the objects under the pointer; while the pad is in use, it is handed a list
   holding just the pad's current target instead, so the target looks exactly like a hovered object. */
#define FN_MOUSE_SHADOW 0x813a98
static void *g_hlGoid;
static int g_padPlay, c_highlight = 1, g_hlHooked;
static void TC hk_mouse_shadow(void *godb)
{
    static struct { void *next, *prev, *goid; } fake;
    char *eng = SiegeEngine_Get ? SiegeEngine_Get() : NULL;
    char *ms = rd(eng, 0x44) ? *(char **)(eng + 0x40) : NULL;
    char *head = rd(ms, 0x48) ? *(char **)(ms + 0x44) : NULL;
    if (!g_padPlay || !c_highlight || !rd(head, 8)) {
        ((void (TC *)(void *))FN_MOUSE_SHADOW)(godb);
        return;
    }
    void *saved = *(void **)head;
    if (g_hlGoid) {
        fake.next = head;
        fake.prev = head;
        fake.goid = g_hlGoid;
        *(void **)head = &fake;
    } else *(void **)head = head; /* nothing targeted: nothing hovered */
    ((void (TC *)(void *))FN_MOUSE_SHADOW)(godb);
    *(void **)head = saved;
}
/* Enemy name and health readout: the game shows it for whatever the selected hero is fighting (read
   from the hero's mind). While the pad is in use it is shown for the pad's target instead. */
#define FN_ENEMY_STATUS 0x4b161f
static int g_hlEnemy;
static void *get_leader(void);
static void TC hk_enemy_status(void *pm, float dt)
{
    void *hero = (g_padPlay && g_hlGoid && g_hlEnemy) ? get_leader() : NULL;
    char *mind = hero ? Go_GetMind(hero) : NULL;
    if (!rd(mind, 0x118)) { ((void (TC *)(void *, float))FN_ENEMY_STATUS)(pm, dt); return; }
    void *saved = *(void **)(mind + 0x114);
    *(void **)(mind + 0x114) = g_hlGoid;
    ((void (TC *)(void *, float))FN_ENEMY_STATUS)(pm, dt);
    *(void **)(mind + 0x114) = saved;
}
static void install_highlight(void)
{
    if (!c_highlight) return;
    logf_("enemy readout hook: %d", code_is(FN_ENEMY_STATUS, "558bec83ec148365f000") && patch_call(0x4bd66d, FN_ENEMY_STATUS, hk_enemy_status));
    if (!code_is(FN_MOUSE_SHADOW, "558bec83ec2ca12c16d000")) { logf_("target highlight: hover routine not as expected; off"); return; }
    int a = patch_call(0x42e09b, FN_MOUSE_SHADOW, hk_mouse_shadow), b = patch_call(0x469c8a, FN_MOUSE_SHADOW, hk_mouse_shadow);
    g_hlHooked = a || b;
    logf_("target highlight hooks: %d/%d", a, b);
}
static void install_follower_hooks(void)
{

    if (!code_is(FN_SET_POS, "558bec6a006a00ff7508e8") || !code_is(FN_SET_ROT, "558bec6a00ff75086a00e8") ||
        !code_is(FN_SET_POSROT, "558bec6a00ff750cff7508e8") || !code_is(FN_SET_CHORE, "558bec566a006a006a006a00")) {
        logf_("follower hooks: the game's code is not the expected 2.30 build; direct control disabled");
        return;
    }
    g_origSetPlacement = hook_function(FN_SET_PLACEMENT, "558bec81ec88000000", hk_set_placement);
    logf_("placement guard: %s", g_origSetPlacement ? "installed" : "NOT installed");
    int a = patch_call(0x903909, FN_SET_POS, hk_follower_set_pos);
    int b = patch_call(0x90656e, FN_SET_POS, hk_follower_set_pos);
    int c = patch_call(0x907182, FN_SET_POS, hk_follower_set_pos);
    int d = patch_call(0x906c4f, FN_SET_ROT, hk_follower_set_rot);
    int e = patch_call(0x905a5c, FN_SET_POSROT, hk_follower_set_posrot);
    int f = patch_call(0x90594f, FN_SET_CHORE, hk_follower_set_chore);
    int g = patch_call(0x905d3a, FN_SET_CHORE, hk_follower_set_chore);
    g_ovrHooked = a && b && c && d && e;
    g_choreHooked = f && g;
    logf_("follower hooks: position %d/%d/%d facing %d both %d animation %d/%d", a, b, c, d, e, f, g);
}

/* ------------------------------------------------------------------ direct control
   The mod moves the hero itself every frame. Each step is asked of the game's own pathfinder: "walk
   in a straight line from here, this way, this far" - the same walk-mesh test the game uses, so
   lifts, bridges, doors and furniture behave as they do for mouse movement. */
static int g_direct;
static DWORD g_directAnimCheck;
static int g_directLogs;
static float g_dt = 0.016f;
static int g_everMoved, g_blockedRun;

static int direct_available(void)
{
    if (c_moveMode != 2 || !g_directOK || !g_ovrHooked) return 0;
    void *world = World_Get();
    return rd(world, 0x10) && !World_IsMultiPlayer(world);
}
static void direct_anim(void *hero, int chore)
{
    void *body = Go_GetBody(hero);
    AnimReq r = { chore, 0, -1, 0, 0, 1.0f };
    if (rd(body, 0x50)) GoBody_SAnimate(body, &r);
}
static void shadow_request(void *hero, const SiegePos *np, int force);
static void direct_start(void *hero)
{
    void *mind = Go_GetMind(hero);
    void *mcp = MCP_Get();
    if (GoMind_GetFrontJob(mind, JQ_ACTION)) GoMind_ClearQ(mind, JQ_ACTION);
    if (rd(mcp, 0x10)) MCP_Flush(mcp, Go_GetGoid(hero), 0.0f);
    direct_anim(hero, CHORE_WALK);
    g_direct = 1;
    g_directHero = hero;
    g_directGoid = Go_GetGoid(hero);
    g_directAnimCheck = timeGetTime();
    if (g_directLogs < 30) { g_directLogs++; logf_("direct: start"); }
}
/* The movement planner keeps, per character, the path it last planned (a list of timed positions) and
   moves the character along it. After a walk driven by the stick that path lags behind where the hero
   really is. Here every position on the hero's path is rewritten to his real position, so the planner
   is caught up at once and starts whatever comes next from the right spot.
   Layout (2.30): planner [0x14] map of plans, [0x18] its end; plan [0] character id, [0xc] list head,
   [0x10] count; list node [0] next, [4] previous, [0x30] position. */
#define FN_PLAN_FIND 0x881c59
static int g_planFindOK = -1, g_planNodeOK = -1;
static char *mcp_plan(void *goid)
{
    if (g_planFindOK < 0) {
        g_planFindOK = code_is(FN_PLAN_FIND, "558bec5156578b7d0c578bf1");
        if (!g_planFindOK) logf_("planner: plan lookup routine not found; catch-up disabled");
    }
    char *mcp = MCP_Get();
    if (!g_planFindOK || !rd(mcp, 0x20)) return NULL;
    char *it = NULL;
    void *key = goid;
    ((void (TC *)(void *, char **, void **))FN_PLAN_FIND)(mcp + 0x14, &it, &key);
    if (!it || it == *(char **)(mcp + 0x18) || !rd(it, 0x14)) return NULL;
    char *plan = *(char **)(it + 0x10);
    if (!rd(plan, 0x198) || *(void **)plan != goid) return NULL;
    return plan;
}
static int plan_sync(void *hero, const SiegePos *pos)
{
    char *plan = mcp_plan(Go_GetGoid(hero));
    if (!plan) return 0;
    char *head = *(char **)(plan + 0xc);
    int n = *(int *)(plan + 0x10);
    if (n <= 0 || n > 512 || !rd(head, 8)) return 0;
    if (g_planNodeOK < 0) {
        /* first use: the path's last position must look like a position near the hero */
        char *last = *(char **)(head + 4);
        if (!rd(last, 0x58)) return 0;
        SiegePos *p = (SiegePos *)(last + 0x30);
        Vec3 d = { 99, 99, 99 };
        if (p->node) GetSiegeDifference(&d, pos, p);
        g_planNodeOK = p->node != 0 && d.x * d.x + d.z * d.z < 16.0f;
        logf_("planner: path end %s (%d points, %.2f,%.2f,%.2f piece %08x; hero %.2f,%.2f,%.2f piece %08x)",
              g_planNodeOK ? "verified" : "MISMATCH - catch-up disabled", n, p->x, p->y, p->z, p->node, pos->x, pos->y, pos->z, pos->node);
    }
    if (g_planNodeOK != 1) return 0;
    char *node = *(char **)head;
    for (int i = 0; i < n && node != head && rd(node, 0x58); i++) {
        *(SiegePos *)(node + 0x30) = *pos;
        node = *(char **)node;
    }
    return 1;
}
/* Hands the hero back to the engine: stand animation, and the planner is told where the hero is.
   The mod's hold on position and facing is kept for a moment longer so the planner's catching-up
   (its last "shadow" stretch) is not shown; any real order releases the hold at once. */
static DWORD g_ovrRelease;
static void direct_release(void)
{
    g_ovrPos = g_ovrRot = 0;
    g_ovrAnim = 0;
    g_ovrRelease = 0;
    g_ovrHandover = 0;
}
/* an order is about to be given: facing and animation go back to the engine now, the position when
   the planner has caught up */
static void direct_handover(void)
{
    if (!g_ovrPos) { direct_release(); return; }
    if (rd(g_directHero, 0x100) && Go_GetGoid(g_directHero) == g_directGoid) plan_sync(g_directHero, &g_ovrPosVal);
    g_ovrHandover = 1;
    g_handT = timeGetTime();
    g_handLast = 1e9f;
    g_ovrRot = 0;
    g_ovrAnim = 0;
    g_ovrRelease = 0;
}
static void direct_stop(void)
{
    if (!g_direct) return;
    g_direct = 0;
    void *hero = g_directHero;
    if (rd(hero, 0x100) && Go_GetGoid(hero) == g_directGoid && !go_alive(hero)) {
        /* the hero died while being walked: nothing is asked of the body (the stand animation here made
           the fallen hero get up again), the engine has it from this moment */
        direct_release();
        if (g_directLogs < 30) { g_directLogs++; logf_("direct: stop (hero is down)"); }
        return;
    }
    if (rd(hero, 0x100) && Go_GetGoid(hero) == g_directGoid) {
        SiegePos sp;
        if (go_pos(hero, &sp)) { g_ovrPosVal = sp; shadow_request(hero, &sp, 1); plan_sync(hero, &sp); }
        direct_anim(hero, CHORE_DEFAULT);
        g_ovrRelease = timeGetTime() + 400;
        if (!g_ovrRelease) g_ovrRelease = 1;
    } else direct_release();
    if (g_directLogs < 30) { g_directLogs++; logf_("direct: stop"); }
}

/* other characters and monsters near the hero, as circles to keep out of */
static float g_blk[40][3];
static const char *g_blkName[40];
static int g_nblk, g_blkHit;
static void collect_blockers(void *hero, const SiegePos *hp)
{
    g_nblk = 0;
    if (c_actorSpacing <= 0.0f) return;
    void *aiq = AIQuery_Get();
    void *mind = Go_GetMind(hero);
    if (!rd(aiq, 0x10)) return;
    g_coll.e = g_coll.b;
    AIQuery_GetOccupantsInSphere(aiq, hp, c_actorSpacing + 3.0f, &g_coll);
    int n = (int)(g_coll.e - g_coll.b);
    if (n <= 0 || n > 8192 || !rd(g_coll.b, n * sizeof(void *))) return;
    for (int i = 0; i < n && g_nblk < 40; i++) {
        void *g = g_coll.b[i];
        SiegePos p;
        Vec3 d = { 0, 0, 0 };
        if (!rd(g, 0x100) || g == hero || !Go_IsActor(g) || !go_pos(g, &p)) continue;
        if (Go_IsScreenPartyMember(g) || !go_alive(g)) continue;
        /* things nobody can see (script helpers, hidden spawns) are not obstacles */
        void *asp = Go_GetAspect ? Go_GetAspect(g) : NULL;
        if (rd(asp, 0x48) && GoAspect_GetIsVisible && !GoAspect_GetIsVisible(asp)) continue;
        /* only monsters are kept at a distance: the game itself lets characters walk through
           bystanders, and some of those are unseen script helpers ("invisible_talker") */
        if (!rd(mind, 0x100) || !GoMind_IsEnemy || !GoMind_IsEnemy(mind, g)) continue;
        GetSiegeDifference(&d, hp, &p);
        g_blk[g_nblk][0] = d.x;
        g_blk[g_nblk][1] = d.z;
        g_blk[g_nblk][2] = c_actorSpacing;
        g_blkName[g_nblk] = Go_GetTemplateName ? Go_GetTemplateName(g) : NULL;
        g_nblk++;
    }
}
static int blocked_by_actor(float ox, float oz)
{
    for (int i = 0; i < g_nblk; i++) {
        float bx = g_blk[i][0], bz = g_blk[i][1];
        float was = bx * bx + bz * bz;
        float nx = bx - ox, nz = bz - oz;
        float now = nx * nx + nz * nz;
        if (now < g_blk[i][2] * g_blk[i][2] && now < was) { g_blkHit = i; return 1; }
    }
    return 0;
}

/* ------------------------------------------------------------------ walk mesh
   Each terrain piece carries walk meshes (triangles grouped into patches), and each walk mesh carries
   the list of walk meshes it is joined to right now - on the same piece and on neighbouring pieces.
   When a lift docks, the platform's mesh is joined to that landing's mesh. A step is tested against
   exactly this data: the hero stands on a mesh, and a step may land on that mesh, on one joined to
   it, or on one joined to those.
   Layout (DungeonSiege2.exe 2.30):
     piece:  [0] array of mesh objects (index = mesh id), [4] count, [0x128] piece identity
     mesh object: [0] mesh data, [8] owning piece, [0xc] floor-type flags, [0x10] number of joins
                  (u16), [0x14] joins, 0x20 bytes each: { piece identity (0x18), info* } with info[0]
                  = mesh id on that piece; [0x20],[0x24] sorted list of occupied patches
     mesh data:   [4] box min xyz, [0x10] box max xyz, [0x1c] -> { ?, triangles } (0x30 bytes each:
                  three corners), [0x24] patch count, [0x28] patches (0x38 bytes each: [0] id (u16),
                  [0x1c] centre, [0x28] triangle count (u16), [0x2c] triangle indices (u16*)) */
#define FN_PIECE    0x6bda10 /* engine: loaded terrain piece by identity */
#define FN_OCCUPIED 0x704a20 /* mesh object: is this patch occupied */
typedef struct { char *ln; unsigned guid[6]; char *nodep; } MeshRef;
static int g_wmOK;
static char *wm_piece(const unsigned *guid)
{
    if (!guid[0]) return NULL;
    char *n = ((char *(TC *)(void *, const unsigned *))FN_PIECE)(SiegeEngine_Get(), guid);
    return rd(n, 0x170) ? n : NULL;
}
static int wm_mesh_ok(char *lm)
{
    /* every array of a mesh is checked once for readability, then remembered */
    static char *good[64], *bad[16];
    static int ng, nb;
    for (int i = 0; i < 64; i++) if (good[i] == lm) return 1;
    for (int i = 0; i < 16; i++) if (bad[i] == lm) return 0;
    int ok = 0;
    if (rd(lm, 0x34)) {
        int nleaf = *(int *)(lm + 0x24);
        char *leaves = *(char **)(lm + 0x28), *to = *(char **)(lm + 0x1c);
        if (nleaf > 0 && nleaf < 20000 && rd(leaves, nleaf * 0x38) && rd(to, 8) && rd(*(char **)(to + 4), 0x30)) {
            char *tris = *(char **)(to + 4);
            ok = 1;
            for (int i = 0; i < nleaf && ok; i++) {
                char *lf = leaves + i * 0x38;
                int nt = *(WORD *)(lf + 0x28);
                WORD *idx = *(WORD **)(lf + 0x2c);
                if (!nt) continue;
                if (nt > 4000 || !rd(idx, nt * 2)) { ok = 0; break; }
                for (int k = 0; k < nt; k++)
                    if (!rd(tris + idx[k] * 0x30, 0x24)) { ok = 0; break; }
            }
        }
    }
    if (ok) good[ng++ % 64] = lm; else { bad[nb++ % 16] = lm; logf_("walk mesh: unreadable mesh data at %p", lm); }
    return ok;
}
static WORD g_wmLeaf;   /* patch of the last triangle wm_on_mesh matched */
static char *g_wmHitLn; /* mesh and patch of the last spot wm_locate found */
static WORD g_wmHitLeaf;
/* is (x,z) over a triangle of this mesh? gives the height there; of several, the one nearest refY */
static int wm_on_mesh(char *ln, float x, float z, float refY, float *yOut)
{
    if (!rd(ln, 0x28)) return 0;
    char *lm = *(char **)ln;
    if (!wm_mesh_ok(lm)) return 0;
    float *bb = (float *)(lm + 4);
    if (x < bb[0] - 0.1f || x > bb[3] + 0.1f || z < bb[2] - 0.1f || z > bb[5] + 0.1f) return 0;
    int nleaf = *(int *)(lm + 0x24), hit = 0;
    char *leaves = *(char **)(lm + 0x28), *tris = *(char **)(*(char **)(lm + 0x1c) + 4);
    float best = 1.6f, bestCentre = 1e9f;
    /* A triangle can be listed in several patches. The patch that matters (the "occupied" mark is kept
       per patch) is the one whose centre is nearest the point, which is how the game picks it too. */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < nleaf; i++) {
            char *lf = leaves + i * 0x38;
            int nt = *(WORD *)(lf + 0x28);
            WORD *idx = *(WORD **)(lf + 0x2c);
            for (int k = 0; k < nt; k++) {
                float *v = (float *)(tris + idx[k] * 0x30);
                float ax = v[0], az = v[2], bx = v[3], bz = v[5], cx = v[6], cz = v[8];
                float d = (bz - cz) * (ax - cx) + (cx - bx) * (az - cz);
                if (d > -1e-6f && d < 1e-6f) continue;
                float u = ((bz - cz) * (x - cx) + (cx - bx) * (z - cz)) / d;
                float w = ((cz - az) * (x - cx) + (ax - cx) * (z - cz)) / d;
                float q = 1.0f - u - w;
                if (u < -0.01f || w < -0.01f || q < -0.01f) continue;
                float y = u * v[1] + w * v[4] + q * v[7];
                if (pass == 0) {
                    float dy = fabsf(y - refY);
                    if (dy < best) { best = dy; *yOut = y; hit = 1; }
                } else if (fabsf(y - *yOut) < 0.05f) {
                    float *c = (float *)(lf + 0x1c);
                    float ex = c[0] - x, ey = c[1] - y, ez = c[2] - z, cd = ex * ex + ey * ey + ez * ez;
                    if (cd < bestCentre) { bestCentre = cd; g_wmLeaf = *(WORD *)lf; }
                    break;
                }
            }
        }
        if (!hit) break;
    }
    return hit;
}
/* may a body with these movement permissions use a mesh with these floor flags (the game's own rule) */
static int wm_allowed(DWORD flags, DWORD perm)
{
    DWORD c = flags & perm;
    return (c & 6) && (c & 0x7cf8) && !((c ^ perm) & 0x300);
}
static DWORD wm_perm(void *hero)
{
    void *body = hero ? Go_GetBody(hero) : NULL;
    return rd(body, 0x50) && GoBody_GetPerm ? (DWORD)GoBody_GetPerm(body) : 0xffffffff;
}
/* which mesh is the hero standing on */
static int wm_stand(void *hero, const SiegePos *hp, MeshRef *s)
{
    char *node = wm_piece(&hp->node);
    DWORD perm = wm_perm(hero);
    if (!node) return 0;
    char **lns = *(char ***)node;
    int n = *(int *)(node + 4);
    if (n <= 0 || n > 64 || !rd(lns, n * sizeof(char *))) return 0;
    float best = 1.2f;
    s->ln = NULL;
    for (int i = 0; i < n; i++) {
        float y;
        if (!rd(lns[i], 0x28) || !wm_allowed(*(DWORD *)(lns[i] + 0xc), perm)) continue;
        if (!wm_on_mesh(lns[i], hp->x, hp->z, hp->y, &y)) continue;
        if (fabsf(y - hp->y) < best) { best = fabsf(y - hp->y); s->ln = lns[i]; }
    }
    memcpy(s->guid, &hp->node, 24);
    s->nodep = node;
    return s->ln != NULL;
}
static int wm_joins(const MeshRef *m, DWORD perm, MeshRef *out, int max)
{
    int n = 0, nc = *(WORD *)(m->ln + 0x10);
    char *conn = *(char **)(m->ln + 0x14);
    if (nc <= 0 || nc > 255 || !rd(conn, nc * 0x20)) return 0;
    for (int i = 0; i < nc && n < max; i++) {
        const unsigned *guid = (const unsigned *)(conn + i * 0x20);
        char *info = *(char **)(conn + i * 0x20 + 0x18);
        if (!rd(info, 1)) continue;
        int id = *(unsigned char *)info;
        char *node = guid[0] == m->guid[0] ? m->nodep : wm_piece(guid);
        if (!node) continue;
        char **lns = *(char ***)node;
        int cnt = *(int *)(node + 4);
        if (id >= cnt || cnt > 64 || !rd(lns, cnt * sizeof(char *))) continue;
        if (!rd(lns[id], 0x28) || !wm_allowed(*(DWORD *)(lns[id] + 0xc), perm)) continue;
        out[n].ln = lns[id];
        memcpy(out[n].guid, guid, 24);
        out[n].nodep = node;
        n++;
    }
    return n;
}
static void pos_on_piece(const SiegePos *in, const unsigned *guid, SiegePos *out)
{
    SiegePos o;
    Vec3 d = { 0, 0, 0 };
    memset(&o, 0, sizeof o);
    memcpy(&o.node, guid, 24);
    GetSiegeDifference(&d, &o, in);
    *out = o;
    out->x = d.x; out->y = d.y; out->z = d.z;
}
static MeshRef *g_wmSet;
static int g_wmSetN;
/* Is this point over the mesh the hero stands on, over one joined to it, or over one joined to
   those? On success the point is rewritten as a position on that mesh (piece + height). */
static int wm_locate(const MeshRef *s, DWORD perm, SiegePos *p)
{
    SiegePos in = *p;
    if (in.node != s->guid[0]) {
        if (!wm_piece(&in.node)) return 0;
        pos_on_piece(p, s->guid, &in);
    }
    static MeshRef set[96];
    int n = 0, first;
    g_wmSet = set;
    set[n++] = *s;
    n += wm_joins(&set[0], perm, set + n, 40);
    first = n;
    for (int i = 1; i < first && n < 84; i++) n += wm_joins(&set[i], perm, set + n, 12);
    float y, bestDy = 9.0f;
    int found = 0;
    SiegePos out = in, conv = in;
    unsigned convNode = 0;
    for (int i = 0; i < n; i++) {
        int dup = 0;
        for (int k = 0; k < i; k++) dup |= set[k].ln == set[i].ln;
        if (dup) continue;
        SiegePos q = in;
        if (set[i].guid[0] != s->guid[0]) {
            if (convNode != set[i].guid[0]) {
                pos_on_piece(&in, set[i].guid, &conv);
                convNode = set[i].guid[0];
            }
            q = conv;
        }
        if (!wm_on_mesh(set[i].ln, q.x, q.z, q.y, &y)) continue;
        float dy = fabsf(y - q.y) + (i >= first ? 0.05f : 0.0f);
        if (dy < bestDy) { bestDy = dy; out = q; out.y = y; found = 1; g_wmHitLn = set[i].ln; g_wmHitLeaf = g_wmLeaf; }
    }
    g_wmSetN = n;
    if (found) *p = out;
    return found;
}
static int wm_occupied(char *ln, WORD leaf)
{
    return ((bool8 (TC *)(void *, WORD))FN_OCCUPIED)(ln, leaf) != 0;
}
/* Fallback when the hero's spot cannot be found on the walk mesh: ask the pathfinder for a straight
   walk. It answers exactly only for short hops inside one patch, so it is the second choice. */
static int line_walk(void *hero, const SiegePos *hp, float ex, float ez, float dist, SiegePos *out)
{
    void *wpf = WorldPathFinder_Get();
    void *body = Go_GetBody(hero);
    Vec3 dir = { ex, 0.0f, ez }, d = { 0, 0, 0 };
    *out = *hp;
    if (!rd(body, 0x50)) return 0;
    if (!WPF_FindLinePath(wpf, hp, GoBody_GetPerm(body), &dir, dist, out)) return 0;
    if (!out->node) return 0;
    GetSiegeDifference(&d, hp, out);
    float ox = d.x - ex * dist, oz = d.z - ez * dist;
    return ox * ox + oz * oz < 0.1f * 0.1f && fabsf(d.y) < 1.0f + dist;
}

static void shadow_request(void *hero, const SiegePos *np, int force)
{
    /* The planner has to see the walk too: area triggers (saves, region loading, scripted events)
       fire from the planner's checks along planned paths, and a hero moved only by direct placement
       never crosses anything as far as it is concerned. So a few times a second the planner is asked
       to plan from where its last plan ended to where the hero now is. That "shadow" path is never
       shown (the placement hooks keep the hero where the stick put them). */
    static DWORD last;
    static int logs;
    DWORD now = timeGetTime();
    void *mcp = MCP_Get();
    if (!c_shadowPlan || g_plApproach < 0 || !g_choreHooked || !rd(mcp, 0x10)) return;
    if (!force && now - last < (DWORD)c_shadowMs) return;
    last = now;
    int ret = MCP_MakeRequest(mcp, Go_GetGoid(hero), g_plApproach, np, 0.0f);
    if (logs < 12) {
        const char *rn = MCPRet_ToString(ret);
        logs++;
        logf_("shadow path request -> %s", rn && !IsBadStringPtrA(rn, 64) ? rn : "?");
    }
}

static void face_dir(void *pl, float dx, float dz)
{
    static int checked;
    Vec3 dir = { dx, 0.0f, dz };
    float q[4] = { 0, 0, 0, 1 };
    QuatFromDirection(q, &dir);
    memcpy(g_ovrQuat, q, 16);
    g_ovrSelf++;
    GoPlacement_SSetOrientation(pl, q);
    g_ovrSelf--;
    g_ovrRot = 1;
    if (!checked && GoPlacement_Get2DDirection) {
        Vec3 f = { 0, 0, 0 };
        checked = 1;
        GoPlacement_Get2DDirection(pl, &f);
        logf_("direct: facing check - asked (%.2f,%.2f), hero now faces (%.2f,%.2f)", dx, dz, f.x, f.z);
    }
}

/* A refused step, written out in full: what the mod's reading of the walk mesh considered, and what
   the game's own floor lookup says about the same point. */
static void wall_report(void *hero, const SiegePos *hp, const MeshRef *stand, const SiegePos *probe0, DWORD perm, int why)
{
    static int reports;
    if (reports >= 8 || !code_is(0x6c0b30, "558bec") || !code_is(0x6bd7b0, "558bec")) return;
    reports++;
    void *eng = SiegeEngine_Get();
    char *np = stand->nodep;
    logf_("WALL %d: reason %d, hero %.2f,%.2f,%.2f piece %08x (%d meshes), on mesh %d flags %08x, permissions %08x, probe %.2f,%.2f,%.2f",
          reports, why, hp->x, hp->y, hp->z, hp->node, rd(np, 8) ? *(int *)(np + 4) : -1, *(unsigned char *)(stand->ln + 4),
          *(DWORD *)(stand->ln + 0xc), perm, probe0->x, probe0->y, probe0->z);
    {
        SiegePos p = *probe0;
        wm_locate(stand, perm, &p);
        char buf[700];
        int o = 0;
        for (int i = 0; i < g_wmSetN && i < 40 && o < 640; i++)
            o += snprintf(buf + o, sizeof buf - o, " %08x/%d", g_wmSet[i].guid[0], *(unsigned char *)(g_wmSet[i].ln + 4));
        buf[o] = 0;
        logf_("WALL   meshes considered (%d):%s", g_wmSetN, buf);
        int nc = *(WORD *)(stand->ln + 0x10);
        char *conn = *(char **)(stand->ln + 0x14);
        o = 0;
        for (int i = 0; i < nc && i < 24 && rd(conn, nc * 0x20) && o < 640; i++) {
            const unsigned *g = (const unsigned *)(conn + i * 0x20);
            char *info = *(char **)(conn + i * 0x20 + 0x18);
            char *node = wm_piece(g);
            int id = rd(info, 1) ? *(unsigned char *)info : -1, cnt = node ? *(int *)(node + 4) : -1;
            DWORD fl = 0;
            if (node && id >= 0 && id < cnt && rd(*(char ***)node, cnt * 4) && rd((*(char ***)node)[id], 0x10)) fl = *(DWORD *)((*(char ***)node)[id] + 0xc);
            o += snprintf(buf + o, sizeof buf - o, " %08x/%d(%s,%08x)", g[0], id, node ? "loaded" : "not loaded", (unsigned)fl);
        }
        buf[o] = 0;
        logf_("WALL   hero's mesh joins (%d):%s", nc, buf);
    }
    {
        /* a map of the floor around the hero as the mod reads it: 0.1 m per character, 1.2 m each way.
           H hero, P the point that was refused, . floor, o floor marked occupied, # no floor */
        logf_("WALL   floor map (x to the right, z downwards):");
        for (int r = -12; r <= 12; r++) {
            char line[32];
            for (int c = -12; c <= 12; c++) {
                SiegePos q = *hp;
                q.x += c * 0.1f;
                q.z += r * 0.1f;
                char ch = '#';
                if (wm_locate(stand, perm, &q)) ch = wm_occupied(g_wmHitLn, g_wmHitLeaf) ? 'o' : '.';
                if (r == 0 && c == 0) ch = 'H';
                else if (fabsf(hp->x + c * 0.1f - probe0->x) < 0.051f && fabsf(hp->z + r * 0.1f - probe0->z) < 0.051f) ch = 'P';
                line[c + 12] = ch;
            }
            line[25] = 0;
            logf_("WALL   %s", line);
        }
    }
    for (int pass = 0; pass < 3; pass++) {
        /* the game's own lookup: snap the point to the floor, then find the mesh it is on */
        SiegePos e = *probe0;
        DWORD mask = pass == 2 ? 0xffffffff : pass ? 0x40000000 : perm;
        int ok = ((bool8 (TC *)(void *, SiegePos *, DWORD, int, int))0x6c0b30)(eng, &e, mask, 1, 0) != 0;
        char *ln = ok ? ((char *(TC *)(void *, SiegePos *, DWORD))0x6bd7b0)(eng, &e, mask) : NULL;
        int inset = 0;
        for (int i = 0; i < g_wmSetN; i++) inset |= g_wmSet[i].ln == ln;
        Vec3 d = { 0, 0, 0 };
        if (ok && e.node) GetSiegeDifference(&d, hp, &e);
        float y = 0;
        int mine = rd(ln, 0x28) ? wm_on_mesh(ln, e.x, e.z, e.y, &y) : -1;
        logf_("WALL   game lookup (%s): %s -> %.2f,%.2f,%.2f piece %08x (%.2f,%.2f,%.2f from hero), mesh %d flags %08x allowed %d, among the considered %d, mod finds a triangle there %d (height %.2f), walkable %d",
              pass == 2 ? "every kind of mesh" : pass ? "any floor" : "hero's floor types", ok ? "floor" : "no floor", e.x, e.y, e.z, e.node, d.x, d.y, d.z,
              rd(ln, 0x28) ? *(unsigned char *)(ln + 4) : -1, rd(ln, 0x28) ? *(DWORD *)(ln + 0xc) : 0,
              rd(ln, 0x28) ? wm_allowed(*(DWORD *)(ln + 0xc), perm) : -1, inset, mine, y,
              ok ? GoBody_IsPositionWalkable(Go_GetBody(hero), &e) : -1);
    }
}
static void direct_step(void *hero, const SiegePos *hp, float dx, float dz, DWORD now)
{
    static const float fan[] = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 1.4f, -1.4f };
    void *pl = Go_GetPlacement(hero);
    void *body = Go_GetBody(hero);
    void *mind = Go_GetMind(hero);
    char *nema = go_nema(hero);
    if (g_direct && g_directHero != hero) direct_stop();
    if (!g_direct) direct_start(hero);
    g_ovrRelease = 0;
    g_ovrHandover = 0;

    {
        /* facing diagnostics: what the hero's facing and animation are at the start of this update,
           i.e. what the frame just drawn showed, against what the stick asked for last update */
        static int fl, cl, jl, lastChore = -1, lastNext = -1;
        static float ldx, ldz;
        static DWORD tl;
        if (g_ovrRot && pl == g_ovrPlacement && GoPlacement_GetOrientation) {
            float *q = GoPlacement_GetOrientation(pl);
            if (rd(q, 16)) {
                float dot = q[0] * g_ovrQuat[0] + q[1] * g_ovrQuat[1] + q[2] * g_ovrQuat[2] + q[3] * g_ovrQuat[3];
                if (fabsf(dot) < 0.999f && fl < 40) {
                    Vec3 f = { 0, 0, 0 };
                    fl++;
                    if (GoPlacement_Get2DDirection) GoPlacement_Get2DDirection(pl, &f);
                    logf_("facing: the hero was drawn facing (%.2f,%.2f), the stick had asked for (%.2f,%.2f) - something turned the hero past the guards (quat %.3f,%.3f,%.3f,%.3f wanted %.3f,%.3f,%.3f,%.3f)",
                          f.x, f.z, ldx, ldz, q[0], q[1], q[2], q[3], g_ovrQuat[0], g_ovrQuat[1], g_ovrQuat[2], g_ovrQuat[3]);
                }
            }
            static unsigned lnode;
            int samePiece = lnode == hp->node;
            lnode = hp->node;
            if (samePiece && ldx * dx + ldz * dz < 0.90f && jl < 40) {
                jl++;
                logf_("facing: the walk direction jumped within one update, (%.2f,%.2f) -> (%.2f,%.2f), frame %.1f ms, piece %08x", ldx, ldz, dx, dz, g_dt * 1000.0f, hp->node);
                logf_("facing:   before: stick %.2f,%.2f camera forward %.2f,%.2f (camera %.2f,%.2f piece %08x, its target %.2f,%.2f piece %08x, from the hero)",
                      g_dirDbgPrev.lx, g_dirDbgPrev.ly, g_dirDbgPrev.fx, g_dirDbgPrev.fz, g_dirDbgPrev.d2x, g_dirDbgPrev.d2z, g_dirDbgPrev.camNode, g_dirDbgPrev.d1x, g_dirDbgPrev.d1z, g_dirDbgPrev.tgtNode);
                logf_("facing:   now:    stick %.2f,%.2f camera forward %.2f,%.2f (camera %.2f,%.2f piece %08x, its target %.2f,%.2f piece %08x, from the hero)",
                      g_dirDbg.lx, g_dirDbg.ly, g_dirDbg.fx, g_dirDbg.fz, g_dirDbg.d2x, g_dirDbg.d2z, g_dirDbg.camNode, g_dirDbg.d1x, g_dirDbg.d1z, g_dirDbg.tgtNode);
            }
            if (samePiece && ldx * dx + ldz * dz < 0.90f && g_hlHave && jl < 41) {
                Vec3 td = { 0, 0, 0 };
                GetSiegeDifference(&td, hp, &g_hlPos);
                float tl = sqrtf(td.x * td.x + td.z * td.z);
                if (tl > 0.01f)
                    logf_("facing:   the highlighted target is %.2f m away in direction (%.2f,%.2f); the new walk direction is %.0f degrees off it, the old one was %.0f",
                          tl, td.x / tl, td.z / tl, acosf(fmaxf(-1, fminf(1, (dx * td.x + dz * td.z) / tl))) * 57.3f,
                          acosf(fmaxf(-1, fminf(1, (ldx * td.x + ldz * td.z) / tl))) * 57.3f);
            }
            g_dirDbgPrev = g_dirDbg;
        }
        ldx = dx; ldz = dz;
        if (nema && Aspect_GetCurrentChore && Aspect_GetNextChore) {
            int c = Aspect_GetCurrentChore(nema), n = Aspect_GetNextChore(nema);
            if ((c != lastChore || n != lastNext) && cl < 60) {
                cl++;
                logf_("facing: animation while walking changed, chore %d -> %d, queued chore %d -> %d", lastChore, c, lastNext, n);
            }
            lastChore = c; lastNext = n;
        }
        if (now - tl > 5000) {
            tl = now;
            if (g_turnWanted[1] || g_turnWanted[2])
                logf_("facing: in the last 5 s the game's walker asked for a different facing %d times (turn only) and %d times (move and turn); all replaced by the stick's", g_turnWanted[1], g_turnWanted[2]);
            g_turnWanted[1] = g_turnWanted[2] = 0;
        }
    }
    float v = nema ? Aspect_GetCurrentVelocity(nema) : 0.0f;
    if (!(v > 0.5f && v < 15.0f)) v = rd(body, 0x50) ? GoBody_GetAvgMoveVelocity(body) : 0.0f;
    if (!(v > 0.5f && v < 15.0f)) v = 4.0f;
    float step = v * c_speedScale * g_dt;

    collect_blockers(hero, hp);
    MeshRef stand;
    DWORD perm = wm_perm(hero);
    int wm = g_wmOK && wm_stand(hero, hp, &stand);
    char *hereLn = NULL;
    WORD hereLeaf = 0;
    int hereBlocked = 0;
    if (wm) {
        SiegePos c = *hp;
        if (wm_locate(&stand, perm, &c)) {
            hereLn = g_wmHitLn;
            hereLeaf = g_wmHitLeaf;
            hereBlocked = wm_occupied(hereLn, hereLeaf);
        }
    }
    {
        static int was = -1;
        static DWORD lastLog;
        if (wm != was && (was < 0 || now - lastLog > 1000)) {
            was = wm;
            lastLog = now;
            logf_(wm ? "walk mesh: in use (piece %08x)" : "walk mesh: hero's spot not found on it (piece %08x); pathfinder line test for now", hp->node);
        }
    }
    int moved = -1;
    for (int i = 0; i < 7 && moved < 0; i++) {
        float c = cosf(fan[i]), s = sinf(fan[i]);
        float ex = dx * c - dz * s, ez = dx * s + dz * c;
        float st = step * c; /* sliding along an obstacle is slower the more the hero is turned away */
        if (st < 0.002f) continue;
        if (blocked_by_actor(ex * st, ez * st)) {
            static int bl;
            static DWORD blast;
            if (i == 0 && bl < 40 && now - blast > 500) {
                const char *tn = g_blkName[g_blkHit];
                bl++;
                blast = now;
                logf_("step refused: kept away from %s, %.2f m off (kept at %.2f m), hero %.2f,%.2f,%.2f piece %08x",
                      tn && !IsBadStringPtrA(tn, 64) ? tn : "?", sqrtf(g_blk[g_blkHit][0] * g_blk[g_blkHit][0] + g_blk[g_blkHit][1] * g_blk[g_blkHit][1]),
                      g_blk[g_blkHit][2], hp->x, hp->y, hp->z, hp->node);
            }
            continue;
        }
        SiegePos t = *hp, probe = *hp;
        t.x += ex * st;
        t.z += ez * st;
        probe.x += ex * (st + c_bodyRadius);
        probe.z += ez * (st + c_bodyRadius);
        if (!g_walkCheck) {
            g_ovrSelf++;
            GoPlacement_SSetPosition(pl, &t, 1);
            g_ovrSelf--;
            moved = i;
            break;
        }
        int why = 0;
        if (wm) {
            /* a step needs floor under it and floor a body-radius further on, neither occupied by
               furniture; a hero standing on an occupied patch may still move about on that patch */
            if (!wm_locate(&stand, perm, &probe)) why = 1;
            else if (wm_occupied(g_wmHitLn, g_wmHitLeaf) && !(hereBlocked && g_wmHitLn == hereLn && g_wmHitLeaf == hereLeaf)) why = 2;
            else if (!wm_locate(&stand, perm, &t)) why = 3;
            else if (wm_occupied(g_wmHitLn, g_wmHitLeaf) && !(hereBlocked && g_wmHitLn == hereLn && g_wmHitLeaf == hereLeaf)) why = 4;
        } else {
            if (!line_walk(hero, hp, ex, ez, st + c_bodyRadius, &probe)) why = 5;
            else if (!line_walk(hero, hp, ex, ez, st, &t)) why = 6;
            else if (!GoBody_IsPositionWalkable(body, &t)) why = 7;
        }
        if (wm && (why == 1 || why == 3)) {
            /* second opinion: the game's pathfinder is asked for the same short straight walk. If it
               walks there, the mod's reading of the walk mesh missed something and the step is taken. */
            SiegePos p2, t2;
            if (line_walk(hero, hp, ex, ez, st + c_bodyRadius, &p2) && line_walk(hero, hp, ex, ez, st, &t2) &&
                GoBody_IsPositionWalkable(body, &t2)) {
                static int logs;
                static DWORD llast;
                if (logs < 40 && now - llast > 500) {
                    char *np = stand.nodep;
                    int nc = *(WORD *)(stand.ln + 0x10);
                    logs++;
                    llast = now;
                    logf_("walk mesh said no floor (%d) but the pathfinder walks there: piece %08x -> %08x, hero %.2f,%.2f,%.2f step to %.2f,%.2f,%.2f, piece has %d meshes, hero's mesh flags %08x joins %d, hero permissions %08x",
                          why, hp->node, t2.node, hp->x, hp->y, hp->z, t2.x, t2.y, t2.z, rd(np, 8) ? *(int *)(np + 4) : -1,
                          *(DWORD *)(stand.ln + 0xc), nc, perm);
                }
                why = 0;
                t = t2;
            }
        }
        if (!why) {
            Vec3 d = { 0, 0, 0 };
            GetSiegeDifference(&d, hp, &t);
            if (d.x * d.x + d.y * d.y + d.z * d.z > (st + 1.0f) * (st + 1.0f)) why = 8; /* not really the next step: never teleport */
        }
        if (why) {
            static int dl;
            static DWORD dlast, wlast;
            if (i == 0 && wm && (why == 1 || why == 3) && now - wlast > 1500) {
                SiegePos p0 = *hp;
                wlast = now;
                p0.x += ex * (st + c_bodyRadius);
                p0.z += ez * (st + c_bodyRadius);
                wall_report(hero, hp, &stand, &p0, perm, why);
            }
            if (i == 0 && dl < 40 && now - dlast > 300) {
                dl++;
                dlast = now;
                logf_("step refused: reason %d (1/3 no floor ahead/under, 2/4 occupied ahead/under, 5-7 pathfinder, 8 too far), piece %08x, on walk mesh %d", why, hp->node, wm);
            }
            continue;
        }
        g_ovrSelf++;
        GoPlacement_RCSetPosition(pl, &t);
        g_ovrSelf--;
        moved = i;
    }
    {
        SiegePos np;
        g_ovrPlacement = pl;
        if (go_pos(hero, &np)) { g_ovrPosVal = np; g_ovrPos = 1; shadow_request(hero, &np, 0); }
        g_ovrNema = nema;
        g_ovrAnim = nema != NULL && g_choreHooked;
    }
    {
        /* a stick held with the hero going nowhere, or only sliding sideways, is worth a line */
        static int sl;
        static DWORD slast;
        if (moved != 0 && sl < 40 && now - slast > 700) {
            sl++;
            slast = now;
            logf_("walk: %s (direction tried %d of 7), step %.3f m, frame %.1f ms, hero %.2f,%.2f,%.2f piece %08x, on walk mesh %d, standing on occupied %d, actors near %d",
                  moved < 0 ? "stuck" : "sliding", moved, step, g_dt * 1000.0f, hp->x, hp->y, hp->z, hp->node, wm, hereBlocked, g_nblk);
        }
    }
    if (moved >= 0) { g_everMoved = 1; g_blockedRun = 0; }
    else if (!g_everMoved && g_walkCheck && ++g_blockedRun > 60) {
        g_walkCheck = 0;
        logf_("direct: every direction refused from the very start; walk test switched off (plain placement is used)");
    }
    {
        /* Facing is stored relative to the terrain piece the hero stands on, and pieces are laid at
           different rotations. dx,dz are in the piece the hero stood on before this step; if the step
           crossed onto another piece, the direction is re-expressed in that one - otherwise the hero is
           shown turned by the difference between the two pieces for one frame. */
        SiegePos np, a = *hp, b = *hp;
        float fx = dx, fz = dz;
        if (go_pos(hero, &np) && np.node != hp->node) {
            Vec3 da = { 0, 0, 0 }, db = { 0, 0, 0 };
            static int cl;
            b.x += dx;
            b.z += dz;
            GetSiegeDifference(&da, &np, &a);
            GetSiegeDifference(&db, &np, &b);
            float x = db.x - da.x, z = db.z - da.z, len = sqrtf(x * x + z * z);
            if (len > 0.5f && len < 1.5f) { fx = x / len; fz = z / len; }
            if (cl < 12) {
                cl++;
                logf_("facing: stepped from piece %08x onto %08x; direction (%.2f,%.2f) there is (%.2f,%.2f)", hp->node, np.node, dx, dz, fx, fz);
            }
        }
        face_dir(pl, fx, fz);
    }

    if (now - g_directAnimCheck > 150) {
        g_directAnimCheck = now;
        /* anything the hero's AI queued in the meantime would fight the walk: drop it */
        int *qj = GoMind_GetFrontJob(mind, JQ_ACTION);
        int jat = rd(qj, 4) ? *qj : -1;
        if (qj && jat != JAT_FIDGET && jat != JAT_NONE && jat != JAT_STARTUP) {
            static int jatLogs;
            if (jatLogs < 12) {
                const char *jn = Jat_ToString ? Jat_ToString(jat) : NULL;
                jatLogs++;
                logf_("direct: job %s was queued during the walk; dropped", jn && !IsBadStringPtrA(jn, 64) ? jn : "?");
            }
            GoMind_ClearQ(mind, JQ_ACTION);
        }
        if (nema && Aspect_GetCurrentChore(nema) != CHORE_WALK && Aspect_GetNextChore(nema) != CHORE_WALK) {
            direct_anim(hero, CHORE_WALK);
            if (g_directLogs < 30) { g_directLogs++; logf_("direct: walk animation re-applied"); }
        }
    }
}

/* fallback movement: ordinary move orders a few metres ahead, refreshed as the stick turns */
static int try_move(void *hero, const SiegePos *hp, float dx, float dz, float dist)
{
    static const float fan[] = { 0.0f, 0.3f, -0.3f, 0.65f, -0.65f };
    static const float scale[] = { 1.0f, 0.6f, 0.3f };
    void *mind = Go_GetMind(hero);
    for (int pass = 0; pass < 3; pass++) {
        float d = dist * scale[pass];
        for (int i = 0; i < 5; i++) {
            float c = cosf(fan[i]), s = sinf(fan[i]);
            SiegePos t = *hp;
            if (g_directOK) {
                t.x += (dx * c - dz * s) * d;
                t.z += (dx * s + dz * c) * d;
                if (!GoBody_IsPositionWalkable(Go_GetBody(hero), &t)) continue;
            } else {
                t.x += (dx * c - dz * s) * d;
                t.z += (dx * s + dz * c) * d;
            }
            if (GoMind_RSMove(mind, &t, QP_CLEAR, AO_USER)) return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ targets */
static void *g_targetGoid, *g_atkGoid;
static DWORD g_lastAttackCheck;
typedef struct { void *goid; int kind; float d2; } Cand;
static Cand g_cand[48];
static int g_ncand, g_candRec;
static void *g_lockGoid;
static int g_lockKind;
static void cand_add(void *goid, int kind, float d2)
{
    if (!g_candRec || g_ncand >= 48) return;
    g_cand[g_ncand].goid = goid;
    g_cand[g_ncand].kind = kind;
    g_cand[g_ncand].d2 = d2;
    g_ncand++;
}
static struct { void *goid; DWORD until; } g_skip[6];
static void *g_lastUseGoid;
static DWORD g_lastUseTime;
static int skipped(void *goid, DWORD now)
{
    for (int i = 0; i < 6; i++)
        if (g_skip[i].goid == goid && (int)(g_skip[i].until - now) > 0) return 1;
    return 0;
}
static void note_use(void *goid, DWORD now)
{
    if (goid == g_lastUseGoid && now - g_lastUseTime < 4000 && now - g_lastUseTime > 300) {
        int slot = 0;
        for (int i = 0; i < 6; i++)
            if ((int)(g_skip[i].until - now) <= 0) { slot = i; break; }
        g_skip[slot].goid = goid;
        g_skip[slot].until = now + 12000;
        logf_("X: target %p did not respond twice; skipping it for a while", goid);
        g_lastUseGoid = NULL;
        return;
    }
    g_lastUseGoid = goid;
    g_lastUseTime = now;
}

/* why the last look for an enemy left hostile creatures out (for the log) */
static int g_enemySeen, g_enemyNoLos, g_enemyNotInMind, g_enemyHidden;
static char g_enemyNote[96];
static void *pick_enemy(void *hero, const SiegePos *hp, int haveDir, float dx, float dz, void **current)
{
    DWORD nowT = timeGetTime();
    void *mind = Go_GetMind(hero);
    void *aiq = AIQuery_Get();
    if (current) *current = NULL;
    g_enemySeen = g_enemyNoLos = g_enemyNotInMind = g_enemyHidden = 0;
    g_enemyNote[0] = 0;
    /* what the hero's own senses list as enemies; they are refreshed only now and then and miss
       creatures the hero has not "noticed" yet, so this is only half of the picture */
    void *known[128];
    int nk = 0;
    g_coll.e = g_coll.b;
    GoMind_GetEnemiesInSphere(mind, c_attackRadius, &g_coll);
    int n = (int)(g_coll.e - g_coll.b);
    if (n > 0 && n <= 4096 && rd(g_coll.b, n * sizeof(void *)))
        for (int i = 0; i < n && nk < 128; i++) known[nk++] = g_coll.b[i];
    /* the other half: everything standing within reach that is hostile to the hero */
    void *all[160];
    int na = 0;
    for (int i = 0; i < nk; i++) all[na++] = known[i];
    if (rd(aiq, 0x10)) {
        g_coll.e = g_coll.b;
        AIQuery_GetOccupantsInSphere(aiq, hp, c_attackRadius, &g_coll);
        n = (int)(g_coll.e - g_coll.b);
        if (n > 0 && n <= 8192 && rd(g_coll.b, n * sizeof(void *)))
            for (int i = 0; i < n && na < 160; i++) {
                void *g = g_coll.b[i];
                int dup = 0;
                if (!rd(g, 0x100) || g == hero || !Go_IsActor(g) || Go_IsScreenPartyMember(g)) continue;
                for (int k = 0; k < nk; k++) dup |= known[k] == g;
                if (!dup) all[na++] = g;
            }
    }
    void *best = NULL, *lockHit = NULL;
    float bestScore = 1e9f;
    for (int i = 0; i < na; i++) {
        void *g = all[i];
        SiegePos ep;
        if (!go_alive(g) || g == hero || !go_pos(g, &ep)) continue;
        if (i >= nk && !GoMind_IsEnemy(mind, g)) continue;
        g_enemySeen++;
        const char *why = NULL;
        void *asp = Go_GetAspect ? Go_GetAspect(g) : NULL;
        if (rd(asp, 0x48) && GoAspect_GetIsVisible && !GoAspect_GetIsVisible(asp)) { g_enemyHidden++; why = "not visible"; }
        /* The game's line-of-sight test fails for some things that can plainly be attacked (the
           tutorial's bracken pod, for one). So it no longer rules an enemy out; it only ranks enemies
           in plain view ahead of the others. */
        int noLos = !why && GoMind_IsLosClear && !GoMind_IsLosClear(mind, g);
        if (noLos) g_enemyNoLos++;
        if (why) {
            if (!g_enemyNote[0]) {
                const char *tn = Go_GetTemplateName ? Go_GetTemplateName(g) : NULL;
                snprintf(g_enemyNote, sizeof g_enemyNote, "%s: %s", tn && !IsBadStringPtrA(tn, 48) ? tn : "?", why);
            }
            continue;
        }
        if (i >= nk) g_enemyNotInMind++;
        if (skipped(Go_GetGoid(g), nowT)) continue;
        if (current && g_targetGoid && Go_GetGoid(g) == g_targetGoid) *current = g;
        Vec3 d = { 0, 0, 0 };
        GetSiegeDifference(&d, hp, &ep);
        float dist = sqrtf(d.x * d.x + d.z * d.z);
        float score = dist;
        if (haveDir && dist > 0.05f) {
            float c = (d.x * dx + d.z * dz) / dist;
            score = dist * (1.0f + 1.5f * (1.0f - c));
        }
        if (noLos) score += 1000.0f;
        cand_add(Go_GetGoid(g), 0, dist * dist + (noLos ? 1.0e6f : 0.0f));
        if (g_lockGoid && g_lockKind == 0 && Go_GetGoid(g) == g_lockGoid) lockHit = g;
        if (score < bestScore) { bestScore = score; best = g; }
    }
    return lockHit ? lockHit : best;
}

static int go_clickable(void *g)
{
    void *asp = Go_GetAspect ? Go_GetAspect(g) : NULL;
    if (!rd(asp, 0x48)) return 1;
    if (GoAspect_GetIsVisible && !GoAspect_GetIsVisible(asp)) return 0;
    if (GoAspect_GetIsSelectable && !GoAspect_GetIsSelectable(asp)) return 0;
    return 1;
}
/* kind: 1 item on the ground, 2 usable object (door, chest, lever), 3 someone to talk to, 4 breakable */
static void *pick_interact(void *hero, const SiegePos *hp, int *kindOut)
{
    DWORD nowT = timeGetTime();
    void *mind = Go_GetMind(hero);
    void *aiq = AIQuery_Get();
    if (!rd(aiq, 0x10)) return NULL;
    g_coll.e = g_coll.b;
    AIQuery_GetOccupantsInSphere(aiq, hp, c_interactRadius, &g_coll);
    int n = (int)(g_coll.e - g_coll.b);
    if (n <= 0 || n > 8192 || !rd(g_coll.b, n * sizeof(void *))) return NULL;
    void *best = NULL, *lockHit = NULL;
    int bestKind = 0, lockKind = 0;
    float bestD = 1e9f;
    for (int i = 0; i < n; i++) {
        void *g = g_coll.b[i];
        SiegePos p;
        int kind = 0;
        if (!rd(g, 0x100) || g == hero || !go_pos(g, &p)) continue;
        if (Go_IsBreakable && Go_IsBreakable(g) && !Go_IsActor(g)) {
            kind = 4;
        } else if (!Go_IsActor(g) && Go_IsUsable(g) && !Go_IsInsideInventory(g)) {
            kind = 2;
        } else if (Go_IsItem(g)) {
            if (!Go_IsInsideInventory(g)) kind = 1;
        } else if (Go_IsActor(g)) {
            if (!Go_IsScreenPartyMember(g) && go_alive(g) && (Go_HasConversation(g) || Go_HasStore(g)) &&
                !GoMind_IsEnemy(mind, g))
                kind = 3;
        } else if (Go_IsUsable(g)) {
            kind = 2;
        }
        if (!kind) continue;
        if (kind != 3 && !go_clickable(g)) continue;
        if (skipped(Go_GetGoid(g), nowT)) continue;
        Vec3 d = { 0, 0, 0 };
        GetSiegeDifference(&d, hp, &p);
        float dist = d.x * d.x + d.z * d.z;
        if (kind == 3) dist *= 0.6f;
        cand_add(Go_GetGoid(g), kind, dist);
        if (g_lockGoid && g_lockKind != 0 && Go_GetGoid(g) == g_lockGoid) { lockHit = g; lockKind = kind; }
        if (kind == 2) dist *= 0.5f;
        if (dist < bestD) { bestD = dist; best = g; bestKind = kind; }
    }
    if (lockHit) { *kindOut = lockKind; return lockHit; }
    if (best) *kindOut = bestKind;
    return best;
}

static void dump_nearby(void *hero, const SiegePos *hp)
{
    static int dumps;
    void *aiq = AIQuery_Get();
    if (dumps >= 4 || !rd(aiq, 0x10)) return;
    dumps++;
    g_coll.e = g_coll.b;
    AIQuery_GetOccupantsInSphere(aiq, hp, 9.0f, &g_coll);
    int n = (int)(g_coll.e - g_coll.b);
    logf_("nothing found; %d objects within 9 m:", n);
    if (n <= 0 || n > 8192 || !rd(g_coll.b, n * sizeof(void *))) return;
    for (int i = 0; i < n && i < 40; i++) {
        void *g = g_coll.b[i];
        SiegePos p;
        Vec3 d = { 0, 0, 0 };
        if (!rd(g, 0x100) || g == hero) continue;
        int hasPos = go_pos(g, &p);
        if (hasPos) GetSiegeDifference(&d, hp, &p);
        const char *tn = Go_GetTemplateName ? Go_GetTemplateName(g) : NULL;
        logf_("   %s dist %.1f item %d actor %d usable %d breakable %d inInv %d", tn && !IsBadStringPtrA(tn, 64) ? tn : "?",
              hasPos ? sqrtf(d.x * d.x + d.z * d.z) : -1.0f, Go_IsItem(g), Go_IsActor(g), Go_IsUsable(g),
              Go_IsBreakable ? Go_IsBreakable(g) : -1, Go_IsInsideInventory(g));
    }
}

static bool8 (*IsAttackJat)(int);
static bool8 (*IsCastJat)(int);
/* is the hero in the middle of an attack (or a cast) right now? idle fidgeting does not count */
static int attack_running(void *mind)
{
    int *job = GoMind_GetFrontJob(mind, JQ_ACTION);
    if (!rd(job, 4)) return 0;
    if (!IsAttackJat) {
        IsAttackJat = (void *)GetProcAddress(g_exe, "?IsAttackJat@@YA_NW4eJobAbstractType@@@Z");
        IsCastJat = (void *)GetProcAddress(g_exe, "?IsCastJat@@YA_NW4eJobAbstractType@@@Z");
    }
    if (!IsAttackJat || !IsCastJat) return 1;
    return IsAttackJat(*job) || IsCastJat(*job);
}
/* "Mirror" party orders. In the game it is the mouse-click handler that makes the companions join in:
   under Mirror orders a click on an enemy gives every party member the attack, each with what it has
   selected - a spell is cast if it can be cast on that target, a weapon attacks. The pad's attack goes
   straight to the hero and never passes that handler, so the same is done here. */
static void mirror_attack(void *hero, void *enemy)
{
    static struct {
        int ok, mirror;
        void *(TC *getParty)(void *);
        int (TC *getOrders)(void *);
        const char *(*ordName)(int);
        void *(TC *getInv)(void *), *(TC *getMagic)(void *);
        bool8 (TC *spellSel)(void *);
        void *(TC *selSpell)(void *);
        bool8 (TC *castable)(void *, void *, bool8);
        bool8 (TC *mayAttack)(void *, void *, bool8);
        void *(TC *lastEngaged)(void *);
        bool8 (TC *rsCast)(void *, void *, void *, int, int);
    } f;
    static struct { void *member, *target; DWORD t; } sent[16];
    static int logs;
    if (!f.ok) {
        f.getParty = (void *)GetProcAddress(g_exe, "?GetParty@Go@@QAEPAVGoParty@@XZ");
        f.getOrders = (void *)GetProcAddress(g_exe, "?GetStandingOrders@GoParty@@QBE?AW4eStandingOrders@@XZ");
        f.ordName = (void *)GetProcAddress(g_exe, "?ToString@@YAPBDW4eStandingOrders@@@Z");
        f.getInv = (void *)GetProcAddress(g_exe, "?GetInventory@Go@@QAEPAVGoInventory@@XZ");
        f.getMagic = (void *)GetProcAddress(g_exe, "?GetMagic@Go@@QAEPAVGoMagic@@XZ");
        f.spellSel = (void *)GetProcAddress(g_exe, "?IsSpellSelected@GoInventory@@QBE_NXZ");
        f.selSpell = (void *)GetProcAddress(g_exe, "?GetSelectedSpell@GoInventory@@QBEPAVGo@@XZ");
        f.castable = (void *)GetProcAddress(g_exe, "?IsCastableOn@GoMagic@@QAE_NPAVGo@@_N@Z");
        f.mayAttack = (void *)GetProcAddress(g_exe, "?MayAttackTarget@GoMind@@QBE_NPBVGo@@_N@Z");
        f.lastEngaged = (void *)GetProcAddress(g_exe, "?GetLastEngagedObject@GoMind@@QBEPBUGoid_@@XZ");
        f.rsCast = (void *)GetProcAddress(g_exe, "?RSCast@GoMind@@QAE_NPBUGoid_@@0W4eQPlace@@W4eActionOrigin@@@Z");
        f.mirror = f.ordName ? enum_find(f.ordName, "so_mirror") : -1;
        f.ok = f.getParty && f.getOrders && f.getInv && f.getMagic && f.spellSel && f.selSpell && f.castable && f.mayAttack &&
               f.lastEngaged && f.rsCast && f.mirror >= 0 ? 1 : -1;
        logf_("mirror orders: link %d (mirror = %d)", f.ok, f.mirror);
    }
    if (f.ok != 1 || !Go_IsActor(enemy)) return;
    void *party = get_party();
    void *gp = party ? f.getParty(party) : NULL;
    if (!rd(gp, 0x40)) return;
    int orders = f.getOrders(gp);
    if (orders != f.mirror) {
        static int ol;
        if (ol < 3) {
            const char *on = f.ordName(orders);
            ol++;
            logf_("mirror orders: the party's orders are %s, so the companions are left to themselves", on && !IsBadStringPtrA(on, 32) ? on : "?");
        }
        return;
    }
    GopColl *kids = Go_GetChildren(party);
    if (!rd(kids, 8)) return;
    int n = (int)(kids->e - kids->b);
    if (n <= 1 || n > 64 || !rd(kids->b, n * sizeof(void *))) return;
    void *members[64], *tgoid = Go_GetGoid(enemy);
    DWORD now = timeGetTime();
    memcpy(members, kids->b, n * sizeof(void *));
    for (int i = 0; i < n; i++) {
        void *m = members[i];
        if (m == hero || !go_alive(m)) continue;
        void *mind = Go_GetMind(m), *inv = f.getInv(m);
        if (!rd(mind, 0x100) || !rd(inv, 0x40)) continue;
        if (f.lastEngaged(mind) == tgoid && attack_running(mind)) continue; /* already on it */
        int si = i & 15;
        if (sent[si].member == m && sent[si].target == tgoid && now - sent[si].t < 1500) continue;
        sent[si].member = m; sent[si].target = tgoid; sent[si].t = now;
        const char *what = "may not attack that target";
        const char *sn = NULL;
        bool8 r = 0;
        if (f.spellSel(inv)) {
            void *spell = f.selSpell(inv);
            void *mg = rd(spell, 0x100) ? f.getMagic(spell) : NULL;
            sn = rd(spell, 0x100) && Go_GetTemplateName ? Go_GetTemplateName(spell) : NULL;
            if (rd(mg, 0x40) && f.castable(mg, enemy, 0)) { r = f.rsCast(mind, tgoid, Go_GetGoid(spell), QP_CLEAR, AO_USER); what = "casts its selected spell"; }
            else what = "has a spell selected that cannot be cast on that target; nothing ordered";
        } else if (f.mayAttack(mind, enemy, 1)) {
            r = GoMind_RSAttack(mind, tgoid, QP_CLEAR, AO_USER);
            what = "attacks with its weapon";
        }
        if (logs < 30) {
            const char *mn = Go_GetTemplateName ? Go_GetTemplateName(m) : NULL, *en = Go_GetTemplateName ? Go_GetTemplateName(enemy) : NULL;
            logs++;
            logf_("mirror orders: %s %s%s%s%s (target %s) -> %d", mn && !IsBadStringPtrA(mn, 64) ? mn : "?", what, sn ? " [" : "",
                  sn && !IsBadStringPtrA(sn, 64) ? sn : "", sn ? "]" : "", en && !IsBadStringPtrA(en, 64) ? en : "?", r);
        }
    }
}
static void issue_attack(void *hero, void *enemy)
{
    direct_handover();
    bool8 r = GoMind_RSAttack(Go_GetMind(hero), Go_GetGoid(enemy), QP_CLEAR, AO_USER);
    mirror_attack(hero, enemy);
    static int logs;
    if (logs < 20) {
        const char *tn = Go_GetTemplateName ? Go_GetTemplateName(enemy) : NULL;
        logs++;
        logf_("attack order on %s -> %d", tn && !IsBadStringPtrA(tn, 64) ? tn : "?", r);
    }
    g_targetGoid = g_atkGoid = Go_GetGoid(enemy);
}

static int do_interact(void *hero, const SiegePos *hp)
{
    void *mind = Go_GetMind(hero);
    int kind = 0;
    void *best = pick_interact(hero, hp, &kind);
    if (!best) return 0;
    if (kind == 1) note_use(Go_GetGoid(best), timeGetTime());
    if (g_lockGoid && g_lockKind != 0 && Go_GetGoid(best) == g_lockGoid) g_lockGoid = NULL;
    g_atkGoid = NULL;
    {
        static int logs;
        if (logs < 30) {
            const char *tn = Go_GetTemplateName ? Go_GetTemplateName(best) : NULL;
            logs++;
            logf_("interact: kind %d (1 pick up, 2 use, 3 talk, 4 break) on %s", kind, tn && !IsBadStringPtrA(tn, 64) ? tn : "?");
        }
    }
    direct_handover();
    if (kind == 4) issue_attack(hero, best);
    else if (kind == 1) GoMind_RSGet(mind, Go_GetGoid(best), QP_CLEAR, AO_USER);
    else if (kind == 2) GoMind_RSUse(mind, Go_GetGoid(best), QP_CLEAR, AO_USER);
    else {
        void *uc = UICommands_Get();
        if (rd(uc, 4)) UICommands_RSTalk(uc, best, hero, AO_USER);
    }
    return 1;
}

static int g_shutdown, g_noBattery;
static void install_battery_fix(void);
/* ------------------------------------------------------------------ item stats box and comparison
   The game builds an item's stats text and puts it in its hover box (the box the inventory shows
   under the pointer) in one routine. That routine is used as it is:
   - for the item on the ground the pad is aiming at, the mod calls it and places the box itself;
   - whenever it runs (inventory, shops, the ground box), the stats of what the controlled hero wears
     in the same slot are added below, so the two can be compared. */
#define FN_TIP_FILL  0x48e0b4 /* inventory manager: (item goid, text box, for-focus flag) */
#define FN_TIP_SET   0x75f780 /* text box: set the stats text (wide string, by value) */
#define FN_TIP_MAKE  0x8081fe /* item gui: build the stats text (out string, actor) */
#define FN_WSTR_ADD  0x489284 /* wide string: append (count, character) */
#define FN_WSTR_FREE 0x40146d
static void (TC *g_origTipFill)(void *, void *, void *, int);
static void *g_tipGoid;
static int g_tipOK;
static struct {
    void *(TC *getGui)(void *);
    int (TC *equipSlot)(void *);
    void *(TC *getInv)(void *);
    void *(TC *getEquipped)(void *, int);
    void *(*invMgr)(void);
    char *(TC *findWin)(void *, const char *, const char *, int);
} g_ii;
static void *goid_go(void *goid)
{
    void *go = NULL;
    ((void *(TC *)(void *, void *))0x7b3c52)(&go, goid); /* the game's goid -> object lookup */
    return rd(go, 0x100) ? go : NULL;
}
static void wappend(WORD **str, const WORD *t, int n)
{
    for (int i = 0; i < n; i++) ((void (TC *)(void *, int, int))FN_WSTR_ADD)(str, 1, t[i]);
}
static void append_equipped(WORD **str, void *go)
{
    if (!c_compare || !rd(go, 0x100)) return;
    void *gui = g_ii.getGui(go);
    if (!rd(gui, 0x40)) return;
    int slot = g_ii.equipSlot(gui);
    if (slot < 0 || slot > 12) return; /* 13 = nothing that is worn: potions, gold, reagents ... */
    void *hero = get_leader();
    char *inv = hero ? g_ii.getInv(hero) : NULL;
    if (!rd(inv, 0x80)) return;
    int first = slot == 12 ? 8 : slot, last = slot == 12 ? 11 : slot; /* 12 = any ring finger */
    for (int sl = first; sl <= last; sl++) {
        void *eq = g_ii.getEquipped(inv, sl);
        if (!rd(eq, 0x100) || eq == go) continue;
        void *egui = g_ii.getGui(eq);
        if (!rd(egui, 0x40)) continue;
        WORD *es = NULL;
        *(void **)&es = (void *)0xd0157c; /* the game's empty string */
        ((void *(TC *)(void *, WORD **, void *))FN_TIP_MAKE)(egui, &es, NULL);
        int n = 0;
        if (rd(es, 2)) while (n < 3000 && rd(es + n, 2) && es[n]) n++;
        if (n > 0) {
            static const WORD head[] = { '\n', ' ', '\n', '-', '-', ' ', 'E', 'q', 'u', 'i', 'p', 'p', 'e', 'd', ' ', '-', '-', '\n' };
            wappend(str, head, sizeof head / sizeof head[0]);
            wappend(str, es, n);
        }
        ((void (TC *)(void *))FN_WSTR_FREE)(&es);
    }
}
static void TC hk_tip_fill(void *mgr, void *goid, void *tb, int flag)
{
    void *prev = g_tipGoid;
    g_tipGoid = goid;
    g_origTipFill(mgr, goid, tb, flag);
    g_tipGoid = prev;
}
/* the finished stats text on its way into the box */
static void TC hk_tip_set(void *tb, WORD *text)
{
    if (c_compare && !g_shutdown && g_tipGoid && rd(text, 2)) {
        void *go = goid_go(g_tipGoid);
        if (go) append_equipped(&text, go);
    }
    ((void (TC *)(void *, WORD *))FN_TIP_SET)(tb, text);
}
static void install_item_info(void)
{
    if (!c_itemInfo && !c_compare) return;
    g_ii.getGui = (void *)GetProcAddress(g_exe, "?GetGui@Go@@QAEPAVGoGui@@XZ");
    g_ii.equipSlot = (void *)GetProcAddress(g_exe, "?GetEquipSlot@GoGui@@QBE?AW4eEquipSlot@@XZ");
    g_ii.getInv = (void *)GetProcAddress(g_exe, "?GetInventory@Go@@QAEPAVGoInventory@@XZ");
    g_ii.getEquipped = (void *)GetProcAddress(g_exe, "?GetEquipped@GoInventory@@QBEPAVGo@@W4eEquipSlot@@@Z");
    g_ii.invMgr = (void *)GetProcAddress(g_exe, "?FUBI_GetClassSingleton@UIInventoryManager@@CAPAV1@XZ");
    g_ii.findWin = (void *)GetProcAddress(g_exe, "?FindUIWindow@UIShell@@QAEPAVUIWindow@@PBD0W4eUIWindowExistanceState@@@Z");
    int code = code_is(FN_TIP_FILL, "558bec83ec0c535657ff7508") && code_is(0x48e2cd, "8b4d0ce8") && code_is(FN_TIP_SET, "558bec83ec14") &&
               code_is(FN_TIP_MAKE, "558bec81ec88000000") && code_is(FN_WSTR_FREE, "56578bf98b07") && code_is(0x7b3c52, "558bec") &&
               code_is(FN_WSTR_ADD, "ff25");
    if (!g_ii.getGui || !g_ii.equipSlot || !g_ii.getInv || !g_ii.getEquipped || !g_ii.invMgr || !g_ii.findWin || !code) {
        logf_("item stats box: the game's routines are not as expected (code %d); feature off", code);
        return;
    }
    g_origTipFill = hook_function(FN_TIP_FILL, "558bec83ec0c", hk_tip_fill);
    int site = g_origTipFill ? patch_call(0x48e2d0, FN_TIP_SET, hk_tip_set) : 0;
    g_tipOK = g_origTipFill && site;
    logf_("item stats box: hooks %d/%d, ground box %d, comparison %d", g_origTipFill != NULL, site, c_itemInfo, c_compare);
}
/* the ground box */
static void *g_infoGoid;
static int g_infoShown;
static int info_windows(char **tb, char **bg)
{
    void *shell = UIShell_Get ? UIShell_Get() : NULL;
    if (!rd(shell, 0x80)) return 0;
    *tb = g_ii.findWin(shell, "gui_rollover_textbox", NULL, 0);
    *bg = g_ii.findWin(shell, "gui_rollover_bg", "rollover_help", 0);
    return rd(*tb, 0x100) && rd(*(void **)*tb, 0x58) && rd(*bg, 0x100) && rd(*(void **)*bg, 0x58);
}
static void win_visible(char *w, int on)
{
    ((void (TC *)(void *, int))(*(void ***)w)[0x54 / 4])(w, on);
}
static void item_info_hide(void)
{
    char *tb, *bg;
    if (!g_infoShown) return;
    g_infoShown = 0;
    g_infoGoid = NULL;
    if (g_tipOK && info_windows(&tb, &bg)) { win_visible(tb, 0); win_visible(bg, 0); }
}
static void item_info_update(void *go, int kind)
{
    char *tb, *bg;
    if (!c_itemInfo || !g_tipOK) return;
    void *gui = (go && kind == 1) ? g_ii.getGui(go) : NULL;
    int slot = rd(gui, 0x40) ? g_ii.equipSlot(gui) : -1;
    if (slot < 0 || slot > 12) { item_info_hide(); return; }
    if (!info_windows(&tb, &bg)) return;
    void *goid = Go_GetGoid(go);
    void *mgr = g_ii.invMgr();
    if (!rd(mgr, 0x40)) return;
    int fresh = goid != g_infoGoid || !g_infoShown;
    if (fresh || !*(BYTE *)(tb + 0xb0)) {
        win_visible(tb, 1);
        win_visible(bg, 1);
        ((void (TC *)(void *, void *, void *, int))FN_TIP_FILL)(mgr, goid, tb, 0);
        if (!*(BYTE *)(tb + 0xb0)) { /* no text for this item: the routine hid the box itself */
            win_visible(bg, 0);
            g_infoShown = 0;
            g_infoGoid = NULL;
            return;
        }
        static int logs;
        if (logs < 6 && fresh) {
            const char *tn = Go_GetTemplateName ? Go_GetTemplateName(go) : NULL;
            logs++;
            logf_("item stats box: shown for %s (slot %d)", tn && !IsBadStringPtrA(tn, 64) ? tn : "?", slot);
        }
        g_infoGoid = goid;
    }
    {
        char *shell = UIShell_Get();
        int *r = (int *)(tb + 0x60);
        int W = *(int *)(shell + 0x78), H = *(int *)(shell + 0x7c);
        int w = r[2] - r[0], h = r[3] - r[1];
        if (w > 0 && h > 0 && W > 0 && H > 0) {
            int x = W * c_infoX / 100, y = H * c_infoY / 100 - h / 2;
            if (x + w > W - 4) x = W - 4 - w;
            if (y + h > H - 4) y = H - 4 - h;
            if (x < 4) x = 4;
            if (y < 4) y = 4;
            if (r[0] != x || r[1] != y) {
                /* the window's own "set rectangle" (left, right, top, bottom), then the frame behind it */
                ((void (TC *)(void *, int, int, int, int, int))(*(void ***)tb)[0x40 / 4])(tb, x, x + w, y, y + h, 0);
                ((void (TC *)(void *, int, int, int, int, int))(*(void ***)bg)[0x40 / 4])(bg, x, x + w, y, y + h, 0);
            }
        }
    }
    g_infoShown = 1;
}

/* One choice for the attack button: any enemy in view and in range beats loot, doors and NPCs. */
static void *pick_target(void *hero, const SiegePos *hp, int haveDir, float dx, float dz, int *kind)
{
    void *cur = NULL;
    void *e = pick_enemy(hero, hp, haveDir, dx, dz, &cur);
    if (e && g_lockGoid && g_lockKind != 0) g_lockGoid = NULL;
    if (g_lockGoid && g_lockKind != 0) {
        int lk = 0;
        void *li = pick_interact(hero, hp, &lk);
        if (li && Go_GetGoid(li) == g_lockGoid) { *kind = lk; return li; }
        g_lockGoid = NULL;
    }
    if (g_lockGoid && g_lockKind == 0) {
        if (e && Go_GetGoid(e) == g_lockGoid) { *kind = 0; return e; }
        g_lockGoid = NULL;
    }
    if (cur) { *kind = 0; return cur; }
    if (e) {
        static int ml;
        static DWORD mlast;
        DWORD t = timeGetTime();
        if (g_enemyNotInMind && ml < 15 && t - mlast > 2000) {
            ml++;
            mlast = t;
            logf_("target: %d of %d hostile creatures in reach were not on the hero's own enemy list; they count as targets now", g_enemyNotInMind, g_enemySeen);
        }
        *kind = 0;
        return e;
    }
    int ik = 0;
    int seen = g_enemySeen, noLos = g_enemyNoLos, hidden = g_enemyHidden;
    char note[96];
    memcpy(note, g_enemyNote, sizeof note);
    void *it = pick_interact(hero, hp, &ik);
    if (it && seen) {
        static int tl;
        static DWORD tlast;
        DWORD t = timeGetTime();
        if (tl < 30 && t - tlast > 1500) {
            const char *tn = Go_GetTemplateName ? Go_GetTemplateName(it) : NULL;
            tl++;
            tlast = t;
            logf_("target: %s (kind %d) chosen although %d hostile creatures are in reach - %d not visible (%s)",
                  tn && !IsBadStringPtrA(tn, 64) ? tn : "?", ik, seen, hidden, note);
            (void)noLos;
        }
    }
    if (it) { *kind = ik; return it; }
    return NULL;
}

/* Right stick click: step to the next candidate target, nearest first. */
static void cycle_target(void *hero, const SiegePos *hp)
{
    void *prevLock = g_lockGoid;
    int k = 0;
    g_lockGoid = NULL;
    g_ncand = 0;
    g_candRec = 1;
    pick_enemy(hero, hp, 0, 0, 0, NULL);
    if (!g_ncand) pick_interact(hero, hp, &k);
    g_candRec = 0;
    if (!g_ncand) return;
    for (int i = 1; i < g_ncand; i++) {
        Cand c = g_cand[i];
        int j = i - 1;
        while (j >= 0 && g_cand[j].d2 > c.d2) { g_cand[j + 1] = g_cand[j]; j--; }
        g_cand[j + 1] = c;
    }
    void *cur = prevLock ? prevLock : (g_hlGoid ? g_hlGoid : g_atkGoid);
    int idx = -1;
    for (int i = 0; i < g_ncand; i++)
        if (g_cand[i].goid == cur) idx = i;
    int next = (idx + 1) % g_ncand;
    g_lockGoid = g_cand[next].goid;
    g_lockKind = g_cand[next].kind;
    static int logs;
    if (logs < 20) { logs++; logf_("target cycle: %d of %d candidates (kind %d)", next + 1, g_ncand, g_lockKind); }
}

/* ------------------------------------------------------------------ weapon / spell cycling
   The hero has two weapon sets and four spell slots. Y steps to the next one that has
   something in it (what clicking the slot on the character panel does). */
static void cycle_active(void *hero)
{
    static void *(TC *getInv)(void *);
    static int (TC *getSel)(void *);
    static void *(TC *getItem)(void *, int);
    static void *(TC *rsSelect)(void *, int, int);
    static const char *(*ilName)(int);
    enum { NLOC = 6 };
    static int loc[NLOC] = { -1, -1, -1, -1, -1, -1 }, ready;
    static const char *const names[NLOC] = { "il_hand_1", "il_hand_2", "il_spell_1", "il_spell_2", "il_spell_3", "il_spell_4" };
    if (!ready) {
        ready = 1;
        getInv = (void *)GetProcAddress(g_exe, "?GetInventory@Go@@QAEPAVGoInventory@@XZ");
        getSel = (void *)GetProcAddress(g_exe, "?GetSelectedLocation@GoInventory@@QBE?AW4eInventoryLocation@@XZ");
        getItem = (void *)GetProcAddress(g_exe, "?GetItem@GoInventory@@QBEPAVGo@@W4eInventoryLocation@@@Z");
        rsSelect = (void *)GetProcAddress(g_exe, "?RSSelect@GoInventory@@QAEPAUCookie__@FuBi@@W4eInventoryLocation@@W4eActionOrigin@@@Z");
        ilName = (void *)GetProcAddress(g_exe, "?ToString@@YAPBDW4eInventoryLocation@@@Z");
        for (int i = 0; i < NLOC; i++) loc[i] = enum_find(ilName, names[i]);
        logf_("active slots: weapon sets %d %d, spells %d %d %d %d (link %d)", loc[0], loc[1], loc[2], loc[3], loc[4], loc[5], getInv && getSel && getItem && rsSelect);
    }
    if (!getInv || !getSel || !getItem || !rsSelect) return;
    void *inv = getInv(hero);
    if (!rd(inv, 0x94)) return;
    int cur = getSel(inv), at = -1;
    for (int i = 0; i < NLOC; i++)
        if (loc[i] == cur) at = i;
    for (int k = 1; k <= NLOC; k++) {
        int i = (at + k + NLOC) % NLOC;
        if (loc[i] < 0 || loc[i] == cur) continue;
        if (i != 0 && !getItem(inv, loc[i])) continue; /* empty slot (the first weapon set is always there: bare hands) */
        rsSelect(inv, loc[i], AO_USER);
        static int logs;
        if (logs < 12) { logs++; logf_("active slot: %s -> %s", at >= 0 ? names[at] : "?", names[i]); }
        return;
    }
}

/* ------------------------------------------------------------------ party auto potions
   companions (never the controlled hero) drink a potion of their own when health or mana runs low */
#define AP_TESTGET_SYM "?TestGet@GoInventory@@QBE_NPBUGoid_@@_N@Z"
#define AP_LIST_SYM "?ListItems@GoInventory@@QBE_NW4eInventoryLocation@@AAUGopColl@@@Z"
#define AP_IL_MAIN 16
#define AP_GOLD_DROPPER 1
#define AP_ORIGIN AO_USER
static bool8 (TC *g_arCast)(void *, void *, void *, int, int);
#define AR_CAN_CAST ((g_arCast = (void *)GetProcAddress(g_exe, "?RSCast@GoMind@@QAE_NPBUGoid_@@0W4eQPlace@@W4eActionOrigin@@@Z")) != NULL)
#define AR_CAST(mind, target, spell) g_arCast(mind, target, spell, QP_CLEAR, AO_USER)
/* ------------------------------------------------------------------ automatic resurrection
   When a party member is dead and someone alive in the party carries a resurrect scroll, that member
   uses it on the fallen one - once no enemy is near the caster. Only companions do this; the hero the
   player controls never casts on its own. */
static void auto_resurrect(void *leader, DWORD now)
{
    static struct {
        int ok;
        void *(TC *getInv)(void *), *(TC *getMagic)(void *);
        bool8 (TC *isSpell)(void *);
        bool8 (TC *oneUse)(void *);
        bool8 (TC *castable)(void *, void *, bool8);
        bool8 (TC *listItems)(void *, int, GopColl *);
        float (TC *curMana)(void *);
        bool8 (TC *marked)(void *), (TC *inWorld)(void *);
    } f;
    static struct { void *goid; DWORD t; } tried[8];
    static DWORD last;
    static int logs;
    if (!c_autoRes || now - last < 1000) return;
    last = now;
    if (!f.ok) {
        f.getInv = (void *)GetProcAddress(g_exe, "?GetInventory@Go@@QAEPAVGoInventory@@XZ");
        f.getMagic = (void *)GetProcAddress(g_exe, "?GetMagic@Go@@QAEPAVGoMagic@@XZ");
        f.isSpell = (void *)GetProcAddress(g_exe, "?IsSpell@Go@@QBE_NXZ");
        f.oneUse = (void *)GetProcAddress(g_exe, "?GetIsOneUse@GoMagic@@QBE_NXZ");
        f.castable = (void *)GetProcAddress(g_exe, "?IsCastableOn@GoMagic@@QAE_NPAVGo@@_N@Z");
        f.listItems = (void *)GetProcAddress(g_exe, AP_LIST_SYM);
        f.curMana = (void *)GetProcAddress(g_exe, "?GetCurrentMana@GoAspect@@QBEMXZ");
        f.marked = (void *)GetProcAddress(g_exe, "?IsMarkedForDeletion@Go@@QBE_NXZ");
        f.inWorld = (void *)GetProcAddress(g_exe, "?IsInAnyWorldFrustum@Go@@QBE_NXZ");
        f.ok = f.getInv && f.getMagic && f.isSpell && f.oneUse && f.castable && f.listItems && f.curMana && f.marked && f.inWorld && AR_CAN_CAST ? 1 : -1;
        logf_("auto resurrect: link %d", f.ok);
    }
    if (f.ok != 1) return;
    void *party = get_party();
    GopColl *kids = party ? Go_GetChildren(party) : NULL;
    int n = rd(kids, 8) ? (int)(kids->e - kids->b) : 0;
    void *members[16];
    if (n <= 1 || n > 16 || !rd(kids->b, n * sizeof(void *))) return;
    memcpy(members, kids->b, n * sizeof(void *));
    /* Leaving the game takes the party apart piece by piece, and a member that is half gone also reads
       as "not alive". Nothing is done unless every member is still whole and in the world. */
    int dead = 0;
    for (int i = 0; i < n; i++) {
        if (!rd(members[i], 0x100) || f.marked(members[i]) || !f.inWorld(members[i])) return;
        dead += !go_alive(members[i]);
    }
    if (!dead) return;
    /* who can cast: living companions with a scroll, the one with most mana first */
    void *caster = NULL, *scroll = NULL;
    float bestScore = -1e9f;
    for (int i = 0; i < n; i++) {
        void *c = members[i];
        if (!rd(c, 0x100) || !go_alive(c)) continue;
        if (c == leader) continue; /* the hero the player controls never casts on its own */
        void *inv = f.getInv(c), *asp = Go_GetAspect ? Go_GetAspect(c) : NULL, *mind = Go_GetMind(c);
        if (!rd(inv, 0x40) || !rd(asp, 0x40) || !rd(mind, 0x70)) continue;
        g_coll.e = g_coll.b;
        f.listItems(inv, AP_IL_MAIN, &g_coll);
        int ni = (int)(g_coll.e - g_coll.b);
        void *sc = NULL;
        for (int k = 0; k < ni && ni <= 512 && rd(g_coll.b, ni * sizeof(void *)) && !sc; k++) {
            void *it = g_coll.b[k];
            if (!rd(it, 0x100) || !f.isSpell(it)) continue;
            const char *tn = Go_GetTemplateName ? Go_GetTemplateName(it) : NULL;
            void *mg = f.getMagic(it);
            if (!tn || IsBadStringPtrA(tn, 64) || !strstr(tn, "resurrect") || !rd(mg, 0x40) || !f.oneUse(mg)) continue;
            sc = it;
        }
        if (!sc) continue;
        float score = f.curMana(asp);
        if (score > bestScore) { bestScore = score; caster = c; scroll = sc; }
    }
    if (!caster) return;
    void *mind = Go_GetMind(caster);
    /* not in the middle of a fight */
    g_coll.e = g_coll.b;
    GoMind_GetEnemiesInSphere(mind, 10.0f, &g_coll);
    int ne = (int)(g_coll.e - g_coll.b);
    for (int k = 0; k < ne && ne < 4096 && rd(g_coll.b, ne * sizeof(void *)); k++)
        if (go_alive(g_coll.b[k])) return;
    void *mg = f.getMagic(scroll);
    for (int i = 0; i < n; i++) {
        void *t = members[i];
        if (!rd(t, 0x100) || go_alive(t) || t == caster) continue;
        if (!f.castable(mg, t, 0)) continue; /* unconscious only, or otherwise not something the scroll works on */
        void *tg = Go_GetGoid(t);
        int skip = 0, slot = 0;
        for (int k = 0; k < 8; k++) {
            if (tried[k].goid == tg && now - tried[k].t < 10000) skip = 1; /* a cast takes a while; no orders on top of each other */
            if (tried[k].t < tried[slot].t) slot = k;
        }
        if (skip) continue;
        tried[slot].goid = tg;
        tried[slot].t = now;
        if (logs < 20) {
            const char *a = Go_GetTemplateName ? Go_GetTemplateName(caster) : NULL, *b = Go_GetTemplateName ? Go_GetTemplateName(t) : NULL;
            logs++;
            logf_("auto resurrect: %s uses a resurrect scroll on %s", a && !IsBadStringPtrA(a, 64) ? a : "?", b && !IsBadStringPtrA(b, 64) ? b : "?");
        }
        AR_CAST(mind, tg, Go_GetGoid(scroll));
        return; /* one at a time */
    }
}

/* ------------------------------------------------------------------ automatic pick-up
   Gold and potions lying within reach of the controlled hero are taken without a button press, with
   the routine the game's own "pick up" order ends in. The two kinds have separate switches. */
static void auto_pickup(void *hero, const SiegePos *hp, DWORD now)
{
    static struct {
        int ok;
        void *(TC *getInv)(void *);
        bool8 (TC *isGold)(void *), (TC *isPotion)(void *);
        bool8 (TC *testGet)(void *, void *, bool8);
        bool8 (TC *autoGet)(void *, void *, int);
        bool8 (TC *listItems)(void *, int, GopColl *);
        void *(TC *getGold)(void *);
        void *(TC *droppedBy)(void *);
    } f;
    /* potions that have been in a party member's pack: one of those lying on the ground was put
       there by the player, and is left alone */
    static void *owned[768];
    static int nOwned, ownedAt;
    static struct { void *goid; DWORD t; } tried[24];
    static DWORD last;
    static int ti, logs;
    if ((!c_pickGold && !c_pickPotions) || now - last < 200) return;
    last = now;
    if (!f.ok) {
        f.getInv = (void *)GetProcAddress(g_exe, "?GetInventory@Go@@QAEPAVGoInventory@@XZ");
        f.isGold = (void *)GetProcAddress(g_exe, "?IsGold@Go@@QBE_NXZ");
        f.isPotion = (void *)GetProcAddress(g_exe, "?IsPotion@Go@@QBE_NXZ");
        f.testGet = (void *)GetProcAddress(g_exe, AP_TESTGET_SYM);
        f.autoGet = (void *)GetProcAddress(g_exe, "?SAutoGet@GoInventory@@QAE_NPBUGoid_@@W4eActionOrigin@@@Z");
        f.listItems = (void *)GetProcAddress(g_exe, AP_LIST_SYM);
        if (AP_GOLD_DROPPER) {
            f.getGold = (void *)GetProcAddress(g_exe, "?GetGold@Go@@QAEPAVGoGold@@XZ");
            f.droppedBy = (void *)GetProcAddress(g_exe, "?GetDroppedBy@GoGold@@QBEPBUGoid_@@XZ");
        }
        f.ok = f.getInv && f.isGold && f.isPotion && f.testGet && f.autoGet ? 1 : -1;
        logf_("auto pick-up: link %d, gold %d, potions %d, radius %.1f m", f.ok, c_pickGold, c_pickPotions, c_pickRadius);
    }
    if (f.ok != 1) return;
    void *aiq = AIQuery_Get();
    void *inv = f.getInv(hero);
    if (!rd(aiq, 0x10) || !rd(inv, 0x40)) return;
    if (c_pickPotions && f.listItems) {
        /* note every potion the party carries right now */
        void *party = get_party();
        GopColl *kids = party ? Go_GetChildren(party) : NULL;
        int nm = rd(kids, 8) ? (int)(kids->e - kids->b) : 0;
        void *members[16];
        if (nm > 0 && nm <= 16 && rd(kids->b, nm * sizeof(void *))) {
            memcpy(members, kids->b, nm * sizeof(void *));
            for (int m = 0; m < nm; m++) {
                void *minv = rd(members[m], 0x100) ? f.getInv(members[m]) : NULL;
                if (!rd(minv, 0x40)) continue;
                g_coll.e = g_coll.b;
                f.listItems(minv, AP_IL_MAIN, &g_coll);
                int ni = (int)(g_coll.e - g_coll.b);
                if (ni <= 0 || ni > 512 || !rd(g_coll.b, ni * sizeof(void *))) continue;
                for (int i = 0; i < ni; i++) {
                    void *it = g_coll.b[i];
                    if (!rd(it, 0x100) || !f.isPotion(it)) continue;
                    void *gd = Go_GetGoid(it);
                    int known = 0;
                    for (int k = 0; k < nOwned && !known; k++) known = owned[k] == gd;
                    if (known) continue;
                    if (nOwned < 768) owned[nOwned++] = gd;
                    else { owned[ownedAt] = gd; ownedAt = (ownedAt + 1) % 768; }
                }
            }
        }
    }
    g_coll.e = g_coll.b;
    AIQuery_GetOccupantsInSphere(aiq, hp, c_pickRadius, &g_coll);
    int n = (int)(g_coll.e - g_coll.b);
    if (n <= 0 || n > 8192 || !rd(g_coll.b, n * sizeof(void *))) return;
    void *items[48];
    int ni = 0;
    for (int i = 0; i < n && ni < 48; i++) {
        void *g = g_coll.b[i];
        if (!rd(g, 0x100) || g == hero || Go_IsActor(g) || !Go_IsItem(g) || Go_IsInsideInventory(g)) continue;
        int gold = f.isGold(g), potion = !gold && f.isPotion(g);
        if (!(gold && c_pickGold) && !(potion && c_pickPotions)) continue;
        if (!go_clickable(g)) continue;
        if (potion) {
            void *gd = Go_GetGoid(g);
            int mine = 0;
            for (int k = 0; k < nOwned && !mine; k++) mine = owned[k] == gd;
            if (mine) continue; /* dropped by the player */
        }
        if (gold && f.getGold && f.droppedBy) {
            /* gold remembers who dropped it: the party's own is left alone as well */
            void *gg = f.getGold(g);
            void *by = rd(gg, 0x20) ? f.droppedBy(gg) : NULL;
            if (by) {
                int own = 0;
                void *party = get_party();
                GopColl *kids = party ? Go_GetChildren(party) : NULL;
                int nm = rd(kids, 8) ? (int)(kids->e - kids->b) : 0;
                for (int m = 0; m < nm && nm <= 16 && rd(kids->b, nm * sizeof(void *)) && !own; m++)
                    own = rd(kids->b[m], 0x100) && Go_GetGoid(kids->b[m]) == by;
                if (own) continue;
            }
        }
        items[ni++] = g; /* the calls below reuse the game's list buffer */
    }
    for (int i = 0; i < ni; i++) {
        void *g = items[i], *goid = Go_GetGoid(g);
        int skip = 0;
        for (int k = 0; k < 24; k++)
            if (tried[k].goid == goid && now - tried[k].t < 4000) skip = 1; /* could not be taken a moment ago (no room): not asked again at once */
        if (skip) continue;
        const char *tn = Go_GetTemplateName ? Go_GetTemplateName(g) : NULL;
        char name[48];
        lstrcpynA(name, tn && !IsBadStringPtrA(tn, 64) ? tn : "?", sizeof name); /* the object is gone once it is taken */
        bool8 can = f.testGet(inv, goid, 1);
        bool8 r = can ? f.autoGet(inv, goid, AP_ORIGIN) : 0;
        tried[ti].goid = goid;
        tried[ti].t = now;
        ti = (ti + 1) % 24;
        if (logs < 25) { logs++; logf_("auto pick-up: %s -> %s", name, !can ? "no room / not allowed" : r ? "taken" : "failed"); }
    }
}

#define PS_LIST_SYM "?ListItems@GoInventory@@QBE_NW4eInventoryLocation@@AAUGopColl@@@Z"
#define PS_ORIGIN AO_USER
static int potion_il_main(void)
{
    /* asked for by value: walking through every possible number made the game write an error report
       for each one that is not a location, which cost 15 ms the first time a companion needed a potion */
    const char *(*ilName)(int) = (void *)GetProcAddress(g_exe, "?ToString@@YAPBDW4eInventoryLocation@@@Z");
    const char *n = ilName ? ilName(16) : NULL;
    return n && !IsBadStringPtrA(n, 32) && !lstrcmpiA(n, "il_main") ? 16 : -1;
}
/* ------------------------------------------------------------------ potion sharing
   A companion that needs a health or mana potion and has none is handed one by another member of
   the party, using the game's own "give" routine. A member only gives while it has more than
   PotionShareKeep potions of that kind, so nobody is emptied out. */
/* PotionShare, PotionShareKeep: set in load_config (defaults 1 and 3) */
static struct {
    int ok;
    void *(TC *getInv)(void *);
    void *(TC *getMagic)(void *);
    bool8 (TC *isPotion)(void *);
    bool8 (TC *isHealth)(void *), (TC *isMana)(void *), (TC *isRejuv)(void *);
    float (TC *fullRatio)(void *);
    bool8 (TC *listItems)(void *, int, GopColl *);
    bool8 (TC *autoGive)(void *, void *, void *, int);
    int ilMain;
} g_ps;
static void potion_link(void)
{
    if (g_ps.ok) return;
    g_ps.getInv = (void *)GetProcAddress(g_exe, "?GetInventory@Go@@QAEPAVGoInventory@@XZ");
    g_ps.getMagic = (void *)GetProcAddress(g_exe, "?GetMagic@Go@@QAEPAVGoMagic@@XZ");
    g_ps.isPotion = (void *)GetProcAddress(g_exe, "?IsPotion@Go@@QBE_NXZ");
    g_ps.isHealth = (void *)GetProcAddress(g_exe, "?IsHealthPotion@GoMagic@@QBE_NXZ");
    g_ps.isMana = (void *)GetProcAddress(g_exe, "?IsManaPotion@GoMagic@@QBE_NXZ");
    g_ps.isRejuv = (void *)GetProcAddress(g_exe, "?IsRejuvenationPotion@GoMagic@@QBE_NXZ");
    g_ps.fullRatio = (void *)GetProcAddress(g_exe, "?GetPotionFullRatio@GoMagic@@QBEMXZ");
    g_ps.listItems = (void *)GetProcAddress(g_exe, PS_LIST_SYM);
    g_ps.autoGive = (void *)GetProcAddress(g_exe, "?SAutoGive@GoInventory@@QAE_NPBUGoid_@@0W4eActionOrigin@@@Z");
    g_ps.ilMain = potion_il_main();
    g_ps.ok = g_ps.getInv && g_ps.getMagic && g_ps.isPotion && g_ps.isHealth && g_ps.isMana && g_ps.isRejuv && g_ps.fullRatio &&
              g_ps.listItems && g_ps.autoGive && g_ps.ilMain >= 0 ? 1 : -1;
    logf_("potion sharing: link %d (main inventory location %d), share %d, keep %d", g_ps.ok, g_ps.ilMain, c_potionShare, c_potionKeep);
}
/* how many potions of this kind (mana 0 = health, 1 = mana) the character carries; rejuvenation
   potions count for both. *give is the one to hand over: a plain potion before a rejuvenation one */
static int potion_count(void *go, int mana, void **give)
{
    void *inv = g_ps.getInv(go);
    void *items[256];
    int n, cnt = 0, haveOwn = 0;
    if (give) *give = NULL;
    if (!rd(inv, 0x40)) return 0;
    g_coll.e = g_coll.b;
    g_ps.listItems(inv, g_ps.ilMain, &g_coll);
    n = (int)(g_coll.e - g_coll.b);
    if (n <= 0 || n > 256 || !rd(g_coll.b, n * sizeof(void *))) return 0;
    memcpy(items, g_coll.b, n * sizeof(void *));
    for (int i = 0; i < n; i++) {
        void *it = items[i];
        if (!rd(it, 0x100) || !g_ps.isPotion(it)) continue;
        void *mg = g_ps.getMagic(it);
        if (!rd(mg, 0x40) || !(g_ps.fullRatio(mg) > 0.0f)) continue;
        int own = mana ? g_ps.isMana(mg) : g_ps.isHealth(mg);
        int rej = g_ps.isRejuv(mg);
        if (!own && !rej) continue;
        cnt++;
        if (give && (!*give || (own && !rej && !haveOwn))) { *give = it; haveOwn = own && !rej; }
    }
    return cnt;
}
/* returns 1 when a potion was handed to `needy` */
static int potion_share(void **members, int n, void *needy, int mana)
{
    void *best = NULL, *bestItem = NULL;
    int bestCnt = 0;
    for (int i = 0; i < n; i++) {
        void *d = members[i], *item = NULL;
        if (d == needy || !rd(d, 0x100)) continue;
        int c = potion_count(d, mana, &item);
        if (c > c_potionKeep && c > bestCnt && item) { bestCnt = c; best = d; bestItem = item; }
    }
    static int logs;
    if (!best) {
        static DWORD nl;
        DWORD t = timeGetTime();
        if (logs < 40 && t - nl > 5000) {
            const char *tn = Go_GetTemplateName ? Go_GetTemplateName(needy) : NULL;
            nl = t;
            logs++;
            logf_("potion sharing: %s needs a %s potion and has none; nobody in the party has more than %d to spare",
                  tn && !IsBadStringPtrA(tn, 64) ? tn : "?", mana ? "mana" : "health", c_potionKeep);
        }
        return 0;
    }
    bool8 r = g_ps.autoGive(g_ps.getInv(best), Go_GetGoid(needy), Go_GetGoid(bestItem), PS_ORIGIN);
    int got = potion_count(needy, mana, NULL);
    if (logs < 40) {
        const char *a = Go_GetTemplateName ? Go_GetTemplateName(best) : NULL, *b = Go_GetTemplateName ? Go_GetTemplateName(needy) : NULL;
        const char *c = Go_GetTemplateName ? Go_GetTemplateName(bestItem) : NULL;
        logs++;
        logf_("potion sharing: %s (had %d) gave %s to %s -> %d, the receiver now has %d",
              a && !IsBadStringPtrA(a, 64) ? a : "?", bestCnt, c && !IsBadStringPtrA(c, 64) ? c : "?", b && !IsBadStringPtrA(b, 64) ? b : "?", r, got);
    }
    return got > 0;
}
static void auto_potions(void *leader, DWORD now)
{
    static DWORD last, lifeAt[64], manaAt[64], shareAt[64];
    if (!c_autoPotion || !Go_GetAspect || !GoAspect_GetCurrentLife || !GoAspect_GetMaxLife || !GoAspect_GetCurrentMana ||
        !GoAspect_GetMaxMana || now - last < 300)
        return;
    last = now;
    void *party = get_party();
    if (!party) return;
    GopColl *kids = Go_GetChildren(party);
    if (!rd(kids, 8)) return;
    int n = (int)(kids->e - kids->b);
    if (n <= 1 || n > 64 || !rd(kids->b, n * sizeof(void *))) return;
    void *members[64];
    memcpy(members, kids->b, n * sizeof(void *));
    if (c_potionShare) potion_link();
    int share = c_potionShare && g_ps.ok == 1;
    float lim = c_autoPotionPct / 100.0f;
    for (int i = 0; i < n; i++) {
        void *m = members[i];
        if (m == leader || !go_alive(m)) continue;
        void *mind = Go_GetMind(m);
        void *asp = Go_GetAspect(m);
        if (!rd(mind, 0x70) || !rd(asp, 0x48)) continue;
        float ml = GoAspect_GetMaxLife(asp), mm = GoAspect_GetMaxMana(asp);
        static int pl;
        for (int mana = 0; mana < 2; mana++) {
            float cur = mana ? GoAspect_GetCurrentMana(asp) : GoAspect_GetCurrentLife(asp), max = mana ? mm : ml;
            DWORD *at = mana ? &manaAt[i] : &lifeAt[i];
            if (!(max > 1.0f && cur < max * lim) || now - *at <= 2500) continue;
            if (share && potion_count(m, mana, NULL) == 0) {
                /* nothing to drink: ask the others, and drink on the next pass if one was handed over */
                if (now - shareAt[i] < 1500) continue;
                shareAt[i] = now;
                if (!potion_share(members, n, m, mana)) continue;
            }
            *at = now;
            if (pl < 20) { pl++; logf_("auto potion: companion %d %s %.0f of %.0f", i, mana ? "mana" : "health", cur, max); }
            if (mana) GoMind_RSDrinkMana(mind, AO_USER); else GoMind_RSDrinkLife(mind, AO_USER);
        }
    }
}

/* ------------------------------------------------------------------ main update */
enum { MODE_NONE, MODE_GAME, MODE_MENU, MODE_NIS };
static int g_mode = MODE_NONE, g_forceGame;
static DWORD g_lastUiScan, g_lastPadScan, g_xHoldStart;
static int g_menuCached, g_padIndex = -1;
static DWORD g_prevButtons, g_navNext;
static int g_xLongFired;
static DWORD g_orderHoldUntil, g_orderHoldFrom;
static float g_curFx, g_curFy;

static float axis(SHORT v)
{
    float f = v / 32767.0f;
    if (f < -1) f = -1;
    return f;
}

static int detect_mode(DWORD now)
{
    void *ws = g_uiOK ? WorldState_Get() : NULL;
    int ingame = 0;
    if (rd(ws, 8)) {
        const char *n = WS_ToString(WorldState_GetCurrentState(ws));
        if (n && !IsBadStringPtrA(n, 48)) {
            static const char *lastN;
            static int stLogs;
            if (n != lastN && stLogs < 300) { stLogs++; lastN = n; logf_("world state: %s", n); }
            if (strstr(n, "nis") || strstr(n, "outro") || strstr(n, "intro") || strstr(n, "movie")) return MODE_NIS;
            ingame = strstr(n, "ingame") && !strstr(n, "menu");
        }
    }
    if (!ingame || !g_gameOK) return MODE_MENU;
    if (g_forceGame) return MODE_GAME;
    if (now - g_lastUiScan > 120) {
        g_lastUiScan = now;
        g_menuCached = collect_ui(0);
    }
    return g_menuCached > 0 ? MODE_MENU : MODE_GAME;
}

/* Party follow. The game only moves the companions along when the hero is given a move order with
   the mouse; a hero walked with the stick gets no such order, so the companions would stay behind.
   The game's own "follow" order is no use here: it walks to where the hero's *planned path* ends, and
   with the stick that lags far behind the hero. So each companion that is free, further than
   FollowDistance from the hero and has no enemy near is given a plain move order to a spot just short
   of where the hero really stands, renewed whenever the hero has moved on. Companions that are busy
   (fighting, casting, picking up) are left alone, and one on its way is released when enemies turn up. */
static int JAT_MOVE = -2;
static float plan_end_gap(void *hero, const SiegePos *hp)
{
    char *plan = mcp_plan(Go_GetGoid(hero));
    if (!plan) return -1.0f;
    char *head = *(char **)(plan + 0xc);
    int n = *(int *)(plan + 0x10);
    if (n <= 0 || n > 512 || !rd(head, 8)) return -1.0f;
    char *last = *(char **)(head + 4);
    if (!rd(last, 0x58)) return -1.0f;
    SiegePos *p = (SiegePos *)(last + 0x30);
    Vec3 d = { 0, 0, 0 };
    if (!p->node) return -1.0f;
    GetSiegeDifference(&d, hp, p);
    return sqrtf(d.x * d.x + d.z * d.z);
}
static void update_followers(void *leader, const SiegePos *lp, DWORD now)
{
    static void *prevLeader;
    static DWORD last;
    static struct { void *goid; SiegePos at; DWORD t; } ord[16]; /* the mod's own last order per companion */
    if (JAT_MOVE == -2) JAT_MOVE = enum_find(Jat_ToString, "jat_move");
    if (!c_follow || JAT_MOVE < 0) return;
    int changed = leader != prevLeader;
    if (!changed && now - last < 300) return;
    last = now;
    void *party = get_party();
    if (!party) return;
    GopColl *kids = Go_GetChildren(party);
    if (!rd(kids, 8)) return;
    int n = (int)(kids->e - kids->b);
    if (n <= 1 || n > 64 || !rd(kids->b, n * sizeof(void *))) { prevLeader = leader; return; }
    if (changed) {
        /* the newly controlled hero may itself still be on its way after the previous one */
        void *lm = Go_GetMind(leader);
        int *job = rd(lm, 0x100) ? GoMind_GetFrontJob(lm, JQ_ACTION) : NULL;
        if (rd(job, 4) && (*job == JAT_FOLLOW || *job == JAT_MOVE)) GoMind_RSStop(lm, AO_USER);
    }
    void *members[64];
    memcpy(members, kids->b, n * sizeof(void *)); /* the calls below reuse the game's list buffers */
    int slot = 0;
    for (int i = 0; i < n; i++) {
        void *m = members[i];
        SiegePos mp;
        if (m == leader || !go_alive(m)) continue;
        void *mind = Go_GetMind(m);
        if (!rd(mind, 0x100) || !go_pos(m, &mp)) continue;
        slot++;
        void *goid = Go_GetGoid(m);
        int oi = -1;
        for (int k = 0; k < 16; k++) if (ord[k].goid == goid) oi = k;
        if (oi < 0) { oi = (slot - 1) & 15; ord[oi].goid = goid; ord[oi].t = 0; }
        int *job = GoMind_GetFrontJob(mind, JQ_ACTION);
        int jat = rd(job, 4) ? *job : -1;
        int idle = !job || jat == JAT_FIDGET || jat == JAT_NONE || jat == JAT_STARTUP || jat == JAT_FACE || jat == JAT_STOP;
        /* a follow order (from the game or an earlier build) is always replaceable; a move order only if it is the mod's */
        int mine = jat == JAT_FOLLOW || (jat == JAT_MOVE && ord[oi].t && now - ord[oi].t < 20000);
        Vec3 d = { 0, 0, 0 };
        GetSiegeDifference(&d, lp, &mp);
        float dist = sqrtf(d.x * d.x + d.z * d.z);
        int foes = 0;
        g_coll.e = g_coll.b;
        GoMind_GetEnemiesInSphere(mind, 10.0f, &g_coll);
        int ne = (int)(g_coll.e - g_coll.b);
        for (int k = 0; k < ne && ne < 4096 && rd(g_coll.b, ne * sizeof(void *)) && !foes; k++)
            if (go_alive(g_coll.b[k])) foes = 1;
        {
            static int fl;
            static DWORD flast;
            if (fl < 40 && now - flast > 1500) {
                const char *jn = jat >= 0 && Jat_ToString ? Jat_ToString(jat) : NULL;
                const char *tn = Go_GetTemplateName ? Go_GetTemplateName(m) : NULL;
                fl++;
                if (slot == 1) flast = now - 1; else flast = now;
                logf_("follow: companion %d (%s) front job %s (%s%s), %.1f m from the hero, enemies near %d; the hero's planned path ends %.1f m from the hero",
                      slot, tn && !IsBadStringPtrA(tn, 64) ? tn : "?", jn && !IsBadStringPtrA(jn, 64) ? jn : "none", idle ? "free" : "busy",
                      mine ? ", the mod's order" : "", dist, foes, plan_end_gap(leader, lp));
            }
        }
        if (foes && dist < 14.0f) {
            if (mine) { GoMind_RSStop(mind, AO_USER); ord[oi].t = 0; } /* left to fight */
            continue;
        }
        if (!idle && !mine) continue;          /* busy with something of its own */
        if (dist < c_followDist) continue;     /* near enough; an order still running just finishes */
        if (mine) {
            /* on its way already: a new order only once the hero has moved on from where it was sent */
            Vec3 md = { 0, 0, 0 };
            GetSiegeDifference(&md, lp, &ord[oi].at);
            if (jat == JAT_MOVE && md.x * md.x + md.z * md.z < 1.5f * 1.5f) continue;
        }
        /* a spot short of the hero, on the companion's side, a little further out for each companion */
        float r = 1.4f + 0.6f * (slot - 1);
        SiegePos t = *lp;
        if (dist > 0.1f) { t.x += d.x / dist * r; t.z += d.z / dist * r; }
        bool8 ok = GoMind_RSMove(mind, &t, QP_CLEAR, AO_USER);
        if (!ok) { t = *lp; ok = GoMind_RSMove(mind, &t, QP_CLEAR, AO_USER); }
        ord[oi].at = *lp;
        ord[oi].t = now;
        {
            static int ol;
            if (ol < 16 && (!mine || !ok)) { ol++; logf_("follow: companion %d sent after the hero (%.1f m away) -> %d", slot, dist, ok); }
        }
    }
    prevLeader = leader;
}

static void game_update(DWORD now, DWORD btn, DWORD pressed, float lx, float ly, float rx, float ry)
{
    void *hero = get_leader();
    SiegePos hp;
    if (!hero || !go_pos(hero, &hp)) { direct_stop(); return; }
    void *mind = Go_GetMind(hero);
    int alive = go_alive(hero);
    int lt = (btn & B_LT) != 0;
    {
        static void *lastHero;
        if (hero != lastHero) {
            const char *tn = Go_GetTemplateName ? Go_GetTemplateName(hero) : NULL;
            lastHero = hero;
            logf_("controlled hero: %s at %.2f,%.2f,%.2f piece %08x", tn && !IsBadStringPtrA(tn, 64) ? tn : "?", hp.x, hp.y, hp.z, hp.node);
        }
    }
    {
        /* while the game is paused the hero must not be moved or given orders */
        void *app = AppModule_Get ? AppModule_Get() : NULL;
        if (AppModule_IsUserPaused && rd(app, 0x140) && AppModule_IsUserPaused(app)) {
            direct_stop();
            g_moving = 0;
            alive = 0;
        }
    }

    /* camera */
    hold_key(k_camLeft, rx < -0.5f);
    hold_key(k_camRight, rx > 0.5f);
    hold_key(k_camUp, ry > 0.5f);
    hold_key(k_camDown, ry < -0.5f);
    hold_key(k_zoomIn, !lt && (btn & B_UP));
    hold_key(k_zoomOut, !lt && (btn & B_DOWN));

    if (lt) {
        static const DWORD order[4] = { B_A, B_B, B_X, B_Y };
        for (int i = 0; i < 4; i++)
            if (pressed & order[i]) tap_key(k_power[i]);
        if (pressed & B_LEFT) tap_key(k_mirror);
        if (pressed & B_RIGHT) tap_key(k_rampage);
        if (pressed & B_LB) tap_key(k_skills);
        if (pressed & B_RB) tap_key(k_spellbook);
        if (pressed & B_BACK) tap_key(k_journal);
    } else {
        void *uig = UIGame_Get();
        if ((pressed & B_RIGHT) && rd(uig, 4)) { direct_stop(); direct_release(); UIGame_SelectNextPlayer(uig); }
        if ((pressed & B_LEFT) && rd(uig, 4)) { direct_stop(); direct_release(); UIGame_SelectLastPlayer(uig); }
        if ((pressed & B_LB) && alive) GoMind_RSDrinkLife(mind, AO_USER);
        if ((pressed & B_RB) && alive) GoMind_RSDrinkMana(mind, AO_USER);
        if ((pressed & B_Y) && alive) cycle_active(hero);
        if (pressed & B_BACK) tap_key(k_inventory);
    }
    if (pressed & B_START) tap_key(k_menu);
    if (pressed & B_L3) tap_key(k_map);
    if (pressed & B_R3) {
        if (lt) tap_key(k_pause);
        else if (alive) cycle_target(hero, &hp);
    }

    /* stick direction in the hero's terrain-piece space */
    float mag = sqrtf(lx * lx + ly * ly);
    float dx = 0, dz = 0;
    int haveDir = 0;
    if (mag > c_deadzone) {
        float fx, fz, rgx, rgz;
        if (cam_basis(&hp, &fx, &fz, &rgx, &rgz)) {
            dx = fx * ly + rgx * lx;
            dz = fz * ly + rgz * lx;
            g_dirDbg.lx = lx; g_dirDbg.ly = ly; g_dirDbg.fx = fx; g_dirDbg.fz = fz;
            float l = sqrtf(dx * dx + dz * dz);
            if (l > 0.001f) { dx /= l; dz /= l; haveDir = 1; }
        }
    }

    int attackHeld = !lt && (btn & (B_A | B_RT));
    int attackPressed = !lt && (pressed & (B_A | B_RT));
    int attacking = 0;
    if (alive && attackPressed) {
        int tk = 0;
        void *tg = pick_target(hero, &hp, haveDir, dx, dz, &tk);
        direct_stop();
        if (tg && tk == 0) {
            int same = g_atkGoid && Go_GetGoid(tg) == g_atkGoid && attack_running(mind);
            if (!same) issue_attack(hero, tg);
            attacking = 1;
            { g_orderHoldUntil = now + 700; g_orderHoldFrom = now; } /* a tap commits to the attack: the stick does not cancel it at once */
        } else if (tg) {
            do_interact(hero, &hp);
            { g_orderHoldUntil = now + 1200; g_orderHoldFrom = now; }
        } else {
            static int noneLogs;
            if (noneLogs < 6) { noneLogs++; logf_("A: nothing in range"); dump_nearby(hero, &hp); }
        }
        g_lastAttackCheck = now;
        g_moving = 0;
    } else if (alive && attackHeld && g_targetGoid) {
        attacking = 1;
        if (now - g_lastAttackCheck > 250) {
            /* held: when the target is gone, go on to the next one */
            void *cur = NULL;
            void *best = pick_enemy(hero, &hp, haveDir, dx, dz, &cur);
            g_lastAttackCheck = now;
            if (!best) { g_targetGoid = NULL; attacking = 0; }
            else if (!(cur && attack_running(mind))) issue_attack(hero, cur ? cur : best);
        }
    }
    if (!attackHeld) g_targetGoid = NULL;

    /* X: tap = pick up / use / talk to nearest thing, hold = whole party collects loot */
    if (!lt) {
        if (pressed & B_X) { g_xHoldStart = now; g_xLongFired = 0; }
        if ((btn & B_X) && !g_xLongFired && now - g_xHoldStart > 450) { g_xLongFired = 1; tap_key(k_loot); }
        if (!(btn & B_X) && (g_prevButtons & B_X) && !g_xLongFired && alive) {
            direct_stop();
            if (do_interact(hero, &hp)) { g_orderHoldUntil = now + 1200; g_orderHoldFrom = now; }
        }
        if ((pressed & B_B) && alive) { direct_stop(); direct_release(); GoMind_RSStop(mind, AO_USER); g_moving = 0; g_lockGoid = NULL; g_atkGoid = NULL; }
    }

    /* movement */
    if (g_orderHoldUntil) {
        int *fj = GoMind_GetFrontJob(mind, JQ_ACTION);
        int fjat = rd(fj, 4) ? *fj : -1;
        int idle = !fj || fjat == JAT_FIDGET || fjat == JAT_NONE || fjat == JAT_STARTUP;
        if (!haveDir || (int)(now - g_orderHoldUntil) >= 0 || (idle && now - g_orderHoldFrom > 200)) g_orderHoldUntil = 0;
        else haveDir = 0;
    }
    if (direct_available()) {
        if (alive && haveDir && !attacking) TIMED(SEC_WALK, direct_step(hero, &hp, dx, dz, now));
        else direct_stop();
    } else if (alive && haveDir && !attacking) {
        direct_release();
        float turn = dx * g_lastDx + dz * g_lastDz;
        if (!g_moving || turn < 0.92f || now - g_lastMove > 600) {
            if (now - g_lastMove >= (DWORD)c_moveIntervalMs && try_move(hero, &hp, dx, dz, c_moveDist)) {
                g_moving = 1;
                g_lastMove = now;
                g_lastDx = dx;
                g_lastDz = dz;
            }
        }
    } else if (g_moving && !haveDir) {
        if (alive && !attacking) GoMind_RSStop(mind, AO_USER);
        g_moving = 0;
    }
    {
        static DWORD last;
        if (g_hlHooked && now - last >= 100) {
            int kind = 0;
            void *t = NULL;
            TIMED(SEC_TARGET, t = alive ? pick_target(hero, &hp, haveDir, dx, dz, &kind) : NULL);
            last = now;
            g_hlGoid = t ? Go_GetGoid(t) : NULL;
            g_hlHave = t && go_pos(t, &g_hlPos);
            g_hlEnemy = t && kind == 0;
            item_info_update(t, kind);
        }
    }
    TIMED(SEC_POTION, auto_potions(hero, now));
    if (alive) TIMED(SEC_PICKUP, auto_pickup(hero, &hp, now));
    auto_resurrect(hero, now);
    if (alive) TIMED(SEC_FOLLOW, update_followers(hero, &hp, now));
}

static void menu_update(DWORD now, float dt, DWORD btn, DWORD pressed, float rx, float ry, int entered)
{
    if (entered) {
        collect_ui(0);
        dump_ui();
        nav_snap_nearest();
    }
    /* d-pad / left stick: snap navigation with auto-repeat */
    DWORD dirs = btn & (B_UP | B_DOWN | B_LEFT | B_RIGHT);
    if (btn & N_UP) dirs |= B_UP;
    if (btn & N_DOWN) dirs |= B_DOWN;
    if (btn & N_LEFT) dirs |= B_LEFT;
    if (btn & N_RIGHT) dirs |= B_RIGHT;
    static DWORD prevDirs;
    if (dirs && (dirs != prevDirs || now >= g_navNext)) {
        g_navNext = now + (dirs != prevDirs ? c_navRepeatMs * 2 : c_navRepeatMs);
        if (dirs & B_UP) TIMED(SEC_NAV, nav_move(0, -1));
        else if (dirs & B_DOWN) TIMED(SEC_NAV, nav_move(0, 1));
        else if (dirs & B_LEFT) TIMED(SEC_NAV, nav_move(-1, 0));
        else if (dirs & B_RIGHT) TIMED(SEC_NAV, nav_move(1, 0));
    }
    prevDirs = dirs;

    /* right stick: free cursor */
    float m = sqrtf(rx * rx + ry * ry);
    if (m > c_deadzone) {
        float sp = c_cursorSpeed * (g_uiW / 800.0f) * m * dt;
        g_curFx += rx / m * sp;
        g_curFy -= ry / m * sp;
        int ix = (int)g_curFx, iy = (int)g_curFy;
        g_tgtActive = 0;
        if (ix || iy) {
            g_curFx -= ix;
            g_curFy -= iy;
            nudge_os(ix, iy, now);
        }
    } else {
        g_curFx = g_curFy = 0;
    }
    drive_cursor(now);

    mouse_button(0, (btn & B_A) != 0);
    mouse_button(1, (btn & B_X) != 0);
    hold_key(VK_SHIFT, (btn & B_LT) != 0);
    hold_key(VK_CONTROL, (btn & B_RT) != 0);
    if (pressed & (B_B | B_START)) tap_key(k_menu);
    if (pressed & B_BACK) tap_key(k_inventory);
    if (pressed & B_Y) tap_key(k_spellbook);
    if (pressed & B_L3) tap_key(k_map);
    if (pressed & B_R3) tap_key(k_pause);
    if (g_gameOK && (pressed & (B_LB | B_RB)) && g_mode == MODE_MENU) {
        void *hero = rd(Server_Get(), 0x20) ? get_leader() : NULL;
        if (hero && go_alive(hero)) {
            if (pressed & B_LB) GoMind_RSDrinkLife(Go_GetMind(hero), AO_USER);
            if (pressed & B_RB) GoMind_RSDrinkMana(Go_GetMind(hero), AO_USER);
        }
    }
}

static int g_inited, g_inTick, g_enabled, g_shutdown;
static DWORD g_lastTick, g_mainThread;

static void init_once(void)
{
    g_inited = 1;
    load_config();
    logf_("DS2Pad build %d starting", BUILD);
    memset(g_typeKind, -1, sizeof g_typeKind);
    resolve();
    c_fpsFix = cfg_i("FrameRateFix", 1);
    c_highlight = cfg_i("HighlightTarget", 1);
    if (g_gameOK) install_highlight();
    install_fps_fix();
    if (g_gameOK && g_directOK) install_follower_hooks();
    if (g_gameOK && c_camLock) {
        g_camHooked = patch_call(0x466a30, (DWORD)Server_GetScreenParty, hk_cam_party);
        logf_("camera lock hook: %d", g_camHooked);
    }
    install_ui_scale();
    if (g_uiOK && g_gameOK) install_item_info();
    g_wmOK = code_is(FN_PIECE, "558bec8b4508833800741d8b") && code_is(FN_OCCUPIED, "558bec56578bf98b77248b57");
    logf_("walk mesh routines: %s", g_wmOK ? "found" : "NOT found (pathfinder line test will be used)");
    static const char *dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    for (int i = 0; i < 3 && !XGetState; i++) {
        HMODULE h = LoadLibraryA(dlls[i]);
        if (h) {
            XGetState = (void *)GetProcAddress(h, "XInputGetState");
            if (XGetState) logf_("using %s", dlls[i]);
        }
    }
    if (!XGetState) { logf_("XInput not available"); return; }
    if (!g_gameOK && !g_uiOK) { logf_("no engine functions found - disabled"); return; }
    g_enabled = 1;
}

static void tick(void)
{
    if (g_inTick || g_shutdown) return;
    DWORD now = timeGetTime();
    static LONG seqDone;
    static LONGLONG qpcDone;
    static int pacedLog;
    LONGLONG tickStart = qpc_now();
    int paced = g_frameSeq && now - g_presentMs < 250; /* frames are being drawn: one update per frame */
    float dt;
    if (paced) {
        if (g_frameSeq == seqDone) return;
        dt = qpcDone ? qpc_ms(qpcDone, g_presentQpc) / 1000.0f : 0.016f;
        seqDone = g_frameSeq;
        qpcDone = g_presentQpc;
        if (dt < 0.001f) dt = 0.001f;
    } else {
        if (now - g_lastTick < 5) return;
        dt = (now - g_lastTick) / 1000.0f;
        qpcDone = 0;
    }
    g_inTick = 1;
    if (dt > 0.1f) dt = 0.1f;
    g_dt = dt;
    g_lastTick = now;
    if (paced != pacedLog - 1 && pacedLog < 8) {
        logf_("frame pacing: updates are %s", paced ? "tied to drawn frames" : "on the message loop and timer (nothing is being drawn)");
        pacedLog = paced + 1;
    }
    for (int i = 0; i < SEC_N; i++) g_secMs[i] = 0;
    if (!g_inited) { g_mainThread = GetCurrentThreadId(); init_once(); logf_("first tick on thread %lu, enabled=%d", g_mainThread, g_enabled); }
    if (g_uiOK && GetCurrentThreadId() == g_mainThread) ensure_d3d_hook();
    if (!g_enabled || GetCurrentThreadId() != g_mainThread) { g_inTick = 0; return; }

    /* controller */
    XSTATE xs;
    int ok = 0;
    if (g_padIndex >= 0) ok = XGetState(g_padIndex, &xs) == 0;
    if (!ok) {
        g_padIndex = -1;
        if (now - g_lastPadScan > 1500 || !g_lastPadScan) {
            g_lastPadScan = now ? now : 1;
            static int warned;
            if (!warned++) logf_("looking for an XInput controller...");
            for (int i = 0; i < 4 && !ok; i++) {
                if (c_pad >= 0 && i != c_pad) continue;
                if (XGetState(i, &xs) == 0) { ok = 1; g_padIndex = i; logf_("controller %d connected", i); }
            }
        }
    }
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    if (fg) GetWindowThreadProcessId(fg, &pid);
    if (!ok || pid != GetCurrentProcessId()) {
        release_all();
        if (g_gameOK) { direct_stop(); direct_release(); }
        g_padPlay = 0;
        g_hideCursor = 0;
        g_hlGoid = NULL;
        g_prevButtons = 0;
        g_moving = 0;
        g_inTick = 0;
        return;
    }
    g_hwnd = fg;

    float lx = axis(xs.pad.lx), ly = axis(xs.pad.ly), rx = axis(xs.pad.rx), ry = axis(xs.pad.ry);
    if (c_swapSticks) { float t = lx; lx = rx; rx = t; t = ly; ly = ry; ry = t; }
    DWORD btn = xs.pad.buttons;
    if (xs.pad.lt > 60) btn |= B_LT;
    if (xs.pad.rt > 60) btn |= B_RT;
    if (ly > 0.55f) btn |= N_UP;
    if (ly < -0.55f) btn |= N_DOWN;
    if (lx < -0.55f) btn |= N_LEFT;
    if (lx > 0.55f) btn |= N_RIGHT;
    DWORD pressed = btn & ~g_prevButtons;

    /* both sticks clicked together: the pad stays in gameplay mode whatever the interface shows
       (way out if a HUD panel is mistaken for a menu), and back */
    if ((btn & B_L3) && (btn & B_R3) && (pressed & (B_L3 | B_R3))) {
        g_forceGame = !g_forceGame;
        logf_("gameplay mode %s by hand", g_forceGame ? "forced" : "no longer forced");
        pressed &= ~(B_L3 | B_R3);
    }

    if (g_ovrRelease && (int)(now - g_ovrRelease) >= 0) direct_release();
    int mode = MODE_MENU;
    TIMED(SEC_SCAN, mode = detect_mode(now));
    int entered = (mode != g_mode);
    if (entered) {
        release_all();
        g_moving = 0;
        g_targetGoid = NULL;
        g_tgtActive = 0;
        if (g_gameOK) { direct_stop(); direct_release(); }
        logf_("mode -> %s %s", mode == MODE_GAME ? "gameplay" : mode == MODE_MENU ? "menu" : "cutscene",
              mode == MODE_MENU ? g_menuNames : "");
        {
            static int dumps;
            if (g_uiOK && dumps < 12) { dumps++; dump_interfaces(); }
        }
        g_mode = mode;
        g_modeNow = mode;
    }
    {
        /* is the pad or the mouse the device in use? mouse movement or a click hands over to the mouse */
        static int padLast = 1;
        POINT p;
        int mouseAct = 0;
        int padAct = btn != 0 || fabsf(lx) > c_deadzone || fabsf(ly) > c_deadzone || fabsf(rx) > c_deadzone || fabsf(ry) > c_deadzone;
        static DWORD ownUntil; /* until then, pointer movement and clicks are the mod's own doing */
        if (mode != MODE_GAME || entered || g_mouseL || g_mouseR) ownUntil = now + 400;
        int own = (int)(ownUntil - now) > 0;
        if (GetCursorPos(&p)) {
            /* Only a deliberate move of the mouse counts: PointerWakePixels within a third of a second.
               In menus the pad itself moves the pointer and clicks, so there (and for a moment after)
               the reference point simply follows the pointer. */
            static POINT anchor;
            static DWORD anchorT;
            if (own || now - anchorT > 300) { anchor = p; anchorT = now; }
            if (abs(p.x - anchor.x) > c_pointerWake || abs(p.y - anchor.y) > c_pointerWake) mouseAct = 1;
        }
        if (mode == MODE_GAME) {
            if (!own && ((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON)) & 0x8000)) mouseAct = 1;
            if (mouseAct) padLast = 0;
            else if (padAct) padLast = 1;
        }
        g_padPlay = padLast && mode == MODE_GAME;
        g_hideCursor = c_hideCursor && g_padPlay;
        if (mode != MODE_GAME) { g_hlGoid = NULL; item_info_hide(); }
        else if (!g_padPlay) item_info_hide();
        {
            /* Windows' and Direct3D's pointers (see is_cursor_window) */
            static int was;
            static DWORD chk;
            if (g_hideCursor && (!was || now - chk > 250)) {
                CURSORINFO ci;
                chk = now;
                memset(&ci, 0, sizeof ci);
                ci.cbSize = sizeof ci;
                if (GetCursorInfo(&ci) && (ci.flags & CURSOR_SHOWING) && ci.hCursor) SetCursor(NULL);
                if (g_d3dCursorWanted == 1) d3d_cursor(FALSE);
            }
            if (!g_hideCursor && was && g_d3dCursorWanted == 1) d3d_cursor(TRUE); /* the mouse is back: so is Direct3D's pointer */
            was = g_hideCursor;
        }
    }
    if (mode == MODE_GAME) game_update(now, btn, pressed, lx, ly, rx, ry);
    else if (mode == MODE_MENU) TIMED(SEC_MENU, menu_update(now, dt, btn, pressed, rx, ry, entered));
    else if (pressed & (B_A | B_B | B_START)) tap_key(VK_ESCAPE);

    g_prevButtons = btn;
    {
        /* how much of each frame the mod itself takes, and the frame rate, once in a while */
        static DWORD t0, frames, worst, sum;
        DWORD el = timeGetTime() - now;
        {
            /* an update that took long enough to be felt: say where the time went */
            static int sl;
            static DWORD slast;
            float total = qpc_ms(tickStart, qpc_now());
            if (total > 3.0f && sl < 40 && now - slast > 1000 && !entered) {
                sl++;
                slast = now;
                logf_("slow update: %.1f ms in %s (interface scan %.1f, walking %.1f, target choice %.1f, potions %.1f, pick-up %.1f, followers %.1f, menu %.1f of which stepping between controls %.1f and moving the pointer %.1f)",
                      total, mode == MODE_GAME ? "play" : mode == MODE_MENU ? "a menu" : "a cutscene", g_secMs[SEC_SCAN], g_secMs[SEC_WALK],
                      g_secMs[SEC_TARGET], g_secMs[SEC_POTION], g_secMs[SEC_PICKUP], g_secMs[SEC_FOLLOW], g_secMs[SEC_MENU], g_secMs[SEC_NAV], g_secMs[SEC_CURSOR]);
            }
        }
        frames++;
        sum += el;
        if (el > worst) worst = el;
        if (!t0) t0 = now;
        if (now - t0 >= 10000) {
            static int logs;
            if (logs < 30) { logs++; logf_("timing: %.0f updates a second, mod update %.2f ms on average, %lu ms at worst", frames * 1000.0f / (now - t0), (float)sum / frames, worst); }
            if (g_ft.n > 60) {
                static int fl;
                static ULONG (WINAPI *ntq)(ULONG *, ULONG *, ULONG *);
                ULONG tmin = 0, tmax = 0, tcur = 0;
                if (!ntq) ntq = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryTimerResolution");
                if (ntq) ntq(&tmin, &tmax, &tcur);
                int uneven = (g_ft.late + g_ft.early) * 50 > g_ft.n || g_ft.swing / g_ft.n > 3.0f; /* over 2% odd frames, or frames differing by 3 ms on average */
                if (fl < 300 && (uneven || fl < 3)) {
                    fl++;
                    logf_("frames: %d in the last 10 s, %.1f ms on average, shortest %.1f, longest %.1f, %d over 22 ms, %d under 11 ms, neighbouring frames differ by %.1f ms on average; Windows timer step %.1f ms%s",
                          g_ft.n, g_ft.sum / g_ft.n, g_ft.min, g_ft.max, g_ft.late, g_ft.early, g_ft.swing / g_ft.n, tcur / 10000.0f, uneven ? "  <- uneven" : "");
                }
            }
            memset(&g_ft, 0, sizeof g_ft);
            t0 = now;
            frames = worst = sum = 0;
        }
    }
    g_inTick = 0;
}

/* ------------------------------------------------------------------ hook */
static BOOL (WINAPI *g_origPeek)(LPMSG, HWND, UINT, UINT, UINT);
static BOOL WINAPI hk_PeekMessageA(LPMSG m, HWND h, UINT a, UINT b, UINT r)
{
    tick();
    return g_origPeek(m, h, a, b, r);
}

static int hook_iat(void)
{
    BYTE *base = (BYTE *)GetModuleHandleA(NULL);
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) return 0;
    IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress);
    for (; imp->Name; imp++) {
        if (lstrcmpiA((char *)(base + imp->Name), "USER32.dll")) continue;
        if (!imp->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(base + imp->OriginalFirstThunk);
        IMAGE_THUNK_DATA *iat = (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++) {
            if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
            IMAGE_IMPORT_BY_NAME *n = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
            if (strcmp((char *)n->Name, "PeekMessageA")) continue;
            if ((void *)iat->u1.Function == (void *)hk_PeekMessageA) return 2; /* still in place */
            DWORD old;
            if (!VirtualProtect(&iat->u1.Function, sizeof(void *), PAGE_READWRITE, &old)) return 0;
            g_origPeek = (void *)iat->u1.Function;
            iat->u1.Function = (DWORD_PTR)hk_PeekMessageA;
            VirtualProtect(&iat->u1.Function, sizeof(void *), old, &old);
            return 1;
        }
    }
    return 0;
}

/* Second route onto the main thread: subclass the game window and run from a timer. */
static WNDPROC g_origWndProc;
static HWND g_subHwnd;
static int g_timerSet;
static LRESULT CALLBACK hk_WndProc(HWND h, UINT msg, WPARAM w, LPARAM l)
{
    if (!g_timerSet) { g_timerSet = 1; SetTimer(h, 0xD5AD, 8, NULL); }
    if (msg == WM_TIMER && w == 0xD5AD) { if (!g_shutdown) { install_battery_fix(); tick(); } return 0; }
    /* Windows asks the window which pointer to show whenever the mouse position changes over it */
    if (msg == WM_SETCURSOR && g_hideCursor && LOWORD(l) == HTCLIENT) { SetCursor(NULL); return TRUE; }
    /* power notices (battery low, power status changed) would make the game auto-save and prompt */
    if (msg == WM_POWERBROADCAST && g_noBattery && (w == 0x9 || w == 0xA)) return TRUE;
    if (msg == WM_DESTROY) { g_shutdown = 1; g_hideCursor = 0; logf_("window: destroyed, mod switched off"); }
    return CallWindowProcA(g_origWndProc, h, msg, w, l);
}
static BOOL CALLBACK find_wnd(HWND h, LPARAM out)
{
    DWORD pid = 0;
    RECT rc;
    GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
    if (!GetClientRect(h, &rc) || rc.right < 320 || rc.bottom < 200) return TRUE;
    *(HWND *)out = h;
    return FALSE;
}
static DWORD WINAPI watcher(LPVOID p)
{
    (void)p;
    int lastHook = -1;
    for (int i = 0;; i++) {
        Sleep(i < 40 ? 250 : 2000);
        if (g_shutdown) continue;
        int r = hook_iat();
        if (r != lastHook) { logf_("PeekMessageA import hook: %s", r == 0 ? "failed" : r == 1 ? "installed" : "in place"); lastHook = r; }
        if (g_subHwnd && !IsWindow(g_subHwnd)) { g_subHwnd = NULL; g_timerSet = 0; }
        if (!g_subHwnd) {
            HWND h = NULL;
            EnumWindows(find_wnd, (LPARAM)&h);
            if (h) {
                char cls[64] = "";
                GetClassNameA(h, cls, sizeof cls);
                g_origWndProc = (WNDPROC)SetWindowLongA(h, GWL_WNDPROC, (LONG)hk_WndProc);
                if (g_origWndProc) {
                    g_subHwnd = h;
                    PostMessageA(h, WM_NULL, 0, 0);
                    logf_("subclassed game window %p (class %s)", (void *)h, cls);
                } else logf_("could not subclass window %p (class %s), error %lu", (void *)h, cls, GetLastError());
            }
        }
    }
    return 0;
}

/* Battery check. The game runs a thread that polls the power status and, when it sees battery power,
   interrupts play with a dialog (and auto-saves on "status changed" / "battery low" notices). It is
   told the machine is on mains power with no battery at all. */
static BOOL WINAPI hk_GetSystemPowerStatus(LPSYSTEM_POWER_STATUS st)
{
    if (!st) return FALSE;
    st->ACLineStatus = 1;          /* online */
    st->BatteryFlag = 128;         /* no system battery */
    st->BatteryLifePercent = 255;  /* unknown */
    st->SystemStatusFlag = 0;
    st->BatteryLifeTime = (DWORD)-1;
    st->BatteryFullLifeTime = (DWORD)-1;
    return TRUE;
}
static int g_batteryDone;
static void install_battery_fix(void)
{
    static int tries;
    if (g_batteryDone || tries > 400) return;
    tries++;
    void **slot = (void **)0xb8e1c0; /* the game's import of GetSystemPowerStatus */
    void *real = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetSystemPowerStatus");
    DWORD old;
    g_noBattery = (int)GetPrivateProfileIntA("DS2Pad", "IgnoreBattery", 1, g_ini);
    if (!g_noBattery) { g_batteryDone = 1; logf_("battery check: left as it is"); return; }
    if (!rd(slot, 4) || !real || *slot != real) {
        if (tries == 400) logf_("battery check: the game's import is not where expected; left as it is");
        return;
    }
    if (!VirtualProtect(slot, 4, PAGE_READWRITE, &old)) { g_batteryDone = 1; logf_("battery check: cannot patch the import"); return; }
    *slot = (void *)hk_GetSystemPowerStatus;
    VirtualProtect(slot, 4, old, &old);
    g_batteryDone = 1;
    logf_("battery check: the game is told it runs on mains power");
}
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_DETACH) { g_shutdown = 1; logf_("process exiting (%s)", reserved ? "normal" : "library unloaded"); return TRUE; }
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        GetModuleFileNameA(inst, g_modDir, MAX_PATH);
        char *s = strrchr(g_modDir, '\\');
        if (s) s[1] = 0;
        char exe[MAX_PATH];
        GetModuleFileNameA(NULL, exe, MAX_PATH);
        lstrcpyA(g_dir, exe);
        s = strrchr(g_dir, '\\');
        if (s) s[1] = 0;
        snprintf(g_ini, sizeof g_ini, "%sds2pad.ini", g_dir);
        g_logLevel = (int)GetPrivateProfileIntA("DS2Pad", "Log", 1, g_ini);
        const char *b = strrchr(exe, '\\');
        logf_("---- DS2Pad build %d loaded into %s (pid %lu), library folder %s", BUILD, exe, GetCurrentProcessId(), g_modDir);
        char mark[8];
        if (GetEnvironmentVariableA("DS2PAD_ACTIVE", mark, sizeof mark)) { logf_("another DS2Pad copy is already active in this process; this one stays idle"); return TRUE; }
        SetEnvironmentVariableA("DS2PAD_ACTIVE", "1");
        if (!lstrcmpiA(b ? b + 1 : exe, "DungeonSiege2.exe")) {
            int r = hook_iat();
            logf_("PeekMessageA import hook at load: %s", r ? "installed" : "failed");
            install_battery_fix();
            CloseHandle(CreateThread(NULL, 0, watcher, NULL, 0, NULL));
        } else logf_("not DungeonSiege2.exe - controller support stays off, sound pass-through only");
    }
    return TRUE;
}
