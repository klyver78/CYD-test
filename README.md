# CYD Bamse

En lille animeret bamse til ESP32-2432S028R CYD (2,8" 320 × 240 touchskærm).

- Bamsen blinker automatisk.
- Tryk på skærmen for at få den til at hilse med taleboble og mundanimation.
- Tryk på **DA / EN** øverst til højre for at skifte mellem dansk og engelsk. Valget kræver ingen app eller sprogpakke.

## Webinstaller

Installer firmwaren direkte fra browseren: **[Åbn CYD Bamse Web Installer](https://klyver78.github.io/CYD-test/)**.

Brug Chrome eller Edge på en computer, tilslut CYD’en med et USB-datakabel, og vælg dens serielle port, når browseren spørger. Installationen overskriver den firmware, der allerede ligger på enheden.

## Tale

Standard CYD-modellen har ingen højttaler. Derfor siger denne version “Hej!” / “Hello!” visuelt på skærmen og animerer munden; den afspiller ikke lyd. Der kræves ikke ekstra hardware.

## Kom i gang manuelt

Projektet bruger PlatformIO og bygger til standard CYD (ESP32-2432S028R). Åbn projektet i PlatformIO, og byg/upload miljøet `cyd` via USB. Der kræves ingen Wi-Fi eller konto. Efter upload vælges sprog direkte på CYD-touchskærmen.

Skærm-, touch- og pinopsætningen følger eksemplet [CYD-weather-display](https://github.com/martindamp/CYD-weather-display). Touchkalibrering er defineret øverst i `src/main.cpp`; enkelte CYD-varianter kan kræve justering.
