# Traversal Component

Komponent C++ dla `Character`, do wejścia na przeszkodę (Mantle) lub przeskoku (Vault).
Nie zastępuje automatycznie makra `GrappleHook` i nie obsługuje linki ani staminy.

## Blueprint

1. Po kompilacji dodaj **Traversal** przez Add Component w Blueprintcie postaci.
2. Ustaw zasięg, wysokości i opcjonalne montaże `Mantle Montage` / `Vault Montage`.
3. Wywołaj `Try Traverse`. Wynik false oznacza, że akcja nie zaczęła się;
   odczytaj `Last Failure Reason` i wykonaj zakończenie grapplingu po swojej stronie.
4. `On Traversal Started` informuje o zaakceptowanej akcji i jej celu.
5. `On Traversal Finished` informuje o sukcesie, anulowaniu, blokadzie ruchu lub
   przerwaniu montażu. W tym miejscu zwalniaj własne blokady celowania/strzelania.
6. `Cancel Traversal` wywołuj przy śmierci, otrzymaniu stuna lub zmianie akcji.
   Ponowne anulowanie nic nie robi. `EndPlay` sprząta bez wysyłania zdarzenia.

`Find Traversal Target` pozwala sprawdzić możliwość wykonania ruchu bez jego uruchamiania.
Nie wywołuj detekcji co klatkę bez potrzeby: wykonuje również testy wolnego miejsca kapsuły.

## Podłączenie do grapplingu

Przed `Try Traverse` zatrzymaj przyciąganie (`isGrappling=false`) i ukryj linkę.
Nie uruchamiaj równolegle starego makra. Zachowaj referencję do punktu grapplingu,
aby przywrócić jego kolizję zarówno po sukcesie, jak i po błędzie.

Nie podłączaj starego `ReleaseGrapplingEvent` wprost: ten event dodatkowo teleportuje
postać do `GrapplingFinalLocation`. Nowy komponent sam ustala i osiąga miejsce lądowania.
Przenieś potrzebne przywracanie flag z tego eventu do osobnego cleanupu, bez teleportacji.
Komponent nie zna pól `canMove`, `canShoot`, `canInterrupt`, `bCanAim`, `canGrapple` z BP.
Blueprint musi zwolnić własne blokady również wtedy, gdy `Try Traverse` zwróci false.

## Zasady działania

- Detekcja ściany i podłoża używa `Trace Channel` (domyślnie Visibility).
  Trasa i miejsce lądowania są sprawdzane pełną kapsułą i jej odpowiedziami kolizji.
- Ruch ma trzy odcinki: w górę, nad przeszkodą, w dół. Na każdym działa sweep;
  nowa przeszkoda przerywa akcję. Kolizja kapsuły nigdy nie jest wyłączana.
- Niska przeszkoda z dostępnym lądowaniem za nią daje Vault; w pozostałych przypadkach
  komponent próbuje Mantle na górną powierzchnię. Nie obraca postaci do ściany.
- Na czas ruchu włącza Flying i dodaje własne blokady inputu do kontrolera.
  Kończąc zwalnia wyłącznie własne blokady. Flying przechodzi w Falling, żeby silnik
  znalazł rzeczywistą podłogę. Nie przywraca Flying pozostawionego przez grapple.
- Czas ruchu bez animacji to `Duration`. Z montażem jest to czas zwrócony przez
  `Montage_Play`. Komponent ignoruje root motion na czas własnego montażu, a następnie
  przywraca poprzedni tryb. Montaż musi mieć odpowiedni Slot w AnimBlueprintcie.
- Aktywny inny montaż blokuje start. Przerwanie własnego montażu kończy akcję;
  zakończenie ruchu zatrzymuje pozostałą część własnego montażu.

## Ograniczenia pierwszej wersji

Standalone, pionowa kapsuła i standardowa grawitacja w osi Z. Brak sieciowej predykcji,
Motion Warping, dopasowania rąk IK i śledzenia poruszających się platform. Cele są
zapamiętywane w przestrzeni świata. To baza mechaniki, wymagająca dopasowania animacji
i parametrów do konkretnej postaci. Nie zmienia Blueprintów ani map projektu.
