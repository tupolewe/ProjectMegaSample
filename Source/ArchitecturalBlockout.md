# Architectural Blockout — pierwsza wersja

Natywny aktor do tworzenia prostych i zakrzywionych ścian oraz podłóg w edytorze.
Źródłem geometrii jest edytowalny spline, a nie gotowy model. Wymiary są w cm.

## Uruchomienie

Po pobraniu kodu zamknij Unreal i uruchom `Tools/BuildEditor.ps1` z katalogu projektu.
Otwórz projekt, w Content Browser włącz **Show C++ Classes**, przejdź do
**C++ Classes → ProjectMegaSample** i przeciągnij **ArchitecturalBlockout** do poziomu.
Alternatywnie wyszukaj **Architectural Blockout** w panelu Place Actors.

## Praca w edytorze

1. Domyślnie powstaje pokój o obrysie 600 × 500 cm, ścianach wysokich na 300 cm
   i grubych na 20 cm. Spline biegnie środkiem ściany, więc wnętrze jest mniejsze od obrysu.
2. Wybierz komponent **Outline** aktora, a potem jego punkt w viewporcie.
   Przesuwaj punkty standardowymi uchwytami Unreal. Przyciąganie do siatki jest standardowym
   przyciąganiem edytora. Dodawaj/usuwaj punkty z menu kontekstowego spline'a.
3. Typ punktu **Linear** daje proste odcinki. **Curve** / **Curve Custom Tangent** pozwala
   kształtować łuki uchwytami stycznych. Typ dotyczy odcinka wychodzącego z danego punktu.
4. W Details aktora, w kategorii **Blockout**, zmieniaj **Wall Height**, **Wall Thickness**,
   materiały i **Texture Tile Size**. **Curve Sample Spacing** steruje gęstością podziału łuków.
5. Wyłącz **Closed Room**, aby otrzymać otwarty ciąg ścian z zamkniętymi końcami.
   Podłoga jest generowana wyłącznie dla zamkniętego obrysu.
6. **Create Floor** dodaje podłogę z górną powierzchnią na lokalnym Z=0 i grubością w dół.
   Ściany rosną od Z=0 w górę. Aktora można przesunąć/obrócić jako całość.
7. Edycja przebudowuje geometrię automatycznie. Przycisk **Rebuild Geometry** wymusza
   przebudowę. Standardowe edycje punktów i właściwości można cofać przez Ctrl+Z.
8. **Build Status** opisuje wynik. Błędny obrys usuwa poprzednią geometrię, aby podgląd
   nie przedstawiał nieaktualnej konstrukcji. **Generate Collision** włącza kolizję trójkątową.

## Zakres i ograniczenia

- To pierwsza wersja do blockoutu. Nie zawiera jeszcze drzwi, okien, sufitu, schodów,
  edycji wielu kondygnacji ani eksportu do Static Mesh. Te funkcje nie są ukrytymi opcjami.
- Obrys jest płaski: wysokość punktów spline'a jest rzutowana na lokalną płaszczyznę XY.
- Obsługiwane są proste, wklęsłe i zakrzywione obrysy bez samoprzecięć oraz bez dziur.
  Bardzo ciasne łuki, zawracające odcinki i zbyt grube ściany są odrzucane.
- Geometria zakrzywiona jest przybliżana odcinkami; narożniki mają połączenia na skos.
  Limit wynosi 1024 próbki obrysu. Mniejszy odstęp próbek oznacza większy koszt przebudowy.
- Dla przewidywalnych wymiarów i UV utrzymuj skalę aktora 1,1,1; używaj parametrów i punktów.
- Podłoga dochodzi do osi ściany. Oddzielne aktory nie są automatycznie scalane.
- Aktor działa też w grze i nie wymaga ticka. Kolizja jest przeznaczona dla statycznej
  architektury, nie do symulacji fizyki samego budynku.
- Zmiana właściwości przez Blueprint w czasie gry wymaga wywołania **Rebuild Geometry**.

Implementacja znajduje się w module Runtime, ponieważ utworzona geometria i kolizja mają
pozostawać dostępne w grze. Edycja spline'a i Details korzysta z narzędzi edytora Unreal.
Osobny moduł Editor będzie właściwym miejscem dla przyszłego dedykowanego panelu i eksportu.
