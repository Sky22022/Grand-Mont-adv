// Grand Mont Adventure — openworld ski pour NumWorks (EADK)
// v3 : 10 vraies stations, villages, 5 remontees, PNJ (file d'attente,
//      danseurs au resto d'altitude, promeneurs), snowpark, boarder X,
//      course chrono A->B avec fantome + trace colore, figures composees,
//      ski/snow x carving/freestyle, vestiaire (couleur + materiel),
//      pass caches, noms de pistes/remontees selon le pays
#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define COL(r,g,b) ((eadk_color_t)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3)))

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Grand Mont Adventure";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define W 320
#define H 240
#define TOP 16
#define BTM 224
#define CHARW 7

#define MAXGW 240
#define MAXGH 176
#define NM 10
#define NGMAX 40
#define NKMAX 24
#define NLTMAX 8   // remontees max par montagne (5 -> 8 sur les grandes)
#define NPMAX 8    // PNJ
#define NPMAXI 9   // pass caches (items dorures)
#define GHMAXR 420 // taille max du fantome

static int iabsi(int a) { return a < 0 ? -a : a; }
static int imaxi(int a, int b) { return a > b ? a : b; }
static int imini(int a, int b) { return a < b ? a : b; }

// sin/cos sans libm (Taylor, |x| <= PI)
static const float PI = 3.14159265f;
static float wrapPi(float a) {
  while (a > PI) a -= 2 * PI;
  while (a < -PI) a += 2 * PI;
  return a;
}
static float fsin(float a) {
  a = wrapPi(a);
  float a2 = a * a;
  return a - a * a2 / 6.0f + a * a2 * a2 / 120.0f;
}
static float fcos(float a) {
  a = wrapPi(a);
  float a2 = a * a;
  return 1.0f - a2 / 2.0f + a2 * a2 / 24.0f;
}

// ---- RNG (xorshift) ----
static uint32_t rs = 987654321u;
static void srng(uint32_t s) { rs = s ? s : 1; }
static uint32_t rng32(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static int ri(int n) { return (int)(rng32() % (uint32_t)n); }
static float r01(void) { return (rng32() & 0xFFFFFF) * (1.0f / 16777216.0f); }

// ---- couleurs ----
#define CSKY  COL(120,170,235)
#define CSNOW COL(242,246,255)
#define CPOWD COL(226,234,248)
#define CGRM  COL(212,228,246)
#define CTREE COL(22,78,42)
#define CTRE2 COL(35,100,55)
#define CROCK COL(122,116,108)
#define CCLIF COL(70,66,62)
#define CKICK COL(235,145,40)
#define CBILD COL(150,90,50)
#define CHUD  COL(20,26,40)
#define CWH   COL(255,255,255)
#define CYE   COL(255,215,0)
#define CRED  COL(235,60,60)
#define CLIFT COL(60,60,55)
#define CGRN  COL(90,220,100)
#define CGY   COL(130,135,150)
#define CBLU  COL(70,130,235)
#define CPAS  COL(255,200,40)

// ---- types de cellules ----
enum { C_GROOM = 0, C_POW, C_TREE, C_ROCK, C_CLIF, C_KICK, C_BILD, C_PASG, C_RAIL, C_STRT, C_GATE, C_BOSS, C_RAMP, C_PIPE };

static const char * MNAMES[NM] = {
  "Val Thorens", "Chamonix", "Zermatt", "Grandvalira", "Cortina",
  "St Anton", "Verbier", "Hemsedal", "Garmisch", "Sierra Nevada"
};
typedef struct { const char * nom; const char * piste; const char * lifts[4]; const char * vill; const char * rest; } Country;
static Country CO[] = {
  { "France",   "Piste",   { "Telesiege", "Teleski", "Telecabine", "Telepherique" }, "Village", "Refuge" },
  { "Suisse",   "Piste",   { "Sessellift", "Schlepplift", "Gondel", "Seilbahn" }, "Weiler", "Berghaus" },
  { "Andorre",  "Pista",   { "Telesilla", "Telesqui", "Telecadira", "Teleferic" }, "Poble", "Xalet" },
  { "Italie",   "Pista",   { "Seggiovia", "Skilift", "Cabinovia", "Funivia" }, "Rifugio", "Rifugio Alpino" },
  { "Autriche", "Abfahrt", { "Sessellift", "Schlepplift", "Gondel", "Seilbahn" }, "Ort", "Hutte" },
  { "Norvege",  "Lope",    { "Stolheis", "Skiheis", "Gondol", "Taubane" }, "Grend", "Fjellstue" },
  { "Allemagne","Abfahrt", { "Sessellift", "Schlepplift", "Gondel", "Seilbahn" }, "Dorf", "Berghuette" },
  { "Espagne",  "Pista",   { "Telesilla", "Teleski", "Telecabina", "Teleferico" }, "Aldea", "Refugio" },
};
static const int CC[NM] = { 0, 0, 1, 2, 3, 4, 1, 5, 6, 7 };
#define NFEAT 12
static const char * FEAT[NFEAT] = {
  "des Aigles", "du Soleil", "de la Foret", "des Loups", "du Glacier", "du Chamois",
  "du Vent", "de la Crete", "des Sapins", "du Torrent", "des Etoiles", "de l'Ours"
};

typedef struct { int gw, gh, trees, rocks, cliff; } MSpec;
/* v3.4 : cartes x2 en surface (~1.41x par dimension) */
static MSpec MS[NM] = {
  {  92, 152, 1800,  240,  8 },
  { 110, 164, 2400,  360, 10 },
  { 136, 166, 3200,  520, 13 },
  { 160, 160, 4000,  680, 16 },
  { 172, 170, 4800,  840, 19 },
  { 176, 176, 5600, 1040, 22 },
  { 190, 176, 6400, 1280, 26 },
  { 200, 176, 7200, 1520, 30 },
  { 224, 176, 8000, 1800, 34 },
  { 240, 176, 8800, 2200, 40 },
};

// ---- grilles montagne ----
static uint8_t TY_[(MAXGW * MAXGH + 1) / 2];  /* v3.4 : TY compacte 4 bits/cellule */
static uint8_t HG[MAXGW * MAXGH];
static int GW, GH;
static inline uint8_t gt(int x, int y) { int i = y * GW + x; return (uint8_t)((i & 1) ? (TY_[i >> 1] >> 4) : (TY_[i >> 1] & 0x0F)); }
static inline void stc(int x, int y, uint8_t t) {
  int i = y * GW + x; uint8_t v = TY_[i >> 1];
  TY_[i >> 1] = (uint8_t)((i & 1) ? ((v & 0x0F) | (t << 4)) : ((v & 0xF0) | t));
}
static int pxx[MAXGH];              // piste principale (colonne par rangee)
typedef struct { int x1, y1, x2, y2, unlock, type; } Lift;
static Lift LT[NLTMAX];
static int nLifts;
static const int LUNS[NLTMAX] = { 0, 3, 6, 9, 12, 14, 16, 19 };
static const int ASP[4] = { 230, 170, 130, 85 };  // duree ascension par type
#define NVM 3
static int VX[NVM], VY[NVM]; static int NV;
static int gx[NGMAX], gy[NGMAX], gdone[NGMAX]; static int ng;
static int kx[NKMAX], ky[NKMAX]; static int nk;
static int bx[NGMAX], by[NGMAX], bdone[NGMAX]; static int nbg; static int bcTop, bcEnd;
static int nRails;
static int reX, reY;                // restaurant d'altitude
static int chx, chy;                // chalet/refuge (2x2)
static int CPX[MAXGH]; static int CPY0 = -1, CPY1 = -1;  // trace course (remontee 1)

// ---- animations de montagne : damneuses + train ----
typedef struct { float r; int dir, wait; } Groom;   // r = rangee sur la piste
static Groom GR[2];
static int trnOn, trnT100, trnDir, trnWait;          // train entre gare vallee et chalet
static int stX, stY;                                 // petite gare (batiment 2x2)
static int wea = 0;                                  // 0 soleil, 1 coucher de soleil, 2 neige
static int weaT = 900;

// ---- progression ----
static int unlocked = 1;
static int curch = -1;
static int evdone[NM];
static int npass[NM];
static int tpass[NPMAXI];
static int PASX[NPMAXI], PASY[NPMAXI];
static char PNAMEB[32], LNAMEB[NLTMAX][26], VNAMEB[NVM][22];
static char RNAMEB[26], CHANAM[26];   /* v3.4 : restaurant d'altitude + chalet nommes */

// ---- skieur ----
static float skx, sky, hdg, spd;
static int air = 0, crash = 0;
static int fp, bp, spin, grb;
static int score = 0, jrec = 0;
static int combo = 1;
static char hudmsg[44] = ""; static int hudt = 0;
static int curchal = -1;   // -1 libre, 0 slalom, 1 chrono, 2 freeride, 3 course, 4 boarderX
static const char * EVN[5] = { "Slalom", "Chrono", "Freeride", "Course A-B", "BoarderX" };
static const int EVA[5] = { 0, 0, 3, 6, 9 };
static const int EVR[5] = { 1, 1, 2, 2, 3 };
static int chachr = 0, evlimit = 750, evpar = 0, racePar = 0;
static int raceX2, raceY2;
static int lift = 0; static float liftp = 0;
static int nearC = 0, nearL = 0, nearR = 0, rgiven = 0;
static int mode = 0, outfit = 0;   // 0 ski carving, 1 ski freestyle, 2 snow carving, 3 snow freestyle
static const char * MODES[4] = { "Ski carving", "Ski freestyle", "Snow carving", "Snow freestyle" };
static const eadk_color_t OUTC[8] = {
  COL(235,60,60), COL(60,90,225), COL(50,190,80), COL(240,140,30),
  COL(160,70,220), COL(240,100,180), COL(60,210,220), COL(240,220,60)
};
static uint16_t GHOST[GHMAXR][2];
static int ghLen = 0, ghRec = 0;
static int bestT[NM][2];

// ---- PNJ ----
typedef struct { float x, y; int st, qi, rt, kind, spd46; } Npc;
// st: 0 ski, 1 file, 2 remontee, 3 danse, 4 balade
static Npc NC_[NPMAX];
static int NQC = 8, qlen = 0;

// ---- ecrans ----
typedef enum { S_TIT, S_CODE, S_AIDE, S_SKI, S_MAP, S_SEL, S_CHA, S_CPL, S_VES } Screen;
static Screen scr = S_TIT;
static int selv = 0, selv2 = 0, pg = 0, fr = 0, cplType = 0, cham = 0, vestT = 0;
static eadk_keyboard_state_t ks;
static bool prevK[20];
static int coded[4] = {0,0,0,0}; static int ccur = 0;

static bool kd(eadk_key_t k) { return eadk_keyboard_key_down(ks, k); }
static bool pr(eadk_key_t k) { bool v = kd(k); bool p = prevK[k & 19]; prevK[k & 19] = v; return v && !p; }

static void fillR(int x, int y, int w, int h, eadk_color_t c) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (w <= 0 || h <= 0 || x >= W || y >= H) return;
  if (x + w > W) w = W - x;
  if (y + h > H) h = H - y;
  eadk_rect_t r = {(uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h};
  eadk_display_push_rect_uniform(r, c);
}

static void drawStr(const char * s, int x, int y, eadk_color_t tc, eadk_color_t bc) {
  if (x < 0) x = 0;
  eadk_display_draw_string(s, (eadk_point_t){(uint16_t)x, (uint16_t)y}, false, tc, bc);
}

static void sayhud(const char * t) { snprintf(hudmsg, sizeof(hudmsg), "%s", t); hudt = 150; }
static int gatesPassed(void);
static int bxCount_(void);
static void npcInit(void);
static void freeRun(void);

// ================= generation =================
static void carve(int x, int y, uint8_t t) {
  if (x >= 0 && x < GW && y >= 0 && y < GH) stc(x, y, t);
}
static void carveSafe(int x, int y, uint8_t t) {
  if (x >= 0 && x < GW && y >= 0 && y < GH && gt(x, y) != C_BILD && gt(x, y) != C_CLIF)
    stc(x, y, t);
}
static uint8_t cellAt(int x, int y) {
  if (x < 0 || x >= GW || y < 0 || y >= GH) return C_TREE;
  return gt(x, y);
}

static void pisteWalk(int sx, int sy, int tx, int ty, int wide, int isMain, int * sink) {
  int cx = sx;
  for (int yy = imaxi(1, sy); yy <= imini(ty, GH - 2); yy++) {
    if (yy < ty - 2) {
      int drift = (int)(3.0f * fsin(yy * 0.09f + curch * 0.7f + isMain * 3)) + (ri(3) - 1);
      if (drift > 1) cx++;
      else if (drift < -1) cx--;
      if (tx > cx && yy % 2 == 0) cx++;
      if (tx < cx && (yy + 1) % 2 == 0) cx--;
    } else if (yy == ty - 1) cx = tx;
    if (cx < 3) cx = 3;
    if (cx > GW - 4) cx = GW - 4;
    if (isMain) pxx[yy] = cx;
    if (sink) sink[yy] = cx;
    for (int dx = 0; dx < wide; dx++)
      carveSafe(cx - wide / 2 + dx, yy, C_GROOM);
  }
  if (isMain) {
    pxx[0] = pxx[1];
    int last = ty - 1 < 1 ? 1 : ty - 1;
    for (int yy = ty; yy < GH; yy++) pxx[yy] = pxx[last];
  }
}

static void genNames(const Country * ct) {
  int base = ri(NFEAT);
  snprintf(PNAMEB, sizeof(PNAMEB), "%s %s", ct->piste, FEAT[base]);
  for (int k = 0; k < nLifts; k++)
    snprintf(LNAMEB[k], sizeof(LNAMEB[0]), "%s %s", ct->lifts[LT[k].type], FEAT[(base + k) % NFEAT]);
  for (int v = 0; v < NV; v++)
    snprintf(VNAMEB[v], sizeof(VNAMEB[0]), "%s %s", ct->vill, FEAT[(base + 6 + v) % NFEAT]);
  snprintf(RNAMEB, sizeof(RNAMEB), "%s %s", ct->rest, FEAT[(base + 3) % NFEAT]);  /* v3.4 */
  snprintf(CHANAM, sizeof(CHANAM), "%s %s", ct->rest, FEAT[(base + 9) % NFEAT]);
}

static void genMount(int m) {
  curch = m;
  MSpec * sp = &MS[m];
  GW = sp->gw; GH = sp->gh;
  CPY0 = CPY1 = -1;
  memset(tpass, 0, sizeof(tpass));
  srng(0xF00Du + (uint32_t)m * 7919u + 0x5EEDu);
  // hauteurs
  for (int y = 0; y < GH; y++) {
    int base = 235 - 205 * y / (GH - 1);
    for (int x = 0; x < GW; x++) {
      int h = base
        + (int)(14.0f * fsin((x * 0.25f) + y * 0.25f))
        + (int)(10.0f * fsin((x * 0.25f) - y * 0.25f))
        + (int)(8.0f * fsin(x * 0.125f + y * 0.125f))
        + (int)(r01() * 4);
      if (h < 5) h = 5;
      if (h > 252) h = 252;
      HG[y * GW + x] = (uint8_t)h;
    }
  }
  /* v3.4 : lissage 3x3 sur place (3 lignes brutes au lieu d'un 2e tampon plein) */
  {
    static uint8_t rows[3][MAXGW];
    uint8_t * rp = rows[0], * rc = rows[1], * rn = rows[2];
    if (GH > 0) memcpy(rc, HG, (size_t)GW);
    if (GH > 1) memcpy(rn, HG + GW, (size_t)GW);
    for (int y = 0; y < GH; y++) {
      for (int x = 0; x < GW; x++) {
        int s = rc[x], cnt = 1;
        for (int j = -1; j <= 1; j++) {
          int yy = y + j;
          if (yy < 0 || yy >= GH) continue;
          const uint8_t * row = (j < 0) ? rp : ((j == 0) ? rc : rn);
          for (int i2 = -1; i2 <= 1; i2++) {
            int xx = x + i2;
            if (xx < 0) xx = 0; if (xx >= GW) xx = GW - 1;
            s += row[xx]; cnt++;
          }
        }
        HG[y * GW + x] = (uint8_t)(s / cnt);
      }
      { uint8_t * tmp = rp; rp = rc; rc = rn; rn = tmp; }
      if (y + 2 < GH) memcpy(rn, HG + (y + 2) * GW, (size_t)GW);
    }
  }
  for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++) stc(x, y, C_POW);
  // falaises / rochers / arbres
  for (int k = 0; k < sp->cliff; k++) {
    int cx = 6 + ri(GW - 12), cy = 8 + ri(GH - 16);
    int len = 4 + ri(6);
    int dirx = ri(3) - 1;
    for (int i2 = 0; i2 < len; i2++) {
      carve(cx, cy, C_CLIF);
      cx += dirx;
      if (cx < 2 || cx >= GW - 2) break;
      if (r01() < 0.5f) cy++;
    }
  }
  for (int k = 0; k < sp->rocks; k++) {
    int x = ri(GW), y = ri(GH);
    if (HG[y * GW + x] > 55) stc(x, y, C_ROCK);
  }
  for (int k = 0; k < sp->trees / 4; k++) {
    int x = 1 + ri(GW - 2), y = 1 + ri(GH - 2);
    if (HG[y * GW + x] < 200) stc(x, y, C_TREE);
  }
  // ---- forets : bosquets denses (look GMA) ----
  for (int g2 = 0, nf = 10 + m * 2; g2 < nf; g2++) {
    int gx3 = 3 + ri(GW - 6), gy3 = 3 + ri(GH - 12), rad = 2 + ri(3);
    for (int dy2 = -rad; dy2 <= rad; dy2++) for (int dx2 = -rad; dx2 <= rad; dx2++)
      if (dx2 * dx2 + dy2 * dy2 <= rad * rad) {
        int tx2 = gx3 + dx2, ty2 = gy3 + dy2;
        if (tx2 > 0 && tx2 < GW - 1 && ty2 > 0 && ty2 < GH - 4
            && gt(tx2, ty2) == C_POW && HG[ty2 * GW + tx2] < 200)
          stc(tx2, ty2, C_TREE);
      }
  }
  // ---- remontees (5) ----
  nLifts = imini(5 + m / 3, 7 + m / 9);   // 5 remontees au debut, jusqu'a 8 sur les enormes
  for (int k = 0; k < nLifts; k++) LT[k].unlock = LUNS[k];
  LT[0].type = 0; LT[1].type = 1; LT[2].type = 2; LT[3].type = 3;
  LT[4].type = (m % 2) ? 2 : 1;
  chy = GH - 7;
  chx = GW / 2 + ri(GW / 4) - GW / 8;
  if (chx < 6) chx = 6;
  if (chx > GW - 8) chx = GW - 8;
  for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) carve(chx + dx, chy + dy, C_BILD);
  LT[0].x1 = chx; LT[0].y1 = chy + 1;
  { int tx = chx + 10 + ri(GW - 20); if (tx < 5) tx = 5; if (tx > GW - 6) tx = GW - 6;
    LT[0].x2 = tx; LT[0].y2 = 3 + ri(3); }
  // villages
  NV = m >= 4 ? 3 : 2;
  VX[0] = chx; VY[0] = chy;
  for (int v = 1; v < NV; v++) {
    int vx2 = chx + (v == 1 ? -(8 + ri(10)) : (8 + ri(10)));
    if (vx2 < 6) vx2 = 6 + ri(4);
    if (vx2 > GW - 8) vx2 = GW - 10 - ri(2);
    VX[v] = vx2; VY[v] = GH - 7 - ri(3);
    for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) carve(vx2 + dx, VY[v] + dy, C_BILD);
    for (int h = 0; h < 4; h++) {
      int hx = vx2 + ri(9) - 4, hy = VY[v] + ri(5) - 1;
      carve(hx, hy, C_BILD);
    }
  }
  // pistes des remontees 1..4
  for (int k = 1; k < nLifts; k++) {
    int bi = k <= 2 ? 1 : 2;
    if (bi >= NV) bi = 0;
    int tx = VX[bi] + (ri(31) - 15);
    int ty = 5 + ri(GH / 3);
    if (tx < 5) tx = 5;
    if (tx > GW - 6) tx = GW - 6;
    LT[k].x1 = VX[bi]; LT[k].y1 = VY[bi] + 2;
    if (iabsi(tx - VX[bi]) < 8) tx = (VX[bi] + GW / 2) % (GW - 10) + 5;
    LT[k].x2 = tx; LT[k].y2 = ty;
    pisteWalk(tx, ty, LT[k].x1, LT[k].y1, 2, 0, k == 1 ? CPX : 0);
    if (k == 1) { CPY0 = ty; CPY1 = LT[k].y1; }
  }
  // piste principale (remontee 0)
  pxx[0] = LT[0].x2;
  pisteWalk(LT[0].x2, LT[0].y2, LT[0].x1, LT[0].y1, 3, 1, 0);
  // restaurant d'altitude pres du sommet de la remontee 2
  reX = LT[2].x2 + 2; reY = LT[2].y2;
  if (reX > GW - 4) reX = GW - 4;
  for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) carve(reX + dx, reY + dy, C_BILD);
  // kickers de la piste principale
  nk = 0;
  for (int yy = 12; yy < GH - 10 && nk < NKMAX; yy += 18) {
    int x = pxx[yy] + (ri(7) - 3);
    if (x > 1 && x < GW - 2 && gt(x, yy) != C_BILD && gt(x, yy) != C_CLIF) {
      kx[nk] = x; ky[nk] = yy; nk++;
    }
  }
  for (int i2 = 0; i2 < nk; i2++) carve(kx[i2], ky[i2], C_KICK);
  // portes de slalom
  ng = 0;
  for (int yy = 12; yy < GH - 8 && ng < NGMAX; yy += 8) {
    gx[ng] = pxx[yy]; gy[ng] = yy; gdone[ng] = 0; ng++;
  }
  // ---- snowpark : de plus en plus grand et style avec le niveau de la montagne ----
  nRails = 0;
  {
    int niv = imini(m, 9);
    int rows = 1 + imini(2, niv / 4);                 // 1 rangée ( debut) -> 3 rangées
    int rlen = 4 + imini(4, niv / 2);                 // rails de 4 -> 8 cases
    int px2 = LT[2].x2 + (ri(9) - 4), py2 = LT[2].y2 + 12 + ri(6);
    if (px2 < 4) px2 = 4;
    if (px2 > GW - rlen - 3) px2 = GW - rlen - 3;
    if (px2 < 4) px2 = 4;
    if (py2 > GH - rows * 4 - 10) py2 = GH - rows * 4 - 10;
    if (py2 < 8) py2 = 8;
    for (int rr = 0; rr < rows; rr++) {
      for (int dx = 0; dx < rlen; dx++) { carve(px2 + dx, py2 + rr * 4, C_RAIL); nRails++; }
      carve(px2 + (rr % 2 ? rlen : -1), py2 + rr * 4 + 2, C_KICK);
      carveSafe(px2 + (rr % 2 ? rlen + 1 : -2), py2 + rr * 4 + 2, C_KICK);
      if (niv >= 7 && rr == 1 && rows >= 3)           // style : double rail central
        for (int dx = 0; dx < rlen; dx++) { carve(px2 + dx, py2 + rr * 4 + 1, C_RAIL); nRails++; }
    }
  }
  // ---- boarder cross : couloir depuis remontee 1 ----
  nbg = 0; bcTop = LT[1].y2 + 1; bcEnd = GH - 6;
  {
    int cx = LT[1].x2, side = 1;
    for (int yy = bcTop; yy < bcEnd && yy < GH - 4; yy++) {
      if (cx < 3) cx = 3;
      if (cx > GW - 4) cx = GW - 4;
      for (int dx = -1; dx <= 0; dx++) carveSafe(cx + dx, yy, C_GROOM);
      if ((yy - bcTop) % 9 == 0 && yy < bcEnd - 4 && nbg < NGMAX) {
        int gxp = cx + side;
        side = -side;
        if (gxp < 2) gxp = cx + 1;
        if (gxp > GW - 3) gxp = cx - 1;
        bx[nbg] = gxp; by[nbg] = yy + 2; bdone[nbg] = 0; nbg++;
        carveSafe(gxp, yy + 2, C_GATE);
      }
      if ((yy - bcTop) % 15 == 7) carveSafe(cx + 1, yy + 1, C_KICK);
      if (r01() < 0.4f) cx += (ri(3) - 1);
    }
  }
  // ---- bosses (moguls hors-piste) ----
  for (int k = 0, nbm = 20 + ri(20); k < nbm; k++) {
    int xb = 2 + ri(GW - 4), yb = 6 + ri(GH - 14);
    if (gt(xb, yb) == C_POW && iabsi(xb - pxx[yb]) > 3) stc(xb, yb, C_BOSS);
  }
  // ---- halfpipe serpentin + rampes (montagnes avancees) ; rampes discretes partout ----
  if (m >= 4) {
    int hpx = 4 + ri(GW - 16), hpy = 10 + ri(GH - 26);
    if (hpx > GW - 5) hpx = GW - 5;
    if (hpy > GH - 20) hpy = GH - 20;
    for (int rr = 0; rr < 14; rr++) {
      int xx = hpx + (int)(3.5f * fsin(rr * 0.45f));
      if (xx < 4) xx = 4;
      if (xx > GW - 5) xx = GW - 5;
      carve(xx, hpy + rr, C_PIPE);
      carveSafe(xx - 1, hpy + rr, C_RAMP);
      carveSafe(xx + 1, hpy + rr, C_RAMP);
    }
  }
  for (int k = 0, nr = 20 + m * 2; k < nr; k++) {
    int xb = 2 + ri(GW - 4), yb = 6 + ri(GH - 14);
    if (gt(xb, yb) == C_POW && iabsi(xb - pxx[yb]) > 3) stc(xb, yb, C_RAMP);
  }
  // ---- pass caches hors-piste ----
  int np2 = 0, guard = 0;
  while (np2 < NPMAXI && guard < 4000) {
    guard++;
    int x = 2 + ri(GW - 4), y = 4 + ri(GH - 10);
    if (gt(x, y) != C_POW) continue;
    if (iabsi(x - pxx[y]) <= 2) continue;
    int ok2 = 1;
    for (int k = 0; k < nLifts; k++)
      if (iabsi(x - LT[k].x1) + iabsi(y - LT[k].y1) < 5) ok2 = 0;
    if (!ok2) continue;
    for (int i2 = 0; i2 < np2; i2++)
      if (iabsi(x - PASX[i2]) + iabsi(y - PASY[i2]) < 6) { ok2 = 0; break; }
    if (!ok2) continue;
    PASX[np2] = x; PASY[np2] = y; np2++;
  }
  while (np2 < NPMAXI) { PASX[np2] = PASX[0]; PASY[np2] = PASY[0]; np2++; }
  // ---- damneuses (IA : patrouillent la piste principale) ----
  GR[0].r = (float)(GH - 8); GR[0].dir = -1; GR[0].wait = 0;
  GR[1].r = (float)(GH / 2); GR[1].dir = 1; GR[1].wait = 150;
  // ---- train de montagne : gare de vallee <-> chalet (monts pairs) ----
  trnOn = ((m & 1) == 0);
  trnT100 = 0; trnDir = 1; trnWait = 60;
  stX = 0; stY = 0;
  if (trnOn) {
    stX = VX[0] + 4; if (stX > GW - 5) stX = GW - 5;
    stY = VY[0];
    for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++)
      carveSafe(stX + dx, stY + dy, C_BILD);          // petite gare
  }
  // dernier : porte de depart de course en haut de piste
  for (int dx = -2; dx <= 2; dx++) carveSafe(pxx[6] + dx, 6, C_STRT);
  // profil monotone le long de la piste principale : jamais de cuvette ni de
  // contrepente, la descente de bout en bout ne s'arrete jamais
  {
    int yTop = LT[0].y2; if (yTop < 1) yTop = 1;
    int hTop = HG[yTop * GW + pxx[yTop]];
    int yBot = GH - 6;
    int hBot = HG[yBot * GW + pxx[yBot]];
    if (hBot > hTop - 30) hBot = hTop - 30;
    for (int y2m = yTop; y2m < yBot; y2m++) {
      int tgt = hTop - (hTop - hBot) * (y2m - yTop) / (yBot - yTop);
      for (int ddx = -1; ddx <= 1; ddx++) {
        int xx2 = pxx[y2m] + ddx; if (xx2 < 1) xx2 = 1; if (xx2 >= GW - 1) xx2 = GW - 1;
        if (HG[y2m * GW + xx2] > (uint8_t)tgt) HG[y2m * GW + xx2] = (uint8_t)tgt;
      }
    }
    for (int y2m = yTop; y2m < yBot; y2m++) {
      int h1 = HG[y2m * GW + pxx[y2m]], h2 = HG[(y2m + 1) * GW + pxx[y2m + 1]];
      if (h2 > h1) HG[(y2m + 1) * GW + pxx[y2m + 1]] = (uint8_t)h1;
    }
  }
  // types logiques selon le denivele : les 5 remontees triees du plus petit au
  // plus grand denivele -> tire-fesse / telesiege / telesiege / telecabine /
  // telepherique (immense et pentue)
  {
    int rk[NLTMAX];
    for (int k2 = 0; k2 < nLifts; k2++) rk[k2] = k2;
    for (int a2 = 1; a2 < nLifts; a2++) for (int b2 = 0; b2 < a2; b2++)
      if (LT[rk[b2]].y1 - LT[rk[b2]].y2 > LT[rk[a2]].y1 - LT[rk[a2]].y2) {
        int t2 = rk[a2]; rk[a2] = rk[b2]; rk[b2] = t2;
      }
    for (int k2 = 0; k2 < nLifts; k2++)    // rang 0 = tire-fesse ... dernier = telepherique
      LT[rk[k2]].type = (k2 == 0) ? 0 : (k2 >= nLifts - 1 ? 3 : (k2 >= (nLifts - 1) / 2 + 1 ? 2 : 1));
  }
  // noms (pays)
  genNames(&CO[CC[m]]);
  // PNJ
  npcInit();
}

// ================= PNJ =================
static void npcInit(void) {
  qlen = 0;
  for (int i2 = 0; i2 < 4; i2++) {
    int row = 10 + ri(GH / 2);
    NC_[i2].x = (float)pxx[row] + (float)(ri(5) - 2);
    NC_[i2].y = (float)row;
    NC_[i2].st = 0; NC_[i2].qi = 0; NC_[i2].rt = 0;
    NC_[i2].kind = i2 % 3;
    NC_[i2].spd46 = 10 + ri(12);
  }
  NC_[4].st = 3; NC_[4].x = (float)reX; NC_[4].y = (float)((float)reY + 2.5f);
  NC_[5].st = 3; NC_[5].x = (float)((float)reX + 2); NC_[5].y = (float)((float)reY + 3);
  NC_[4].kind = 0; NC_[5].kind = 2; NC_[4].spd46 = 0; NC_[5].spd46 = 0; NC_[4].qi = 0; NC_[5].qi = 0; NC_[4].rt = 0; NC_[5].rt = 0;
  NC_[6].st = 4; NC_[6].x = (float)((float)VX[0] - 3); NC_[6].y = (float)((float)VY[0] + 3);
  NC_[7].st = 4; NC_[7].x = (float)((float)VX[0] + 2); NC_[7].y = (float)((float)VY[0] + 4);
  NC_[6].kind = 1; NC_[7].kind = 2; NC_[6].spd46 = 6; NC_[7].spd46 = 8;
  NC_[6].qi = 0; NC_[7].qi = 0; NC_[6].rt = ri(40); NC_[7].rt = ri(40);
  NQC = 8;
}

static void scync(void) {
  if (GH < 20) return;
  for (int g = 0; g < 2; g++) {
    if (GR[g].wait > 0) { GR[g].wait--; continue; }
    GR[g].r += 0.05f * (float)GR[g].dir;
    if (GR[g].r <= 4.0f) { GR[g].r = 4.0f; GR[g].dir = 1; GR[g].wait = 220; }
    if (GR[g].r >= (float)(GH - 7)) { GR[g].r = (float)(GH - 7); GR[g].dir = -1; GR[g].wait = 260; }
  }
  if (trnOn) {
    if (trnWait > 0) trnWait--;
    else {
      trnT100 += 2 * trnDir;
      if (trnT100 >= 100) { trnT100 = 100; trnDir = -1; trnWait = 90; }
      if (trnT100 <= 0) { trnT100 = 0; trnDir = 1; trnWait = 90; }
    }
  }
}

static void npcTick(void) {
  if (nLifts == 0) return;
  int qchanged = 0;
  for (int i2 = 0; i2 < 4; i2++) {
    Npc * n = &NC_[i2];
    if (n->st == 0) {
      int row2 = (int)n->y + 2; if (row2 >= GH) row2 = GH - 1;
      int tx = pxx[row2] + (n->kind % 3) - 1;
      float mv = (float)n->spd46 / 140.0f;
      if ((float)tx > n->x) n->x += mv * 0.25f; else if ((float)tx < n->x) n->x -= mv * 0.25f;
      n->y += mv;
      if (n->x >= GW - 1) n->x = GW - 1;
      if (n->y >= GH - 7) { n->st = 1; n->qi = qlen++; n->rt = 1; qchanged = 1; }
    } else if (n->st == 1) {
      if (--n->rt <= 0) {
        if (n->qi == 0) { n->st = 2; n->rt = ASP[LT[0].type]; qchanged = 1; }
        else n->rt = 1;
      }
    } else if (n->st == 2) {
      float pp = 1.0f - (float)n->rt / (float)ASP[LT[0].type];
      n->x = (float)((float)LT[0].x1 + ((float)LT[0].x2 - (float)LT[0].x1) * pp);
      n->y = (float)((float)LT[0].y1 + ((float)LT[0].y2 - (float)LT[0].y1) * pp);
      if (--n->rt <= 0) { n->st = 0; n->x = (float)LT[0].x2; n->y = (float)((float)LT[0].y2 + 1); qchanged = 1; }
    }
  }
  if (qchanged) {
    int q2 = 0;
    for (int i2 = 0; i2 < 4; i2++) if (NC_[i2].st == 1) NC_[i2].qi = q2++;
    qlen = q2;
    for (int i2 = 0; i2 < 4; i2++)
      if (NC_[i2].st == 1 && NC_[i2].rt <= 0) NC_[i2].rt = 50 * (NC_[i2].qi + 1);
  }
  for (int i2 = 0; i2 < 4; i2++)
    if (NC_[i2].st == 1) {
      NC_[i2].x = (float)((float)LT[0].x1 - 3.5f - (float)(NC_[i2].qi % 4));
      NC_[i2].y = (float)((float)LT[0].y1 + 0.5f + (float)(NC_[i2].qi / 4));
    }
  for (int i2 = 6; i2 < 8; i2++) {
    Npc * n = &NC_[i2];
    n->x += (n->spd46 > 7 ? 0.05f : -0.05f);
    if (i2 == 6) {
      if (n->x < (float)((float)VX[0] - 3)) n->spd46 = 8;
      if (n->x > (float)((float)VX[0] + 3)) n->spd46 = 6;
    } else {
      if (n->x < (float)((float)VX[0] - 2)) n->spd46 = 10;
      if (n->x > (float)((float)VX[0] + 4)) n->spd46 = 8;
    }
  }
}

// ================= progression =================
static int codeFor(int level) { return (4117 + level * 411 + ((level * level) % 97)) % 10000; }
static int needM(int m) { return 12 + imini(m, 3); }

static void checkUnlock(void) {
  if (curch < 0 || curch + 1 >= NM) return;
  if (npass[curch] >= needM(curch) && unlocked == curch + 1) {
    unlocked = curch + 2;
    snprintf(hudmsg, sizeof(hudmsg), "%s debloque ! Code %04d", MNAMES[curch + 1], codeFor(curch + 1));
    hudt = 300;
  }
}

static void completeChallenge(int ev) {
  int first = (evdone[curch] & (1 << ev)) == 0;
  if (first) {
    evdone[curch] |= (1 << ev);
    npass[curch] += EVR[ev];
  } else {
    score += 150;
  }
  if ((ev == 1 || ev == 3) && ghRec > 0) {
    int slot = ev == 1 ? 0 : 1;
    if (bestT[curch][slot] == 0 || chachr < bestT[curch][slot]) {
      bestT[curch][slot] = chachr;
      ghLen = ghRec;
    }
  }
  snprintf(hudmsg, sizeof(hudmsg), "%s reussi !%s +%d PASS", EVN[ev],
           first ? "" : " (deja fait)", first ? EVR[ev] : 0);
  hudt = 300;
  checkUnlock();
}


static void newMountain(int m) {
  genMount(m);
  freeRun();
  curchal = -1;
  snprintf(hudmsg, sizeof(hudmsg), "%s - %s", MNAMES[m], CO[CC[m]].nom);
  hudt = 220;
}

// ================= physique =================
static void freeRun(void) {
  skx = (float)pxx[3]; sky = 3.0f;
  hdg = 0; spd = 0.30f; air = 0; crash = 0;
  fp = bp = spin = grb = 0;
  for (int i2 = 0; i2 < ng; i2++) gdone[i2] = 0;
  for (int i2 = 0; i2 < nbg; i2++) bdone[i2] = 0;
  score = 0; combo = 1; chachr = 0; lift = 0; liftp = 0; ghRec = 0;
}

static void boardLift(int k) {
  skx = (float)LT[k].x1; sky = (float)((float)LT[k].y1 + 0.5f);
  lift = k + 1; liftp = 0; spd = 0; hdg = 0; air = 0; crash = 0;
  sayhud(LNAMEB[k]);
}

static void doCrash(void) {
  crash = 40; spd = 0.0f; score = imaxi(0, score - 30);
  if (combo > 1) combo = 1;
  sayhud("CRASH!");
}

static void phys(void) {
  if (lift > 0) {
    liftp += 1.0f / (float)ASP[LT[lift - 1].type];
    if (liftp >= 1.0f) {
      skx = (float)LT[lift - 1].x2; sky = (float)((float)LT[lift - 1].y2 + 1);
      spd = 0.05f; hdg = 0; lift = 0; liftp = 0;
      air = 0; crash = 0;
      sayhud("Sommet !");
    }
    return;
  }
  if (crash > 0) {
    crash--;
    if (crash == 0) {
      int row = (int)sky;
      if (row < 1) row = 1;
      if (row > GH - 8) row = GH - 8;
      skx = (float)pxx[row]; sky = (float)row;
      spd = 0.30f; air = 0;
    }
    return;
  }
  nearC = 0; nearL = 0; nearR = 0;
  float turn = 0.055f * ((mode & 1) ? 1.6f : 1.0f);
  if (air == 0) {
    if (kd(eadk_key_left)) hdg -= turn + (spd > 0.35f ? 0.02f : 0);
    if (kd(eadk_key_right)) hdg += turn + (spd > 0.35f ? 0.02f : 0);
    if (hdg < -1.2f) hdg = -1.2f;
    if (hdg > 1.2f) hdg = 1.2f;
  }
  float dx = fsin(hdg), dy = fcos(hdg);
  int cxi = (int)(skx + dx * 1.2f), cyi = (int)(sky + dy * 1.2f);
  if (cxi < 0) cxi = 0;
  if (cxi >= GW) cxi = GW - 1;
  if (cyi < 0) cyi = 0;
  if (cyi >= GH) cyi = GH - 1;
  int ddx = iabsi((int)skx - cxi), ddy = iabsi((int)sky - cyi);
  if (ddx < 1) ddx = 1;
  if (ddy < 1) ddy = 1;
  float dz = (float)(HG[cyi * GW + cxi] - HG[(int)sky * GW + (int)skx]);
  float slope = dz / (float)(ddx > ddy ? ddx : ddy);
  float acc = -slope * 0.15f + (kd(eadk_key_up) ? 0.004f : 0.0f);
  spd += acc - 0.004f;
  const uint8_t ct = cellAt((int)(skx + dx * 1.2f), (int)(sky + dy * 1.2f));
  if (kd(eadk_key_down)) spd -= 0.02f;
  float vg = (mode & 1) ? 0.64f : 0.72f;
  float vp = (mode & 1) ? 0.50f : 0.55f;
  float vo = (mode & 1) ? 0.36f : 0.40f;
  float vmax = (ct == C_GROOM || ct == C_GATE || ct == C_STRT || ct == C_PASG) ? vg
             : (ct == C_POW || ct == C_KICK || ct == C_PIPE || ct == C_RAMP) ? vp : vo;
  if (spd > vmax) spd = vmax;
  if (spd < 0.02f) spd = 0.02f;
  float nx = skx + dx * spd, ny = sky + dy * spd;
  if (nx < 0.5f) { nx = 0.5f; spd *= 0.5f; }
  if (nx >= GW - 0.5f) { nx = GW - 0.5f; spd *= 0.5f; }
  if (ny < 0.5f) { ny = 0.5f; spd *= 0.5f; }
  if (ny >= GH - 0.5f) { ny = GH - 0.5f; spd *= 0.5f; }
  const uint8_t ncell = cellAt((int)nx, (int)ny);
  if (ncell == C_TREE || ncell == C_ROCK) {
    if (air == 0) { doCrash(); return; }
  }
  if (ncell == C_CLIF && air == 0) {
    air = (mode & 1) ? 20 : 16; fp = bp = 0; spin = 0; grb = 0; spd += 0.12f; score += 20 * combo;
    sayhud("Falaise ! (+20)");
  }
  if (ncell == C_KICK && air == 0) {
    air = (mode & 1) ? 26 : 22; fp = bp = 0; spin = 0; grb = 0; spd += 0.15f; score += 50 * combo;
    int dist2 = (int)(spd * (float)air * 130.0f / 2.0f / 10.0f);
    if (dist2 > jrec) { jrec = dist2; snprintf(hudmsg, sizeof(hudmsg), "KICKER ! record %d m", dist2); hudt = 150; }
    else { snprintf(hudmsg, sizeof(hudmsg), "KICKER ! compose ta figure"); hudt = 150; }
  }
  if (ncell == C_RAIL && air == 0) { score += 30; sayhud("Rail ! +30"); spd = 0.20f; }
  if (ncell == C_BOSS && air == 0) { air = 2 + ri(4); spd -= 0.03f; score += 10; }
  if (ncell == C_PIPE && air == 0) { score += 2; if (spd < 0.55f) spd += 0.012f; }
  if (ncell == C_RAMP && air == 0) { air = 4 + ri(4); spd += 0.02f; score += 25; }
  skx = nx; sky = ny;
  // ---- figures en l'air ----
  if (air > 0) {
    air--;
    if (pr(eadk_key_left)) { spin++; hdg -= 0.55f; }
    if (pr(eadk_key_right)) { spin++; hdg += 0.55f; }
    if (pr(eadk_key_up)) fp++;
    if (pr(eadk_key_down)) bp++;
    if (kd(eadk_key_ok)) grb++;
    if (air == 0) {
      int flips = fp + bp;
      int cn = (mode & 1) ? 1 : 2;
      int mult = combo > 4 ? 4 : combo;
      if (flips || spin || grb) {
        int b = 20 + flips * 70 + spin * 45 + imaxi(0, imini(grb, 15)) * 3;
        if (mode & 1) b = b * 12 / 10;
        b *= mult;
        score += b;
        char mid[26];
        mid[0] = 0;
        if (fp && bp) snprintf(mid, sizeof(mid), "Salto Dbl");
        else if (fp) snprintf(mid, sizeof(mid), "Salto Av%s", fp > 1 ? "x2" : "");
        else if (bp) snprintf(mid, sizeof(mid), "Salto Ar%s", bp > 1 ? "x2" : "");
        snprintf(hudmsg, sizeof(hudmsg), "%s%s%s%s +%d", mid,
                 spin ? (mid[0] ? " +Vrille" : "Vrille") : "",
                 grb ? ((mid[0] || spin) ? " +Grab" : "Grab") : "",
                 (flips + spin >= cn) ? " COMBO!" : "", b);
        hudt = 150;
        if (flips + spin >= cn && combo < 9) combo++;
      }
      fp = bp = 0;
      if ((mode == 0 || mode == 2) && spin >= 4) { // carving : pas de switch !
        int sw = spin;
        spin = 0;
        doCrash();
        snprintf(hudmsg, sizeof(hudmsg), "Switch en carving ! CHUTE (%d)", sw);
        hudt = 150;
        return;
      }
      spin = 0;
      uint8_t lc = cellAt((int)skx, (int)sky);
      if (lc == C_TREE || lc == C_ROCK || lc == C_CLIF || lc == C_BILD) { doCrash(); return; }
    }
  } else {
    fp = bp = 0;
  }
  // pass caches
  for (int i2 = 0; i2 < NPMAXI; i2++) {
    if (!tpass[i2] && iabsi((int)skx - PASX[i2]) <= 1 && iabsi((int)sky - PASY[i2]) <= 1) {
      tpass[i2] = 1;
      npass[curch]++;
      int remM = needM(curch) - npass[curch];
      int remL = 99;
      for (int k2 = 0; k2 < nLifts; k2++)
        if (LT[k2].unlock > npass[curch]) remL = imini(remL, LT[k2].unlock - npass[curch]);
      char hm[80];
      if (remM <= 0)
        snprintf(hm, sizeof(hm), "Bravo ! Ski pass pret : montagne suivante !");
      else if (remL <= 2 && remL < remM)
        snprintf(hm, sizeof(hm), "Bravo ! Ski pass ! Plus que %d et une remontee !", remL);
      else
        snprintf(hm, sizeof(hm), "Bravo ! Ski pass ! Plus que %d pour la montagne suivante", remM);
      snprintf(hudmsg, sizeof(hudmsg), "%s", hm);
      hudt = 240;
      checkUnlock();
    }
  }
  // signatures slalom
  if (curchal == 0) {
    int row = (int)ny;
    for (int i2 = 0; i2 < ng; i2++)
      if (!gdone[i2] && gy[i2] == row && iabsi((int)nx - gx[i2]) <= 1) {
        gdone[i2] = 1; score += 100 * combo; combo++;
        sayhud("Porte ! (+100)");
      }
  }
  // signatures boarder X
  if (curchal == 4) {
    int row = (int)ny;
    for (int i2 = 0; i2 < nbg; i2++)
      if (!bdone[i2] && by[i2] == row && iabsi((int)nx - bx[i2]) <= 1) {
        bdone[i2] = 1; score += 80 * combo; combo++;
        sayhud("Porte BX ! (+80)");
      }
  }
  if (curchal == 1 || curchal == 3 || curchal == 4) chachr++;
  if (curchal == 2) {
    chachr++;
    if (chachr >= evlimit) {
      if (score >= 400) { cplType = 2; scr = S_CPL; return; }
      curchal = -1;
      sayhud("Temps ecoule !");
    }
  }
  // enregistrement fantome
  if ((curchal == 1 || curchal == 3) && ghRec < GHMAXR) {
    GHOST[ghRec][0] = (uint16_t)(skx * 16.0f);
    GHOST[ghRec][1] = (uint16_t)(sky * 16.0f);
    ghRec++;
  }
  // arrivee course
  if (curchal == 3) {
    if (iabsi((int)skx - raceX2) <= 2 && iabsi((int)sky - raceY2) <= 2) {
      if (chachr <= racePar) { cplType = 3; scr = S_CPL; return; }
      snprintf(hudmsg, sizeof(hudmsg), "Trop lent ! (%ds > %ds)", chachr / 13, racePar / 13);
      hudt = 220;
      curchal = -1;
    }
  }
  // fin boarder X
  if (curchal == 4) {
    if ((int)sky >= bcEnd) {
      if (bxCount_() >= nbg) { cplType = 4; scr = S_CPL; return; }
      if (hudt <= 0) {
        snprintf(hudmsg, sizeof(hudmsg), "Il reste %d portes BX", nbg - bxCount_());
        hudt = 100;
      }
    }
  }
  // arrivee chalet
  if (iabsi((int)skx - chx) <= 2 && iabsi((int)sky - chy) <= 1) {
    if (curchal == 0) {
      if (gatesPassed() >= ng) { cplType = 0; spd = 0; scr = S_CPL; return; }
      skx -= dx * spd; sky -= dy * spd; spd = 0.02f;
      nearC = 1;
      snprintf(hudmsg, sizeof(hudmsg), "Il reste %d portes", ng - gatesPassed());
      hudt = 100;
    } else if (curchal == 1) {
      if (chachr <= evpar) { cplType = 1; scr = S_CPL; return; }
      snprintf(hudmsg, sizeof(hudmsg), "Par rate ! (%ds > %ds) rejoue !", chachr / 13, evpar / 13);
      hudt = 220;
      curchal = -1;
    } else if (curchal == 2) {
      if (score >= 400) { cplType = 2; scr = S_CPL; return; }
      skx -= dx * spd; sky -= dy * spd; spd = 0.02f;
      nearC = 1;
      snprintf(hudmsg, sizeof(hudmsg), "Freeride : %d pts !", score);
      hudt = 100;
    } else {
      skx -= dx * spd; sky -= dy * spd; spd = 0.02f;
      nearC = 1;
    }
  }
  // restaurant d'altitude
  if (iabsi((int)skx - reX - 1) <= 2 && iabsi((int)sky - reY - 1) <= 2) {
    nearR = 1;
    if (!rgiven) { score += 50; snprintf(hudmsg, sizeof(hudmsg), "Fondue au resto ! +50"); hudt = 150; rgiven = 1; }
  } else rgiven = 0;
  // proximite station de remontee
  if (nearC == 0 && lift == 0) {
    for (int k = 0; k < nLifts; k++) {
      if (npass[curch] >= LT[k].unlock && iabsi((int)skx - LT[k].x1) <= 2 && iabsi((int)sky - LT[k].y1) <= 2) {
        nearL = k + 1;
        break;
      }
    }
  }
}

static int bxCount_(void) { int n2 = 0; for (int i2 = 0; i2 < nbg; i2++) if (bdone[i2]) n2++; return n2; }
static int gatesPassed(void) { int n2 = 0; for (int i2 = 0; i2 < ng; i2++) if (gdone[i2]) n2++; return n2; }

// ================= affichage =================
static eadk_color_t weaTint(eadk_color_t c) {
  if (wea == 0) return c;
  int r5 = (c >> 11) & 31, g6 = (c >> 5) & 63, b5 = (int)(c & 31);
  if (wea == 1) {           // coucher de soleil : tire vers l'orange
    r5 = imini(31, r5 + 6);
    g6 = imini(63, g6 + 2);
    b5 = b5 * 3 / 4 + 2;
  } else {                  // neige : tout tire vers le gris bleu
    r5 = r5 * 3 / 4;
    g6 = g6 * 3 / 4;
    b5 = imini(31, b5 + 2);
  }
  return (eadk_color_t)((r5 << 11) | (g6 << 5) | b5);
}
static void drawSlope(int camx, int camy) {
  for (int j = 0; j < 26; j++) {
    int gyv = camy + j;
    int sy = TOP + j * 8;
    if (gyv < 0 || gyv >= GH) { fillR(0, sy, W, 8, weaTint(CSNOW)); continue; }
    for (int i2 = 0; i2 < 41; i2++) {
      int gxv = camx + i2;
      if (gxv < 0 || gxv >= GW) { fillR(i2 * 8, sy, 8, 8, weaTint(CSNOW)); continue; }
      uint8_t t = gt(gxv, gyv);
      uint8_t h = HG[gyv * GW + gxv];
      // ombrage du relief : face au soleil / ombre, comme un vrai domaine
      uint8_t hl = HG[gyv * GW + imaxi(gxv - 1, 0)];
      uint8_t hu = HG[imaxi(gyv - 1, 0) * GW + gxv];
      int sh = ((int)h - (int)hl) + ((int)h - (int)hu) / 2;
      eadk_color_t c;
      if (sh > 4) c = COL(250,252,255);         // face ensoleillee
      else if (sh < -3) c = COL(208,219,242);   // ombre de la pente
      else c = COL(234,240,252);
      if (h > 205 && sh > 0) c = COL(246,248,255);
      if (t == C_GROOM) {
        c = CGRM;
        uint8_t gl = gt(imaxi(gxv - 1, 0), gyv), gu = gt(gxv, imaxi(gyv - 1, 0));
        if (gl != C_GROOM || gu != C_GROOM) c = COL(178,196,232);   // bord de piste
      }
      if (t != C_GROOM) c = weaTint(c);
      else if (wea == 1) c = weaTint(c);
      fillR(i2 * 8, sy, 8, 8, c);
      if (t == C_POW && ((gxv + gyv) % 3) == 0) fillR(i2 * 8 + 3, sy + 3, 1, 1, weaTint(CPOWD));
    }
  }
  if (curchal == 1 || curchal == 3) {
    for (int yy = 4; yy < GH - 6; yy += 2) {
      int col = curchal == 1 ? pxx[yy] : ((yy >= CPY0 && yy <= CPY1) ? CPX[yy] : -1);
      if (col < 0) continue;
      int sx = (col - camx) * 8, sy = TOP + (yy - camy) * 8;
      fillR(sx + 3, sy, 2, 8, curchal == 1 ? COL(235,70,80) : COL(80,140,240));
    }
  }
}

static void drawObjects(int camx, int camy) {
  for (int j = 0; j < 26; j++) {
    int gyv = camy + j;
    if (gyv < 0 || gyv >= GH) continue;
    int sy = TOP + j * 8;
    for (int i2 = 0; i2 < 41; i2++) {
      int gxv = camx + i2;
      if (gxv < 0 || gxv >= GW) continue;
      uint8_t t = gt(gxv, gyv);
      int px2 = i2 * 8, py2 = sy;
      if (t == C_TREE) {
        fillR(px2 + 3, py2 + 5, 2, 3, COL(70,50,35));
        fillR(px2 + 1, py2 - 2, 6, 8, CTREE);
        fillR(px2 + 2, py2 - 4, 4, 4, CTRE2);
      } else if (t == C_ROCK) {
        fillR(px2 + 1, py2 + 2, 6, 5, CROCK);
        fillR(px2 + 2, py2 + 1, 4, 2, COL(150,145,138));
      } else if (t == C_CLIF) {
        fillR(px2, py2, 8, 3, CCLIF);
        fillR(px2, py2 + 3, 8, 5, COL(50,48,45));
      } else if (t == C_KICK) {
        fillR(px2 + 1, py2 + 4, 6, 3, CKICK);
        fillR(px2 + 2, py2 + 2, 4, 2, COL(180,110,30));
      } else if (t == C_BILD) {
        fillR(px2, py2 + 2, 8, 6, CBILD);
        fillR(px2 + 1, py2, 6, 3, COL(172,66,52));                   // toit rouge chalet
        fillR(px2 + 1, py2 + 2, 6, 1, COL(210,90,70));
      } else if (t == C_RAIL) {
        fillR(px2, py2 + 3, 8, 2, COL(90,95,110));
      } else if (t == C_BOSS) {
        fillR(px2 + 1, py2 + 2, 6, 5, COL(255,255,255));
        fillR(px2 + 2, py2 + 3, 4, 2, COL(208,218,240));
        fillR(px2 + 1, py2 + 7, 6, 1, COL(150,165,200));
      } else if (t == C_PIPE) {
        fillR(px2, py2 + 1, 8, 6, COL(120,160,220));
        fillR(px2 + 1, py2 + 3, 6, 3, COL(225,238,255));
      } else if (t == C_RAMP) {
        fillR(px2, py2 + 1, 8, 6, COL(165,175,195));
        fillR(px2, py2 + 1, 8, 2, COL(223,230,248));
      } else if (t == C_PASG) {
        for (int pi = 0; pi < NPMAXI; pi++) {
          if (!tpass[pi] && PASX[pi] == gxv && PASY[pi] == gyv) {
            fillR(px2 + 2, py2 + 2, 4, 4, CPAS);     // diamant dore vivant
            fillR(px2 + 3, py2 + 1, 2, 6, CPAS);
            fillR(px2 + 1, py2 + 3, 6, 2, CPAS);
            fillR(px2 + 3, py2 + 3, 2, 2, CWH);
            if (fr % 8 < 4) fillR(px2 + 6, py2, 2, 2, CWH);
          }
        }
      } else if (t == C_STRT) {
        fillR(px2, py2 + 2, 8, 2, CRED);
        fillR(px2, py2, 2, 4, CWH);
      } else if (t == C_GATE) {
        fillR(px2, py2, 2, 7, CBLU);
        fillR(px2 + 6, py2, 2, 7, CBLU);
      }
    }
  }
  // drapeau restaurant d'altitude
  {
    int fx = (reX - camx) * 8, fy = (reY - camy) * 8 + TOP - 8;
    if (fx > -8 && fx < W && fy < H) fillR(fx + 3, fy, 2, 4, CYE);
  }
  // remontees
  for (int k = 0; k < nLifts; k++) {
    eadk_color_t lc2 = LT[k].type == 3 ? CCLIF : (LT[k].type == 2 ? COL(120,70,160) : (LT[k].type == 1 ? COL(40,90,60) : CLIFT));
    if (npass[curch] < LT[k].unlock) lc2 = COL(90,90,100);
    int x1 = (LT[k].x1 - camx) * 8 + 4, y1 = (LT[k].y1 - camy) * 8 + 4;
    int x2 = (LT[k].x2 - camx) * 8 + 4, y2 = (LT[k].y2 - camy) * 8 + 4;
    for (int t = 0; t <= 24; t++) {
      int px3 = x1 + (x2 - x1) * t / 24, py3 = y1 + (y2 - y1) * t / 24;
      if (px3 > -6 && px3 < W && py3 > TOP - 6 && py3 < H) {
        if (LT[k].type == 0) { fillR(px3, py3, 1, 5, lc2); }        // tire-fesse : fil fin + piquets
        else fillR(px3, py3, 2, 4, lc2);
        if (t % 6 == 3) fillR(px3, py3, 2, 7, COL(96,72,50));       // pylone
      }
    }
    fillR(x1 - 2, y1 - 3, 7, 5, lc2);
    fillR(x2 - 2, y2 - 3, LT[k].type == 3 ? 9 : 7, 5, lc2);
  }
  // drapeau arrivee course
  if (curchal == 3) {
    int fx = (raceX2 - camx) * 8, fy = (raceY2 - camy) * 8 + TOP;
    fillR(fx - 4, fy, 8, 3, CRED);
    fillR(fx - 4, fy - 6, 2, 6, CWH);
  }
  // fantome
  if ((curchal == 1 || curchal == 3) && ghLen > 0) {
    int gi = imini(ghRec, ghLen) - 1;
    if (gi >= 0) {
      int sx = (int)((GHOST[gi][0] / 16.0f - camx) * 8) + 4, sy = (int)((GHOST[gi][1] / 16.0f - camy) * 8) + 4;
      if (sx >= 2 && sx < W - 2 && sy >= TOP && sy < H - 2) {
        fillR(sx - 1, sy - 3, 2, 5, COL(200,210,230));
        fillR(sx - 2, sy - 5, 4, 3, CWH);
      }
    }
  }
  // PNJ
  for (int i2 = 0; i2 < NQC; i2++) {
    Npc * n = &NC_[i2];
    int bob = (n->st == 3) ? (fr / 6 % 2) : 0;
    int sx = (int)((n->x - camx) * 8) + 4, sy = (int)((n->y - camy) * 8) + 4 - bob;
    if (sx < -4 || sx >= W + 4 || sy < TOP || sy >= H) continue;
    eadk_color_t c2 = n->kind == 0 ? COL(40,120,220) : (n->kind == 1 ? COL(200,60,160) : COL(60,180,140));
    if (n->st == 3) c2 = n->kind == 0 ? CRED : CGRN;
    fillR(sx - 1, sy - 2, 3, 3, c2);
    fillR(sx, sy + 1, 1, 2, COL(250,210,170));
    if (n->st == 3) {
      fillR(sx - 3, sy - 4 + bob, 2, 2, c2);
      fillR(sx + 2, sy - 5 + bob, 2, 2, c2);
    }
    if (n->st == 4 && fr % 16 < 8) fillR(sx + 3, sy - 5, 2, 2, CWH);
  }
  // ---- damneuses (orange, gyro clignotant) ----
  for (int g = 0; g < 2; g++) {
    if (GR[g].wait > 200) continue;                  // garée hors champ logique
    int gr2 = (int)GR[g].r; if (gr2 < 1) gr2 = 1; if (gr2 >= GH) gr2 = GH - 1;
    int sx = (pxx[gr2] - camx) * 8 + 4, sy = (gr2 - camy) * 8 + 4;
    if (sx < -8 || sx >= W + 8 || sy < TOP - 8 || sy >= H + 8) continue;
    fillR(sx - 2, sy - 2, 4, 4, COL(240,140,20));
    fillR(sx - 4, sy - 2 + (GR[g].dir < 0 ? 0 : 3), 2, 2, COL(170,175,195)); // lame
    fillR(sx + 1, sy - 3, 1, 1, fr % 30 < 15 ? CRED : COL(70,70,70));        // gyro
  }
  // ---- train de montagne : rail + gare + 2 voitures ----
  if (trnOn) {
    int ex = chx, ey = chy + 1;
    for (int t = 0; t <= 24; t++) {
      int lx = stX + (ex - stX) * t / 24, ly = stY + 2 + (ey - stY - 2) * t / 24;
      int sx = (lx - camx) * 8 + 4, sy = (ly - camy) * 8 + 4;
      if (sx >= -4 && sx < W + 4 && sy >= TOP && sy < H)
        fillR(sx - 1, sy, (t % 2) ? 2 : 1, 1, COL(80,72,62));
    }
    for (int c2 = 0; c2 < 2; c2++) {
      int tt2 = trnT100 - c2 * 6; if (tt2 < 0) tt2 = 0;
      int cx2 = stX + (ex - stX) * tt2 / 100, cy2 = stY + 2 + (ey - stY - 2) * tt2 / 100;
      int sx = (cx2 - camx) * 8 + 3, sy = (cy2 - camy) * 8 + 3;
      if (sx >= -6 && sx < W + 6 && sy >= TOP && sy < H) {
        fillR(sx - 2, sy - 2, 5, 4, CRED);
        fillR(sx - 1, sy - 1, 3, 2, COL(250,220,120));
        if (trnWait > 0 && c2 == 0) {                // fumette au quai
          int fy = sy - 4 - (trnWait / 10);
          if (fy > TOP) fillR(sx + 4, fy, 2, 2, COL(230,230,240));
        }
      }
    }
  }
  // skieur
  if (!lift) {
    float dx = fsin(hdg), dy = fcos(hdg);
    int sx = (int)((skx - camx) * 8) + 4, sy = (int)((sky - camy) * 8) + 4;
    if (crash == 0) {
      for (int tt = -1; tt <= 4; tt++)
        fillR(sx + (int)(dx * tt) - 1, sy + (int)(dy * tt) - 1, 2, 2, COL(30,30,60));
      fillR(sx - 2, sy - 4, 4, 5, OUTC[outfit]);
      fillR(sx - 1, sy - 6, 2, 2, COL(250,210,170));
    } else {
      fillR(sx - 4, sy - 1, 8, 3, COL(120,110,100));
    }
  } else {
    int k2 = lift - 1;
    int x1 = (LT[k2].x1 - camx) * 8 + 4, y1 = (LT[k2].y1 - camy) * 8 + 4;
    int x2 = (LT[k2].x2 - camx) * 8 + 4, y2 = (LT[k2].y2 - camy) * 8 + 4;
    int sx = x1 + (int)((x2 - x1) * liftp), sy = y1 + (int)((y2 - y1) * liftp);
    if (LT[k2].type == 3) {                 // telepherique : grande cabine suspendue
      fillR(sx - 4, sy - 8, 9, 7, OUTC[outfit]);
      fillR(sx - 4, sy - 8, 9, 2, COL(90,95,110));
      fillR(sx - 4, sy - 11, 9, 4, COL(190,195,210));
    } else if (LT[k2].type == 2) {          // telecabine : bulle fermée
      fillR(sx - 4, sy - 5, 7, 5, OUTC[outfit]);
      fillR(sx - 4, sy - 5, 7, 2, COL(150,155,175));
      fillR(sx - 1, sy - 8, 2, 4, COL(120,125,140));
    } else if (LT[k2].type == 1) {          // telesiege : chaise
      fillR(sx - 2, sy - 5, 5, 4, OUTC[outfit]);
      fillR(sx - 1, sy - 7, 2, 4, COL(190,195,210));
      fillR(sx - 1, sy - 8, 2, 2, COL(250,210,170));
    } else {                                // tire-fesse : juste un tirant
      fillR(sx - 1, sy - 3, 2, 4, COL(190,195,210));
      fillR(sx - 4, sy - 2, 4, 1, OUTC[outfit]);
    }
  }
}

static const char * selCountry(void) { return CO[CC[curch]].nom; }

static void drawHud(void) {
  fillR(0, 0, W, TOP, CHUD);
  fillR(0, BTM, W, H - BTM, CHUD);
  char t[24];
  if (curchal == 0) { snprintf(t, sizeof(t), "Portes %d/%d", gatesPassed(), ng); drawStr(t, 2, 1, CWH, CHUD); }
  else if (curchal == 1) { snprintf(t, sizeof(t), "TIME %ds", chachr / 13); drawStr(t, 100, 1, CWH, CHUD);
    if (bestT[curch][0]) { snprintf(t, sizeof(t), "REC %d s", bestT[curch][0] / 13); drawStr(t, 230, 1, CYE, CHUD); } }
  else if (curchal == 3) { snprintf(t, sizeof(t), "TIME %ds", chachr / 13); drawStr(t, 100, 1, CWH, CHUD);
    if (bestT[curch][1]) { snprintf(t, sizeof(t), "REC %d s", bestT[curch][1] / 13); drawStr(t, 210, 1, CYE, CHUD); } }
  else { snprintf(t, sizeof(t), "%s", MNAMES[curch]); drawStr(t, 2, 1, CWH, CHUD);
    drawStr(selCountry(), 120, 1, CYE, CHUD); }
  snprintf(t, sizeof(t), "%dkm/h P%d/%d", (int)(spd * 130), npass[curch], needM(curch));
  drawStr(t, W - CHARW * (int)strlen(t) - 2, 1, air ? CYE : CWH, CHUD);
  if (hudt > 0) drawStr(hudmsg, imaxi(2, (W - CHARW * (int)strlen(hudmsg)) / 2), BTM + 4, CYE, CHUD);
  else if (nearL) { snprintf(t, sizeof(t), "OK : %s", LNAMEB[nearL - 1]); drawStr(t, 2, BTM + 4, CGRN, CHUD); }
  else if (nearR) { snprintf(t, sizeof(t), "%s (ambiance !)", RNAMEB); drawStr(t, 2, BTM + 4, CGRN, CHUD); }
  else if (nearC) drawStr("OK : chalet (defis/remontees/vestiaire)", 2, BTM + 4, CGRN, CHUD);
  else drawStr("S:vol D:freine T:carte Tlong:vestiaire", 2, BTM + 4, COL(150,160,180), CHUD);
}

static void drawMapView(void) {
  fillR(0, 0, W, H, CHUD);
  int scale = GH > 128 ? 1 : 2;
  int mx0 = (W - GW * scale) / 2, my0 = (TOP + H - 30 - GH * scale) / 2;
  /* v3.4 : fusion des runs horizontaux (cartes doubles => 1 seul fillR par run) */
  for (int y = 0; y < GH; y++) {
    int x = 0;
    while (x < GW) {
      uint8_t t = gt(x, y);
      eadk_color_t c = CSNOW;
      if (t == C_GROOM) c = CGRN;
      else if (t == C_TREE) c = COL(60,110,70);
      else if (t == C_ROCK || t == C_CLIF) c = COL(90,85,80);
      else if (t == C_BILD) c = CRED;
      else if (t == C_KICK) c = CKICK;
      else if (t == C_RAIL) c = COL(90,95,110);
      else if (t == C_BOSS) c = COL(190,205,230);
      else if (t == C_PIPE) c = COL(120,160,220);
      else if (t == C_RAMP) c = COL(165,175,195);
      else if (HG[y * GW + x] > 205) c = COL(200,210,225);
      c = weaTint(c);
      int run = 1;
      while (x + run < GW) {
        uint8_t t2 = gt(x + run, y);
        eadk_color_t c2 = CSNOW;
        if (t2 == C_GROOM) c2 = CGRN;
        else if (t2 == C_TREE) c2 = COL(60,110,70);
        else if (t2 == C_ROCK || t2 == C_CLIF) c2 = COL(90,85,80);
        else if (t2 == C_BILD) c2 = CRED;
        else if (t2 == C_KICK) c2 = CKICK;
        else if (t2 == C_RAIL) c2 = COL(90,95,110);
        else if (t2 == C_BOSS) c2 = COL(190,205,230);
        else if (t2 == C_PIPE) c2 = COL(120,160,220);
        else if (t2 == C_RAMP) c2 = COL(165,175,195);
        else if (HG[y * GW + x + run] > 205) c2 = COL(200,210,225);
        if (weaTint(c2) != c) break;
        run++;
      }
      fillR(mx0 + x * scale, my0 + y * scale, run * scale, scale, c);
      x += run;
    }
  }
  for (int k = 0; k < nLifts; k++) {
    eadk_color_t lc2 = LT[k].type == 2 ? COL(120,70,160) : (LT[k].type == 1 ? COL(40,90,60) : CBLU);
    if (npass[curch] < LT[k].unlock) lc2 = COL(90,90,100);
    for (int t = 0; t <= 20; t++) {
      int lx = LT[k].x1 + (LT[k].x2 - LT[k].x1) * t / 20, ly = LT[k].y1 + (LT[k].y2 - LT[k].y1) * t / 20;
      fillR(mx0 + lx * scale, my0 + ly * scale, scale, scale, lc2);
    }
  }
  for (int pi = 0; pi < NPMAXI; pi++)
    if (!tpass[pi]) fillR(mx0 + PASX[pi] * scale, my0 + PASY[pi] * scale, scale, scale, CPAS);
  if (trnOn) {
    int ex = chx, ey = chy + 1;
    for (int t = 0; t <= 24; t++) {
      int lx = stX + (ex - stX) * t / 24, ly = stY + 2 + (ey - stY - 2) * t / 24;
      fillR(mx0 + lx * scale, my0 + ly * scale, scale, scale, COL(80,72,62));
    }
    int tt2 = trnT100;
    int cx2 = stX + (ex - stX) * tt2 / 100, cy2 = stY + 2 + (ey - stY - 2) * tt2 / 100;
    fillR(mx0 + cx2 * scale - 1, my0 + cy2 * scale - 1, 3 * scale, 2 * scale, CRED);
  }
  if (fr % 2 == 0) fillR(mx0 + (int)skx * scale - 1, my0 + (int)sky * scale - 1, 5, 5, CRED);
  fillR(0, 0, W, 30, CHUD);
  fillR(0, H - 24, W, 24, CHUD);
  drawStr(MNAMES[curch], 2, 1, CWH, CHUD);
  drawStr(selCountry(), 2, 14, CYE, CHUD);
  drawStr("Piste:", 2, H - 22, CGRN, CHUD);
  drawStr(PNAMEB, 56, H - 22, CWH, CHUD);
  drawStr(LNAMEB[0], 2, H - 11, CGRN, CHUD);
  drawStr(VNAMEB[0], 176, H - 11, CWH, CHUD);
  drawStr("T:retour", W - CHARW * 8 - 2, 14, CGY, CHUD);
}


static void drawTitle(void) {
  fillR(0, 0, W, H, CSKY);
  for (int dy = -8; dy <= 8; dy++) for (int dx2 = -8; dx2 <= 8; dx2++)
    if (dx2 * dx2 + dy * dy <= 64) fillR(250 + dx2, 40 + dy, 1, 1, CYE);
  for (int x = 0; x < W; x++) {
    int h1 = 150 - iabsi(x - 80) / 2 - iabsi(x - 200) / 3;
    int h2 = 180 - iabsi(x - 150) * 2 / 3;
    int h3 = 200 - iabsi(x - 60) * 3 / 4 - iabsi(x - 260) * 3 / 5;
    if (h1 < 150) h1 = 150;
    fillR(x, h1, 1, H - h1, COL(150,180,215));
    if (h2 > 0) {
      fillR(x, h2, 1, H - h2, COL(220,230,245));
      fillR(x, h2, 1, 6, CWH);
    }
    if (h3 > 0) fillR(x, h3, 1, H - h3, CWH);
  }
  drawStr("GRAND MONT", 40, 36, CWH, CHUD);
  drawStr("ADVENTURE", 40, 54, CYE, CHUD);
  drawStr("10 stations - openworld ski", 30, 86, COL(230,240,255), CHUD);
  drawStr("OK : Jouer", 40, 116, CWH, CHUD);
  drawStr("Toolbox : Aide", 40, 131, COL(200,210,230), CHUD);
  drawStr("Haut : stations  Droite : code", 20, 148, COL(200,210,230), CHUD);
  char t[24];
  snprintf(t, sizeof(t), "Debloque : %d/10", unlocked);
  drawStr(t, 40, 186, CWH, CHUD);
}

static void drawCode(void) {
  fillR(0, 0, W, H, CHUD);
  drawStr("Entrer le code de deblocage", 30, 50, CWH, CHUD);
  drawStr("(gagne en finissant une station)", 30, 65, CGY, CHUD);
  for (int i2 = 0; i2 < 4; i2++) {
    char b[2] = {(char)('0' + coded[i2]), 0};
    drawStr(b, 104 + i2 * 30, 110, i2 == ccur ? CYE : CWH, CHUD);
    if (i2 == ccur) fillR(104 + i2 * 30, 122, 7, 2, CYE);
  }
  drawStr("Fleches : choix   OK : valider", 30, 160, CGY, CHUD);
  drawStr("BACK : retour", 40, 175, CGY, CHUD);
  if (hudt > 0) drawStr(hudmsg, 30, 200, CRED, CHUD);
}

static void drawSel(void) {
  fillR(0, 0, W, H, CHUD);
  drawStr("Choisir une station :", 30, 24, CWH, CHUD);
  for (int i2 = 0; i2 < NM; i2++) {
    char t[44];
    char sub[28];
    if (i2 < unlocked) snprintf(sub, sizeof(sub), "-> %s", CO[CC[i2]].nom);
    else snprintf(sub, sizeof(sub), "X (%d pass req.)", i2 == unlocked ? needM(unlocked - 1) : 0);
    snprintf(t, sizeof(t), "%d. %s %s", i2 + 1, MNAMES[i2], sub);
    drawStr(t, 30, 44 + 15 * i2, i2 == selv ? CYE : (i2 < unlocked ? CWH : CGY), CHUD);
  }
  drawStr("OK : skier   BACK : retour", 36, 208, CGY, CHUD);
}

static const char * AP[3][10] = {
  { "GRAND MONT ADVENTURE", "",
    "Fleches : skier. La pente donne",
    "la vitesse, BAS freine.",
    "En saut : G/D = vrille, HAUT =",
    "salto avant, BAS = salto ar,",
    "OK tenu = grab. Combos = points",
    "Carving rapide mais switch =",
    "CHUTE ! Freestyle : ok", "" },
  { "Chalet (OK pres du chalet) :", "",
    "5 defis : Slalom, Chrono,",
    "Freeride chronometre, Course",
    "A->B avec FANTOME (ton record),",
    "Boarder Cross.",
    "Remontees et defis se deverrouillent",
    "au fil des PASS (0/3/6/9/12).",
    "9 PASS d'or caches hors-piste !", "" },
  { "Vestiaire : tenir ToolBox 1s", "",
    "Couleur d'outfit + materiel :",
    "Ski ou Snowboard, Carving (vite)",
    "ou Freestyle (tourne +, tricks",
    "plus faciles, switch autorise).",
    "Pistes et remontees prennent les",
    "noms locaux : Seggiovia, Gondel,",
    "Stolheis, Telesilla, Lope...", "" }
};

static void drawAide2(void) {
  fillR(0, 26, W, H - 26, CHUD);
  char t[16];
  snprintf(t, sizeof(t), "Aide %d/3", pg + 1);
  drawStr(t, 30, 30, CYE, CHUD);
  for (int i2 = 0; i2 < 10; i2++)
    if (AP[pg][i2][0]) drawStr(AP[pg][i2], 30, 50 + 14 * i2, CWH, CHUD);
  drawStr("OK : page   BACK : retour", 36, 208, CGY, CHUD);
}

static const char * CHROWS[8] = { "Slalom", "Chrono", "Freeride", "Course A-B", "Boarder X", "Remontees", "Vestiaire", "Retour" };

static void drawCha(void) {
  fillR(30, 40, 260, 160, CHUD);
  char t[44];
  snprintf(t, sizeof(t), "%s - %s", CHANAM, selCountry());  /* v3.4 : chalet nomme */
  drawStr(t, 40, 46, CYE, CHUD);
  snprintf(t, sizeof(t), "PASS %d/%d", npass[curch], needM(curch));
  drawStr(t, 40, 60, CPAS, CHUD);
  if (cham == 0) {
    for (int i2 = 0; i2 < 8; i2++) {
      char row[34];
      if (i2 < 5) {
        if (npass[curch] < EVA[i2]) snprintf(row, sizeof(row), "%s (%d pass)", CHROWS[i2], EVA[i2]);
        else snprintf(row, sizeof(row), "%s%s", CHROWS[i2], (evdone[curch] & (1 << i2)) ? " *" : "");
      } else snprintf(row, sizeof(row), "%s", CHROWS[i2]);
      drawStr(row, 50, 74 + 16 * i2, i2 == selv ? CYE : CWH, CHUD);
    }
  } else if (cham == 1) {
    for (int k = 0; k < nLifts; k++) {
      bool unl = npass[curch] >= LT[k].unlock;
      char row[44];
      snprintf(row, sizeof(row), "%s %s", LNAMEB[k], unl ? "" : "(verrouillee)");
      drawStr(row, 44, 60 + 17 * k, k == selv2 ? CGRN : (unl ? CWH : CGY), CHUD);
    }
  }
  drawStr("OK : choisir  BACK : sortir", 36, 214, CGY, CHUD);
}

static void drawVes(void) {
  fillR(0, 0, W, H, CHUD);
  drawStr("VESTIAIRE", 110, 24, CYE, CHUD);
  drawStr("Materiel (haut/bas) :", 60, 60, CWH, CHUD);
  drawStr(MODES[mode], 70, 76, CYE, CHUD);
  drawStr((mode & 1) ? "Switch autorise, tourne +" : "Rapide mais switch = CHUTE", 70, 90, CGY, CHUD);
  drawStr("droite/gauche : changer", 70, 104, CGY, CHUD);
  drawStr("Couleur d'outfit :", 60, 130, CWH, CHUD);
  fillR(200, 128, 22, 12, OUTC[outfit]);
  char t[32];
  snprintf(t, sizeof(t), "%d/8", outfit + 1);
  drawStr(t, 234, 130, CGY, CHUD);
  snprintf(t, sizeof(t), "Record saut : %d m", jrec);
  drawStr(t, 60, 160, CPAS, CHUD);
  drawStr("BACK/OK : retour au ski", 70, 196, CGY, CHUD);
}

static void drawSelColor(void) { (void)0; }

static void startEvent(int ev) {
  freeRun();
  curchal = ev;
  if (ev == 0) { sayhud("Slalom : toutes les portes !"); }
  else if (ev == 1) {
    evpar = (int)(GH * 2.6f);
    skx = (float)pxx[3]; sky = 3.0f;
    chachr = 0;
    sayhud("Chrono -> chalet !");
  } else if (ev == 2) {
    evlimit = 750;
    chachr = 0;
    skx = (float)pxx[3]; sky = 3.0f;
    sayhud("Freeride 60s : 400 pts !");
  } else if (ev == 3) {
    int k = npass[curch] >= 3 ? 1 : 0;
    raceX2 = LT[k].x1; raceY2 = LT[k].y1;
    racePar = (GH - LT[k].y2) * 3;
    skx = (float)LT[k].x2; sky = (float)((float)LT[k].y2 + 1);
    chachr = 0; hdg = 0; spd = 0.30f;
    sayhud("Course vers la station !");
  } else if (ev == 4) {
    skx = (float)bx[0]; sky = (float)((float)bcTop + 1);
    chachr = 0; hdg = 0; spd = 0.30f;
    sayhud("Boarder X : toutes les portes !");
  }
  scr = S_SKI;
}

static void drawCpl(void) {
  fillR(0, 0, W, H, CHUD);
  char t[24];
  snprintf(t, sizeof(t), "%s REUSSI !", EVN[cplType]);
  drawStr(t, 100, 60, CYE, CHUD);
  if (cplType == 1) snprintf(t, sizeof(t), "Temps : %ds / %ds", chachr / 13, evpar / 13);
  else if (cplType == 3) snprintf(t, sizeof(t), "Temps : %ds / %ds", chachr / 13, racePar / 13);
  else if (cplType == 2) snprintf(t, sizeof(t), "Points : %d / 400", score);
  else if (cplType == 4) snprintf(t, sizeof(t), "Portes : %d/%d", nbg, nbg);
  else snprintf(t, sizeof(t), "Points : %d", score);
  drawStr(t, 40, 90, CWH, CHUD);
  snprintf(t, sizeof(t), "+%d PASS  (%d/%d)", EVR[cplType], npass[curch], needM(curch));
  drawStr(t, 40, 110, CPAS, CHUD);
  snprintf(t, sizeof(t), "Record saut : %d m", jrec);
  drawStr(t, 40, 140, CWH, CHUD);
  drawStr(hudmsg, 20, 170, CWH, CHUD);
  drawStr("OK : rejouer   BACK : menu", 40, 190, CGY, CHUD);
}

int main(int argc, char ** argv) {
  (void)argc; (void)argv;
  rs = eadk_random();
  while (1) {
    ks = eadk_keyboard_scan();
    fr++;
    if (scr == S_SKI && --weaT <= 0) {
      wea = ri(3); weaT = 900 + ri(900);
      { static const char * WN[3] = { "Grand soleil !", "Coucher de soleil...", "Il neige !" };
        sayhud(WN[wea]); }
    }
    switch (scr) {
      case S_TIT:
        drawTitle();
        if (pr(eadk_key_toolbox)) { scr = S_AIDE; pg = 0; }
        if (pr(eadk_key_left) || pr(eadk_key_right)) { scr = S_CODE; ccur = 0; hudt = 0; }
        if (pr(eadk_key_up) || pr(eadk_key_down)) { scr = S_SEL; selv = 0; }
        if (pr(eadk_key_ok)) { newMountain(imini(unlocked, NM) - 1); scr = S_SKI; }
        break;
      case S_CODE: {
        drawCode();
        if (pr(eadk_key_left)) ccur = (ccur + 3) % 4;
        if (pr(eadk_key_right)) ccur = (ccur + 1) % 4;
        if (pr(eadk_key_up)) coded[ccur] = (coded[ccur] + 1) % 10;
        if (pr(eadk_key_down)) coded[ccur] = (coded[ccur] + 9) % 10;
        if (pr(eadk_key_ok)) {
          int code = coded[0] * 1000 + coded[1] * 100 + coded[2] * 10 + coded[3];
          int ok2 = 0;
          for (int l = 1; l < NM; l++)
            if (code == codeFor(l)) { unlocked = imaxi(unlocked, l + 1); ok2 = 1; }
          if (ok2) scr = S_TIT;
          else { sayhud("Code invalide"); }
        }
        if (pr(eadk_key_back)) scr = S_TIT;
        if (hudt > 0) hudt--;
        break;
      }
      case S_AIDE:
        drawAide2();
        if (pr(eadk_key_ok)) pg = (pg + 1) % 3;
        if (pr(eadk_key_back)) scr = S_TIT;
        break;
      case S_SEL:
        drawSel();
        if (pr(eadk_key_up)) selv = (selv + NM - 1) % NM;
        if (pr(eadk_key_down)) selv = (selv + 1) % NM;
        if (pr(eadk_key_ok) && selv < unlocked) { newMountain(selv); scr = S_SKI; }
        if (pr(eadk_key_back)) scr = S_TIT;
        break;
      case S_SKI: {
        phys();
        bool okpr = pr(eadk_key_ok);
        if (scr == S_SKI) {
          int camx = imaxi(0, imini(GW - 40, (int)skx - 20));
          int camy = imaxi(0, imini(GH - 26, (int)sky - 13));
          drawSlope(camx, camy);
          drawObjects(camx, camy);
          if (wea == 2)
            for (int f = 0; f < 30; f++) {
              int fxx = (f * 53 + fr * 5 + (f % 4) * 7) % W;
              int fyy = TOP + 2 + (f * 37 + fr * (2 + f % 3)) % (BTM - TOP - 20);
              fillR(fxx, fyy, 2, 1, CWH);
            }
          drawHud();
        }
        if (scr == S_SKI) {
          if (hudt > 0) hudt--;
          if (nearC && okpr) { selv = 0; cham = 0; scr = S_CHA; }
          else if (nearL && okpr && lift == 0 && (spd < 0.20f || kd(eadk_key_down))) { boardLift(nearL - 1); }
          else if (nearL && okpr) { sayhud("Ralentis pour embarquer !"); }
          if (kd(eadk_key_toolbox)) {
            vestT++;
            if (vestT == 14) { vestT = 0; scr = S_VES; }
          } else {
            if (vestT > 0 && vestT <= 10) scr = S_MAP;
            vestT = 0;
          }
          if (pr(eadk_key_back)) scr = S_TIT;
        }
        break;
      }
      case S_MAP:
        drawMapView();
        if (pr(eadk_key_toolbox) || pr(eadk_key_ok) || pr(eadk_key_back)) scr = S_SKI;
        break;
      case S_CHA:
        if (hudt > 0) hudt--;
        drawCha();
        if (cham == 0) {
          if (pr(eadk_key_up)) selv = (selv + 7) % 8;
          if (pr(eadk_key_down)) selv = (selv + 1) % 8;
          if (pr(eadk_key_ok)) {
            if (selv == 7) { curchal = -1; scr = S_SKI; }
            else if (selv == 6) { cham = 2; selv2 = 0; }
            else if (selv == 5) { cham = 1; selv2 = 0; }
            else if (npass[curch] >= EVA[selv]) startEvent(selv);
            else { snprintf(hudmsg, sizeof(hudmsg), "Verrouille : %d PASS requis !", EVA[selv]); hudt = 150; }
          }
          if (pr(eadk_key_back)) { curchal = -1; scr = S_SKI; }
        } else if (cham == 1) {
          if (pr(eadk_key_up)) selv2 = (selv2 + nLifts - 1) % nLifts;
          if (pr(eadk_key_down)) selv2 = (selv2 + 1) % nLifts;
          if (pr(eadk_key_back)) { cham = 0; selv = 5; }
          if (pr(eadk_key_ok)) {
            if (npass[curch] >= LT[selv2].unlock) {
              scr = S_SKI; cham = 0;
              boardLift(selv2);
            } else {
              snprintf(hudmsg, sizeof(hudmsg), "Verrouillee : %d PASS requis", LT[selv2].unlock);
              hudt = 150;
            }
          }
        } else {
          drawVes();
          drawSelColor();
          if (pr(eadk_key_up) || pr(eadk_key_down)) mode = (mode + 1) % 4;
          if (pr(eadk_key_right)) outfit = (outfit + 1) % 8;
          if (pr(eadk_key_left)) outfit = (outfit + 7) % 8;
          if (pr(eadk_key_back) || pr(eadk_key_ok)) { cham = 0; scr = S_SKI; }
        }
        break;
      case S_VES:
        drawVes();
        if (pr(eadk_key_up) || pr(eadk_key_down)) mode = (mode + 1) % 4;
        if (pr(eadk_key_right)) outfit = (outfit + 1) % 8;
        if (pr(eadk_key_left)) outfit = (outfit + 7) % 8;
        if (pr(eadk_key_back) || pr(eadk_key_ok)) scr = S_SKI;
        break;
      case S_CPL:
        drawCpl();
        if (pr(eadk_key_ok)) { completeChallenge(cplType); freeRun(); curchal = -1; scr = S_SKI; }
        if (pr(eadk_key_back)) { curchal = -1; scr = S_TIT; }
        break;
    }
    if (scr == S_SKI || scr == S_MAP) { npcTick(); scync(); }
    eadk_timing_msleep(80);
  }
  return 0;
}
