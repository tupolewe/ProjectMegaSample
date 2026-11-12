# Expanding Wave Component

Komponent wykrywa pionowe kapsuly `ACharacter` w rozszerzajacym sie poziomym
pierscieniu. Nie zadaje obrazen, nie wybiera druzyn i nie tworzy VFX.

## Podlaczenie w Blueprint

1. Po kompilacji otworz edytor i dodaj do postaci `Expanding Wave Component`.
2. Zaznacz komponent i dodaj event `On Character Detected`.
   Parametr `Character` to wykryta postac. Tu sprawdz druzyne/interfejs
   i wywolaj swoja logike obrazen. Wlasciciel komponentu jest pomijany.
3. W momencie Anim Notify wywolaj `Start Wave` na komponencie.
   `Ground Origin` musi byc punktem na ziemi w przestrzeni swiata.
   Dla stojacej postaci mozna uzyc pozycji kapsuly minus jej scaled half height
   w osi Z; przy nierownym podlozu lepszy jest punkt trafienia trace'a w podloze.
   Nie podawaj srodka postaci: fala znajdzie sie wtedy za wysoko.
4. `Start Wave` zwraca false, jesli fala juz dziala lub parametry sa niepoprawne.
   Pierwsze wykrywanie nastepuje w kolejnym ticku. Ustawienia sa kopiowane
   przy starcie; zmiany konfiguracji dotycza kolejnej fali.
5. `Stop Wave` przerywa wykrywanie bez eventu zakonczenia.
   `On Wave Finished` oznacza naturalne osiagniecie maksymalnego promienia.
   Przy ogluszeniu/przerwaniu animacji wywolaj Stop Wave, jesli fala ma zniknac.

Poczatkowe ustawienia (cm): Max Radius 1200, Expansion Speed 600 cm/s,
Ring Width 60, Wave Height 40. Czas rozszerzania: 2 sekundy.
`Get Current Radius` i `Get Wave Origin` pozwalaja synchronizowac wlasne VFX.

## Geometria i filtrowanie

- Zewnetrzna krawedz zaczyna od promienia 0; pierscien ma szerokosc Ring Width
  do wewnatrz. Konczy sie, gdy zewnetrzna krawedz osiagnie Max Radius.
- Pas pionowy: GroundOrigin.Z ... GroundOrigin.Z + WaveHeight.
  Kapsula calkowicie ponad pasem nie jest wykrywana. Liczy sie kolizja kapsuly,
  a nie animowana pozycja stop. Jeden aktor jest zglaszany tylko raz na fale.
- Obiekty sa wstepnie wyszukiwane przez collision query; kapsula musi miec
  wlaczone Query Collision i typ obiektu znajdujacy sie w Target Object Channels
  (domyslnie Pawn). Generate Overlap Events nie jest wymagane.
- Wykrywane sa tylko ACharacter i ich glowne pionowe kapsuly; dodatkowe meshe,
  bronie i ciala ragdoll nie powoduja dodatkowych trafien.
- Fala obejmuje caly dystans radialny przebyty od poprzedniej klatki.
  Polozenie/skok celu sa probkowane raz na tick: przy bardzo niskim FPS wynik
  blisko momentu przeskoku moze byc przyblizony. Nie jest to ciagle CCD celu.
- Fala pozostaje na stalej wysokosci: nie podaza po schodach ani zboczach
  i przenika przez sciany. To detektor pierscienia, nie symulacja podloza.
- Brak replikacji. W multiplayer uruchamiaj wykrywanie/obrazenia na serwerze;
  synchronizacje efektow wizualnych wykonaj osobno.

## Test w edytorze

Wlacz Draw Debug. Ustaw kilka postaci w odleglosci 300, 600 i 900 cm.
W evencie wypisz nazwe Character i czas. Powinny zostac wykryte kolejno,
po jednym razie. Przeskocz pierscien: kapsula ponad gorna linia debug nie
powinna generowac eventu. Sprawdz tez postac poza Max Radius, druga fale
(ponowne wykrywanie), Stop Wave (brak dalszych eventow) oraz wywolanie
Stop Wave/Start Wave wewnatrz eventu wykrycia. Sprawdz zachowanie przy 30 FPS.
