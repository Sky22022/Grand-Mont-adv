# Grand Mont Adventure

Openworld de ski en 10 stations inspirees de vrais domaines — jouable
**sur PC (HTML5, la meilleure qualite, avec le son)** et sur **NumWorks**
(app externe .nwa, Epsilon 16+).

**10 stations** : Val Thorens, Chamonix, Zermatt, Grandvalira, Cortina,
St Anton, Verbier, Hemsedal, Garmisch, **Sierra Nevada**.

## Version PC (recommandee)

Un seul fichier : `pc/index.html`. **Double-clic dessus** (aucune
installation, tout est dans le fichier, son inclu — 100% WebAudio synthetise).

| Touche | Action |
|---|---|
| Fleches ou ZQSD | Tourner / accelerer / freiner |
| **Alterner gauche-droite** | **Pas patineur** : pousser sur le plat (comme en patin a glace) |
| Entree ou Espace | Remontee / valider (maintenu en l'air = grab) |
| Echap | Retour |
| M (ou tap Tab) | Carte du domaine (pass dores visibles) |
| Tab (tenu ~1 s) | **Vestiaire** : outfit + materiel |
| 1, 2, 3, 4 | Changer directement de materiel (Ski/Snowboard x Carving/Freestyle) |

**Son** : vent qui grossit avec la vitesse, glisse sur neige dammee ou
poudreuse, frictions sur les rails, whoosh des sauts, atterrissages, chocs,
musique festive autour du refuge et des chalets, ronflement des dameuses,
bourdonnement des remontees.

**Qualite PC** : cartes deux fois plus grandes par dimension (4x la surface
des cartes calculette, deja x2 l'aire vs les versions precedentes), terrain
pre-rendu avec ombrage de relief, forets denses, remontees animees avec
cabines, meteo (couchers de soleil ranges, chutes de neige), fantome de ton
record, sauvegarde automatique (localStorage).

## Version calculatrice

### Installation
1. Branche la calculatrice en USB et va sur my.numworks.com/apps.
2. Glisse le fichier `app.nwa` (bouton "Installer une application externe").
3. L'icone **Grand Mont Adventure** apparait dans la liste des apps.

### Commandes

| Touche | Action |
|---|---|
| Haut / Bas / Gauche / Droite | Tourner, accelerer, freiner |
| (alterner Gauche/Droite) | **Pas patineur** : pousser sur le plat |
| OK | Prendre une remontee (freine + OK pour embarquer) / valider |
| BACK | Retour / titre |
| TOOLBOX (tap) | Carte du domaine |
| TOOLBOX (tenu ~1 s) | **Vestiaire** : couleur d'outfit + ski/snowboard carving/freestyle |

### Vestiaire
Ski carving (rapide, switch = CHUTE), ski freestyle (tourne plus, switch OK,
tricks x1.2), snowboard carving / freestyle. Gauche/Droite : couleur d'outfit.

### Figures
En l'air : Gauche/Droite = vrille G/D (combinable), Haut = salto avant,
Bas = salto arriere, OK tenu = grab. Tout enchainer = COMBO x2, x3...
En carving, 4 vrilles ou plus = switch -> chute !

## Domaine (v3.4)

- **Cartes doubles** : surface x2 par rapport aux versions precedentes
  (92x152 pour Val Thorens, jusqu'a 240x176 pour Sierra Nevada).
- **5 a 8 remontees** par montagne, types logiques selon le denivele :
  teleski/telesiege pour les petites montees, telecabine pour les grandes,
  **telepherique** pour les tres grandes & pentues (durees 230/170/130/85).
- **2-3 villages + restaurant d'altitude + chalet-defis**, avec **noms
  localises selon le pays** : restaurant/chalets/fetes (Refuge/Berghaus/
  Xalet/Rifugio Alpino/Hutte/Fjellstue/Berghuette/Refugio), pistes
  (Piste/Pista/Abfahrt/Loype), remontees (Telesiege/Sessellift/Seggiovia/
  Stolheis, Gondel, Seilbahn, Taubane, Telesilla...).
- **9 pass dores** caches hors-piste : le diamant **scintille** dans le jeu ;
  message style GMA : « Bravo ! Ski pass ! Plus que X pour la montagne
  suivante » (et « ...et une remontee ! » au seuil suivant). Visibles sur la
  carte MAP.
- **Snowparks evolutifs** : plus de montagne = rails, kickers, bosses,
  rampes, serpentin half-pipe (des la montagne 5), boarder cross.
- **PNJ avec IA** : file d'attente aux remontees, skieurs sur les pistes,
  danseurs au restaurant d'altitude, baladeurs qui sifflent au village.
- **Dameuses IA** (orange, gyros clignotants) qui damment la piste
  principale ; **train de montagne 2 wagons + gare + fumees** sur les monts
  pairs. Cliffs/crevasses naturels conserves.
- **Meteo dynamique** : grand soleil, couchers de soleil oranges, neige
  (palette grise + flocons).

## Progression
- **12 a 15 pass** requis pour la montagne suivante.
- Pass = caches hors-piste + victoires aux epreuves (1 a 3 pass la 1re fois).
- Nouvelles remontees (seuils 0/3/6/9/12/14/16/19 pass) et nouvelles epreuves
  debloquees sur la montagne elle-meme au fil de l'eau.
- Code a 4 chiffres affiche quand tu debloques une station (menu fleches
  gauche/droite au titre).

### Epreuves (au chalet, OK)
1. **Slalom** — toutes les portes de la piste principale.
2. **Chrono** — sommet -> chalet : trace colore, record + **fantome**.
3. **Freeride** — 400 points en temps limite.
4. **Course A-B** — d'un sommet annexe a la station, contre le fantome.
5. **Boarder X** — toutes les portes du couloir + kickers.

Garde un oeil sur le HUD : km/h, PASS x/y, TIME/RECORD.

## Compilation (pour les curieux)
- `gma-src/` : code C EADK (main.c, icone 55x56, Makefile). `make build`
  produit `output/app.nwa` (nwlink + arm-none-eabi-gcc requis).
- Tests : hote natif 18 assertions TOUT OK ; version PC testee en Node
  (19 assertions TOUT OK : generation des 10 stations, physique,
  remontees, passes, meteo, train).

Bonne glisse !
