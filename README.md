# Grand Mont Adventure

Openworld de ski en 10 stations inspirees de vrais domaines, pour NumWorks
(app externe .nwa, Epsilon 16+). Fleches pour skier, TOOLBOX pour la carte,
toutes les figures se combinent en l'air.

**10 stations** : Val Thorens, Chamonix, Zermatt, Grandvalira, Cortina,
St Anton, Verbier, Hemsedal, Garmisch, Sierra Nevada.

## Installation

1. Branche la calculatrice en USB et va sur my.numworks.com/apps.
2. Glisse le fichier `app.nwa` (bouton "Installer une application externe").
3. L'icone **Grand Mont Adventure** apparait dans la liste des apps.

## Commandes

| Touche | Action |
|---|---|
| Haut / Bas / Gauche / Droite | Tourner, accelerer, freiner |
| OK | Prendre une remontee (pres d'une station) / valider les menus |
| BACK | Retour / titre |
| TOOLBOX (tap) | Carte du domaine |
| TOOLBOX (tenu ~1 s) | **Vestiaire** : couleur d'outfit + ski/snowboard carving/freestyle |

### Vestiaire
Haut/Bas change l'equipement : 0 Ski carving, 1 Ski freestyle,
2 Snowboard carving, 3 Snowboard freestyle. Gauche/Droite change la couleur
de l'outfit.

- **Carving** : vitesse maximum. Switch a l'envers (4 vrilles en l'air en
  carving) : CHUTE !
- **Freestyle** : tourne plus fort, tricks plus faciles, switch autorise.

### Figures (en l'air, apres un kicker ou une falaise)
- Gauche / Droite : vrille a gauche/droite (combinable plusieurs fois)
- Haut : salto avant — Bas : salto arriere
- OK (tenu) : grab
- Tout combine = COMBO (multiplicateur de points).
- **Attention** : en carving, 4 vrilles ou plus = switch → chute !

## Domaine
Chaque montagne a 5 remontees (teleski, telesiege, telecabine, telepherique),
2 a 3 villages, un restaurant d'altitude (ses skieurs **dansent**), un snowpark
(rails + kickers), un boarder cross, et un chalet-refuge en haut de la piste
principale (tous les defis s'y lancent). 9 **pass dores** sont caches hors-piste
sur chaque montagne. Les PNJ font la queue aux remontees, skient sur les
pistes, dansent au restaurant et se baladent dans la station.

**Animations vivantes** : sur les monts pairs, un petit **train de montagne**
rouge fait la navette entre la gare de vallee et le chalet (rail visible, fumee
au quai) ; des **dameuses** orange patrouillent la piste principale en la
dammant, gyrophares clignotants.

Les noms des pistes et remontees changent selon le pays (Piste / Pista /
Abfahrt / Lope, Telesiege / Sessellift / Seggiovia / Stolheis...).

## Progression
- Il faut **12 a 15 pass** pour debloquer la montagne suivante.
- Pass gagnes : 9 caches hors-piste + recompenses des epreuves (1 a 3 pass
  la premiere fois).
- Gagner des pass debloque **au fil de l'eau** de nouvelles remontees
  (seuils 0/3/6/9/12 pass) et de nouvelles epreuves sur la meme montagne.
- Code de deblocage a 4 chiffres affiche quand une station est ouverte.

### Epreuves (au chalet, touche OK)
1. **Slalom** — passer toutes les portes de la piste principale.
2. **Chrono** — sommet → chalet : le **trace colore** s'affiche, ton record
   et le **fantome** de ton meilleur temps sont affiches.
3. **Freeride** — marquer 400 points en temps limite.
4. **Course A-B** — d'un sommet annexe a un village, contre la montre (ghost aussi).
5. **Boarder X** — toutes les portes du couloir + kickers.

Garde un oeil sur le HUD : km/h, PASS x/y, TIME/RECORD au centre.

## Compilation (pour les curieux)
Le dossier `gma-src/` contient le code C EADK (main.c, icone 55x56, Makefile).
`make build` produit `output/app.nwa` (nwlink + arm-none-eabi-gcc requis).

Bonne glisse !
