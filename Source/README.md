# C++ w ProjectMegaSample

Moduł `ProjectMegaSample` jest głównym modułem Runtime. Cele:

- `ProjectMegaSampleEditor`: edytor.
- `ProjectMegaSample`: gra.

Ustawienia odpowiadają lokalnemu Unreal Engine 5.8: `BuildSettingsVersion.V7`
oraz `EngineIncludeOrderVersion.Unreal5_8`. Zależności obejmują `Core`,
`CoreUObject`, `Engine`, `AIModule` i `GameplayTasks`.

## Kompilacja

Z katalogu projektu, w PowerShell:

```powershell
.\Tools\BuildEditor.ps1
```

Generowanie rozwiązania Visual Studio 2022:

```powershell
.\Tools\BuildEditor.ps1 -GenerateProjectFiles
```

Skrypt używa aliasów `P:` (projekt) i `U:` (silnik), ponieważ lokalny MSVC
zgłasza błąd C1083 przy zapisie PCH w oryginalnej ścieżce zawierającej `ó`.
Aliasy nie przenoszą ani nie kopiują plików. Są tymczasowe; po ponownym
logowaniu należy ponownie uruchomić skrypt przed użyciem wygenerowanego
rozwiązania. Skrypt nie zastępuje zajętych liter dysków.

Rozwiązanie służy do edycji kodu i IntelliSense. Przy obecnej ścieżce projektu
pełną kompilację uruchamiaj przez `Tools/BuildEditor.ps1`, aby używać aliasów
ASCII zamiast oryginalnych ścieżek zapisanych przez generator Visual Studio.
Ostrzeżenie generatora o .NET 10 dotyczy narzędzi automatyzacji C# w VS 2022;
kompilacja modułu C++ korzysta z .NET dostarczonego razem z silnikiem.

Kompilacja używa `-NoUBA`, aby ominąć zaobserwowany brak postępu z akceleratorem.
Przy zmianach struktury klas i modułów zamknij edytor przed pełną kompilacją.

Moduł jest gotowy na dodanie klas C++; nie zawiera jeszcze tasków ani dekoratorów.
