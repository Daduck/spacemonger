
#include "stdafx.h"
#include "SpaceMonger.h"
#include "lang.h"

static const char *densitynames[] = {
	"Petit Peu de Fichiers", "Peu de Fichiers", "Normal", "Beaucoup de Fichiers",
	"Très Beaucoup de Fichiers", "Trop de Fichiers",
};

static const char *uscolornames[] = {
	"Arc-en-Ceil", "Colours de Windows", "Blanc", "Gris Clair", "Gris Foncé",
	"Rouge", "Orange", "Jaune", "Vert", "Aqua", "Bleu", "Violette",
};

static const char *monthnames[] = {
	"Jan", "Fev", "Mars", "Avril", "Mai", "Juin",
	"Juil", "Août", "Sep", "Oct", "Nov", "Dec",
};

static const char *attribnames[] = {
	"Arch", "Compress", "Dossier", "Encrypt", "Caché", "Pas-au-ligne",
	"Lire-Seulement", "Reparse-Pt", "Clairsemé", "Système", "Temp",
};

Lang LangFr = {
	IDR_TOOLBAR_FR,
	"De SpaceMonger",
	"CE PROGRAMME EST FREEWARE.",
	"«You may distribute this program freely.  Because this program is"
		" free, the author cannot and will not assume responsibility"
		" for this program's behavior.  There are no warranties of any"
		" kind, expressed or implied, on this product.»",
	"Une continuation indépendante du SpaceMonger classique.",
	"Mainteneur actuel : Johnny J. Andersen (Anderki.dk)",
	"Site &Web du projet",
	"Signaler un &bogue",
	"SpaceMonger n'a pas pu ouvrir cette adresse :\n%s",
	"D'accord", "Annul",
	"octets", "Ko", "Mo", "Go",
	"<l'Espace Libre: %d,%d%%>",
	"&Vrombir", "&Démarrer", "Voir &Tout", "&Exécuter", "&Effacer",
	"&Ouvrir un Disque...", "&Rouvrir", "&Montrer l'Espace Libre",
	"%lu,%01lu%%", "%lu,%01lu %s",
	"Total", "Libre", "Fichiers Totals:", "Dossiers Totals:", "Fichiers Trouvés:", "Dossiers Trouvés:",
	',', '.',
	densitynames, uscolornames, monthnames, attribnames,
	"Effacer...",
	"Choisir un Disque",
	"Scruter le Disque...",
	"Configuration de SpaceMonger",
	"Orientation des Fichiers", "Densité:", "Orient:", "Horz", "Égale", "Vert",
	"Colours", "Fichiers", "Dossiers",
	"«ToolTips»", "Montrer Nom «ToolTips»", "Montrer Info «ToolTips»", "Délai:", "msec",
	"Sentier", "Nom-fichier", "Petit Icon", "Icon", "Date/Heure", "Octets", "Attributs",
	"Autres Options",
	"Auto-Rouvrir à Effacer", "Pas de Command «Effacer»",
	"Vrombir Animé", "Souvenir Position du Window",
	"Montrer les Boites «Rollovers»",
	"Properties...",
	"L'analyse n'a pas pu aboutir (erreur Windows %lu).",
	"Analyse incomplète : %llu dossiers n'ont pas pu être entièrement analysés (première erreur Windows %lu). Les tailles peuvent être sous-estimées.",
};
