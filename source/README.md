# STUDIOUS KNIGHT

Pikselna igra za učenje, napisana u C++ (raylib). Igra je na bosanskom ili engleskom jeziku.
**Autor: M33RK4**

Mali vitez sa dugim mačem putuje kroz četiri carstva. Svaki neprijatelj je jedno
pitanje. Napiši tačan odgovor i vitez ga posiječe. Pogriješiš li, neprijatelj te
udari, ekran trepne crveno, izgubiš jedan život i igra ti pokaže tačan odgovor.

## Predmeti i nivoi

**Kreni u boj** i **Vježbalište** prvo pitaju za predmet (zasad **Računarske mreže**), a zatim za
jedan od 4 nivoa - poredani s lijeva na desno od najlakšeg do najtežeg:

| Nivo | Skripta | Fokus | Mapa | Neprijatelji |
|---|---|---|---|---|
| 1 | RM Esejska - 1P | 1. parcijala | Dolina stećaka | orci, boss: orkovski vojskovođa |
| 2 | Pitanja i odgovori RM integralno | integralni ispit | Krvava močvara | crveni orci (lovac, ork iz blata, vrač, puhač), boss: poglavica močvare |
| 3 | RM Esejska - 2P | 2. parcijala | Tamnica kostiju | kosturi (ratnik, strijelac, nekromant, vitez), boss: kralj kostiju |
| 4 | RAČUNARSKE MREŽE - kompletna skripta | cijeli kurs | Vulkansko carstvo | demoni (imp, plameni demon, paklenski pas, dželat), boss: kralj demona |

U boju se nivoi otključavaju redom: kad pobijediš nivo (pređeš ga do kraja), otvara se sljedeći.
Spremanje pamti koji su nivoi pređeni. Vježbalište je uvijek otvoreno za sve nivoe.

## Spremanja (save files)

Na početku biraš jednu od 3 tarot karte:
* **okrenuta poleđinom** = prazno spremanje -> upišeš ime viteza i karta se okrene,
* **okrenuta licem (vitez)** = spremanje na kojem se već igralo (ime, nivo, statistika).

Svako spremanje ima svoju statistiku, nivo i XP. `DEL` (ili `X`) briše izabrano spremanje.
Pri pravljenju novog spremanja biraš spol strelicama `← →` (vitez ili vitezica - vitezica ima krunu, dugu kosu,
haljinu i plašt u bojama svakog oklopa, a počinje u ružičastom), a `TAB` uključuje **hardcore** (trajna smrt). To se bira samo
tada i kasnije se ne može promijeniti: kad hardcore vitez padne u boju, karta postaje spomenik -
statistika se može pogledati, ali se sa tim vitezom više ne može igrati (samo obrisati).
Iz menija se uvijek možeš vratiti na izbor karata (**Promijeni spremanje**).
Spremanja su datoteke `StudiousKnight_save1.txt`, `_save2.txt`, `_save3.txt` pored igre.

## Kako igrati

| Ekran | Tipke |
|---|---|
| Izbor spremanja | `← →` + `ENTER`, `DEL` = obriši, `L` = jezik, može i miš |
| Meni | `↑ ↓` + `ENTER`, može i miš |
| Borba | piši odgovor, `ENTER` = udari, prazan `ENTER` = završni udarac, `ESC` = pauza |
| Vježbalište | `ENTER` = udari lutku, `F1` = pogledaj odgovor, `F2` = strogi režim, `TAB` / `Shift+TAB` = sljedeće / prethodno pitanje, `ESC` = meni |
| Pisanje odgovora | `← → ↑ ↓` = kursor (`Ctrl` = cijela riječ), `Home` / `End`, `Delete`, klik mišem, `Shift+ENTER` = novi red, točkić / `PgUp` / `PgDn` = pomjeranje kroz tekst |
| Svugdje | `F11` = cijeli ekran |

* **Kreni u boj**: svi neprijatelji izabranog nivoa redom (redoslijed je svaki put drugačiji).
  Imaš onoliko života koliko je 45% pitanja (10 pitanja -> 5 života). Neprijatelji napadaju
  na različite načine: skokom i udarcem, ubodom, čarolijom ili strijelom.
* **Rečenicu po rečenicu**: ork ima traku života. Možeš napisati cijeli odgovor odjednom, ili
  slati rečenicu po rečenicu - svaka rečenica rani orka za udio novih ključnih pojmova koje
  pogodi. Rečenica bez ijednog novog pojma košta jedan život. Kad si gotov, prazan `ENTER` je
  završni udarac: ork pada ako si pogodio dovoljno pojmova (`PRAG`), inače te udari i pokaže
  tačan odgovor. Pogodiš li sve pojmove, ork odmah pada.
* **Boss**: zadnjih 10% pitanja (bar jedno) je borba sa bossom nivoa - svako pitanje je jedna
  faza iste borbe.
* **XP po pitanju**: svaki nivo viteza košta 1000 XP. Savršeno pređeni nivoi 1, 2 i 3 daju tačno
  onoliko XP koliko treba do nivoa 150 (zadnji oklop); iznos po pitanju se sam računa iz broja
  pitanja u skriptama. Djelimičan odgovor daje dio XP. Spremanje pamti najbolji rezultat na svakom
  pitanju: ponovljeno pitanje daje samo XP za poboljšanje, a bez napretka 1 XP. Dev spremanje ne dobija XP.
* **Vježbalište**: učiš na lutki, bez XP i bez smrti. `F1` pokazuje odgovor. Ostaješ na istom
  pitanju koliko god želiš (`ENTER` = ponovo); na sljedeće ideš tek sa `TAB`. I ovdje možeš pisati
  rečenicu po rečenicu (pojmovi se sabiraju, prazan `ENTER` = gotovo) ili cijeli odgovor odjednom.
  **Strogi režim (`F2`)**: ne možeš dalje dok ne pogodiš **sve** ključne pojmove; poslije
  netačnog odgovora tvoj tekst ostaje pa ga samo dopuniš.
* **Oklopi**: novi izgled viteza otključava se na nivoima 10, 30, 60, 100 i 150
  (bronzani štitonoša, grimizna straža, vitez sjene, zlatni vitez, nebeski vitez).
  **Anđeoski vitez** (bijeli oklop, velika krila, oreol, lebdi iznad tla i umjesto mača gađa zrakom svjetlosti) otključava se kad
  stigneš do vulkanskog carstva, tj. kad pređeš tamnicu.
  U meniju **Oklopi** biraš `← →` i obučeš sa `ENTER`; izbor se čuva u spremanju.
* **Statistika**: vrijeme provedeno kao vitez, ubijeni neprijatelji, smrti, pobjede, nivo, XP i pređeni nivoi.
* **Dostignuća**: u meniju **Dostignuća** vidiš 9 dostignuća i napredak prema njima (pobijeđeni
  neprijatelji 30 / 50 / 100 / 200, savršen nivo sa 100% na svakom pitanju, poraženi svi bossovi,
  pronađene tajne). Kad otključaš novo, u uglu ekrana iskoči obavijest.
* **Tajne**: u meniju postoje dvije šifre - jedna pretvara dan u noć, a druga otključava tajni oklop.
* **Jezik**: bosanski / engleski (u meniju ili tipka `L`). Pitanja ostaju onakva kako su napisana u datotekama.

## Kako se ocjenjuje odgovor

Svako pitanje ima listu **ključnih pojmova**. Ako tvoj odgovor sadrži dovoljno pojmova
(`PRAG`), odgovor je tačan. Velika/mala slova se ne razlikuju, a kvačice nisu obavezne
(`cvor` = `čvor`, `dj` = `đ`). Poslije odgovora vidiš koje pojmove si pogodio (zeleno) a koje ne (crveno).

## Dodavanje pitanja

Pitanja su u folderu **`pitanja`** pored `.exe` datoteke - po jedna datoteka za svaki nivo:
`rm_1_esejska_1p.txt`, `rm_2_integralno.txt`, `rm_3_esejska_2p.txt`, `rm_4_kompletno.txt`.
Dodaj novo pitanje na kraj datoteke i sačuvaj kao UTF-8 - nema potrebe za ponovnim kompajliranjem.
Svako pitanje = jedan neprijatelj.

```
P: DNS?
K: Prevodi ime domene u IP adresu = prevo | preved | pretvar
K: DNS server = server
K: IP adresa = ip
PRAG: 2
O: DNS (Domain Name System) je servis koji ...
---
```

`P:` pitanje, `K:` ključni pojam (`naziv = alternativa | alternativa`), `PRAG:` koliko pojmova
treba, `O:` red tačnog odgovora, `---` kraj pitanja. Uputstvo je i na vrhu svake datoteke.

## Kompajliranje

Potrebno: CMake 3.16+ i C++17 kompajler. raylib 5.5 se preuzima automatski.

**Visual Studio 2022:** *File → Open → Folder...* izaberi ovaj folder, sačekaj CMake,
izaberi `StudiousKnight.exe` kao Startup Item i pokreni.

**Komandna linija (MinGW / Linux / macOS):**
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Struktura projekta

```
src/main.cpp        ekrani igre: izbor spremanja, meni, predmet i nivoi, borba, vježbalište, statistika
src/grafika.cpp     pikselna grafika: vitez, neprijatelji, lutka, 4 mape, vježbalište, tarot karte
src/neprijatelji_sprite.inl  sprite-ovi svih 21 neprijatelja (pikselne mreže i palete)
src/pitanja.cpp     učitavanje pitanja i ocjenjivanje odgovora
src/statistika.cpp  spremanja (ime, statistika, nivo, XP)
src/tekst.cpp       fontovi i UTF-8 tekst (č ć š ž đ)
src/jezik.hpp       prevod bosanski / engleski
src/zvuk.cpp        8-bitni zvučni efekti generisani u kodu
src/ulaz.cpp        tastatura/miš (+ skripte za automatsko testiranje)
pitanja/            pitanja i odgovori (po jedna datoteka za svaki nivo)
```

## Zasluge

* Fontovi: *Press Start 2P* (CodeMan38) i *VT323* (Peter Hull), SIL Open Font License, vidi `assets/fonts/`.
* Biblioteka: [raylib](https://www.raylib.com) (zlib licenca).

## Licenca / License

Igra je **besplatna za igranje**. Zabranjeno je prodavati je, naplaćivati pristup njoj, objavljivati je
drugdje ili je predstavljati kao svoju. Detalji su u `LICENSE`.

The game is **free to play**. You may not sell it, charge for it, re-upload it elsewhere or present it as
your own. See `LICENSE` for the full terms.
