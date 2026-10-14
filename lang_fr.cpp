
#include "stdafx.h"
#include "SpaceMonger.h"
#include "lang.h"

static const wchar_t *densitynames[] = {
	L"Petit Peu de Fichiers", L"Peu de Fichiers", L"Normal", L"Beaucoup de Fichiers",
	L"Très Beaucoup de Fichiers", L"Trop de Fichiers",
};

static const wchar_t *uscolornames[] = {
	L"Arc-en-Ceil", L"Colours de Windows", L"Blanc", L"Gris Clair", L"Gris Foncé",
	L"Rouge", L"Orange", L"Jaune", L"Vert", L"Aqua", L"Bleu", L"Violette",
};

static const wchar_t *monthnames[] = {
	L"Jan", L"Fev", L"Mars", L"Avril", L"Mai", L"Juin",
	L"Juil", L"Août", L"Sep", L"Oct", L"Nov", L"Dec",
};

static const wchar_t *attribnames[] = {
	L"Arch", L"Compress", L"Dossier", L"Encrypt", L"Caché", L"Pas-au-ligne",
	L"Lire-Seulement", L"Reparse-Pt", L"Clairsemé", L"Système", L"Temp",
};

Lang LangFr = {
	IDR_TOOLBAR_FR,
	L"De SpaceMonger",
	L"CE PROGRAMME EST FREEWARE.",
	L"«You may distribute this program freely.  Because this program is"
		L" free, the author cannot and will not assume responsibility"
		L" for this program's behavior.  There are no warranties of any"
		L" kind, expressed or implied, on this product.»",
	L"Une continuation indépendante du SpaceMonger classique.",
	L"Mainteneur actuel : Johnny J. Andersen (Anderki.dk)",
	L"Site &Web du projet",
	L"Signaler un &bogue",
	L"SpaceMonger n'a pas pu ouvrir cette adresse :\n%s",
	L"D'accord", L"Annul",
	L"octets", L"Ko", L"Mo", L"Go",
	L"<l'Espace Libre: %d,%d%%>",
	L"&Vrombir", L"&Démarrer", L"Voir &Tout", L"&Exécuter", L"&Effacer",
	L"&Ouvrir un Disque...", L"&Rouvrir", L"&Montrer l'Espace Libre",
	L"%lu,%01lu%%", L"%lu,%01lu %s",
	L"Total", L"Libre", L"Fichiers Totals:", L"Dossiers Totals:", L"Fichiers Trouvés:", L"Dossiers Trouvés:",
	',', '.',
	densitynames, uscolornames, monthnames, attribnames,
	L"Effacer...",
	L"Choisir un Disque",
	L"Scruter le Disque...",
	L"Configuration de SpaceMonger",
	L"Orientation des Fichiers", L"Densité:", L"Orient:", L"Horz", L"Égale", L"Vert",
	L"Colours", L"Fichiers", L"Dossiers",
	L"«ToolTips»", L"Montrer Nom «ToolTips»", L"Montrer Info «ToolTips»", L"Délai:", L"msec",
	L"Sentier", L"Nom-fichier", L"Petit Icon", L"Icon", L"Date/Heure", L"Octets", L"Attributs",
	L"Autres Options",
	L"Auto-Rouvrir à Effacer", L"Pas de Command «Effacer»",
	L"Vrombir Animé", L"Souvenir Position du Window",
	L"Montrer les Boites «Rollovers»",
	L"Properties...",
	L"L'analyse n'a pas pu aboutir (erreur Windows %lu).",
	L"Analyse incomplète : %llu dossiers n'ont pas pu être entièrement analysés (première erreur Windows %lu). Les tailles peuvent être sous-estimées.",
	L"Analyse terminée avec %llu dossiers ignorés. Les tailles peuvent être sous-estimées.",
	L"Ignorés : %llu",
	L"Détails de l'analyse",
	L"%llu dossiers ont été ignorés ; %llu chemins sont affichés.",
	L"Copier les chemins",
	L"Indisponible (estimation)",
};
