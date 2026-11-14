# Line Area Attack Component

Komponent tworzy serie kolistych, jednorazowych uderzen na stalej wysokosci.
Nie tworzy VFX ani nie zadaje obrazen: wysyla eventy do Blueprinta.

## Podlaczenie

1. Dodaj `Line Area Attack Component` do Blueprinta przeciwnika.
2. Podepnij eventy komponentu przed uruchomieniem ataku:
   - `On Area Warning`: pokaz zapowiedz na `Area.GroundCenter`.
     Promien: `Area.Radius`, czas zapowiedzi: `Area.WarningDuration`.
     Zapisz efekt pod `Area.Index`, aby usunac go przy aktywacji/anulowaniu.
   - `On Area Activated`: usun zapowiedz dla tego indeksu, uruchom efekt uderzenia.
   - `On Character Detected`: sprawdz druzyne/interfejs wykrytego `Character`
     i zadaj obrazenia. `Area` identyfikuje obszar trafienia.
   - `On Attack Ended`: wyczysc pozostale zapowiedzi. `bCancelled` odroznia
     anulowanie od naturalnego zakonczenia.
3. W momencie rozpoczecia ataku wywolaj `Start Attack`:
   - `Ground Origin`: punkt na ziemi przy przeciwniku (np. wynik trace'a w dol).
   - `Target Location`: pozycja gracza pobrana w tej chwili.
   Komponent zapamietuje kierunek XY; pozniejszy ruch gracza go nie zmienia.
4. `Stop Attack` anuluje pozostale uderzenia. Wywolaj go np. przy ogluszeniu,
   jezeli atak ma byc przerywalny. Przy niszczeniu postaci usun jej VFX w EndPlay;
   komponent nie wysyla zdarzen rozgrywki podczas niszczenia swiata.

`Start Attack` zwraca false, gdy atak juz trwa, kierunek XY jest zerowy lub
konfiguracja jest niepoprawna. Eventy zaczynaja sie od nastepnego ticka.
Zmiany ustawien podczas ataku dotycza kolejnego uruchomienia.

## Rozstaw i czas

Domyslnie: 6 obszarow, srednica 200 cm, wysokosc 150 cm,
Step Delay 0.3 s, Warning Duration 0.6 s, Start Offset 0 cm.

Pierwszy srodek lezy 100 cm przed punktem startu. Kolejne srodki:
300, 500, 700, 900, 1100 cm. Krawedzie sasiednich kol stykaja sie.
Zasieg wynosi StartOffset + AreaCount * AreaDiameter; liczba obszarow
nie dopasowuje sie automatycznie do odleglosci gracza.

Zapowiedzi: 0, 0.3, 0.6, 0.9, 1.2, 1.5 s.
Uderzenia: 0.6, 0.9, 1.2, 1.5, 1.8, 2.1 s (z dokladnoscia do klatki).
Kazdy obszar sprawdza cele JEDEN RAZ w momencie aktywacji. Nie pozostaje
aktywnym polem obrazen. Sprawdzana jest aktualna pozycja celu, wiec odejscie
z zapowiedzi przed aktywacja pozwala uniknac trafienia.

Domyslnie postac zglaszana jest raz na caly atak. Wylacz
`Detect Each Character Once Per Attack`, aby dopuscic jedno trafienie na kazdy
obszar. Wlasciciel jest zawsze pomijany, sojusznikow filtruje Blueprint.

## Ograniczenia i test

- Detekcja obejmuje ACharacter z pionowa glowna kapsula z Query Collision.
  Domyslny Object Type to Pawn; mozna ustawic wlasne Target Object Channels.
- Pas pionowy rozciaga sie od GroundCenter.Z do GroundCenter.Z + AreaHeight.
  Kapsula ponad pasem unika trafienia. Wysokosc 150 cm sprzyja unikowi w bok;
  zmniejsz ja, jesli atak powinien latwo dawac sie przeskoczyc.
- Obszary nie podazaja po zboczach i nie sa zatrzymywane przez sciany.
- Brak replikacji. W multiplayer uruchamiaj detekcje na serwerze i osobno
  synchronizuj wizualizacje.
- Przy zacieciu klatki zalegle kroki wykonuja sie w kolejnosci w tej samej klatce;
  detekcja korzysta z aktualnego polozenia celu, nie z historycznych pozycji.

Wlacz Draw Debug: zolty oznacza zapowiedz, czerwony aktywacje.
Sprawdz odejscie gracza w bok po rozpoczeciu ataku (linia nie skreca), kolejnosc
uderzen, przebywanie na styku kol (brak podwojnego trafienia przy domyslnej
opcji), anulowanie w trakcie zapowiedzi oraz ponowne uruchomienie ataku.
