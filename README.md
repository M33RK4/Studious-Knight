# Studious Knight

Mala pikselna igra koju sam napravio da bi učenje za ispite bilo zabavnije.
Igraš kao vitez koji putuje kroz četiri carstva, a svaki neprijatelj na putu je jedno pitanje.
Odgovoriš li tačno, vitez ga posiječe. Pogriješiš li, neprijatelj te udari, izgubiš život
i igra ti pokaže kako je trebalo odgovoriti.

Igra radi na bosanskom i engleskom jeziku.

**Autor:** M33RK4

## Kako pokrenuti

1. Preuzmi sve fajlove iz ovog repozitorija (zeleno dugme **Code**, pa **Download ZIP**).
2. Raspakuj ZIP u bilo koji folder.
3. Pokreni **`StudiousKnight.exe`**.

Folder `pitanja` mora ostati pored `.exe` fajla jer su u njemu sva pitanja.
Igra radi na Windowsu i ne treba ništa dodatno instalirati.

## Nivoi

Trenutno je tu predmet **Računarske mreže**, podijeljen na četiri nivoa. Idu od najlakšeg
prema najtežem, a svaki ima svoju mapu, svoje neprijatelje i svog bossa na kraju.

| Nivo | Gradivo | Mapa | Neprijatelji |
|---|---|---|---|
| 1 | Prva parcijala | Dolina stećaka | orci, a na kraju orkovski vojskovođa |
| 2 | Integralni ispit | Krvava močvara | močvarni orci, a na kraju poglavica močvare |
| 3 | Druga parcijala | Tamnica kostiju | kosturi, a na kraju kralj kostiju |
| 4 | Cijeli kurs | Vulkansko carstvo | demoni i paklenski pas, a na kraju kralj demona |

U boju se nivoi otključavaju jedan po jedan, tek kad pređeš prethodni.
U vježbalištu su svi nivoi otvoreni od početka.

## Kako se igra

Na početku biraš jednu od tri tarot karte. Prazna karta znači novo spremanje, pa upišeš ime
i izabereš viteza ili vitezicu. Tu možeš uključiti i **hardcore** način igre. Ako hardcore
vitez padne u boju, njegova karta postaje spomenik i s njim se više ne može igrati.

U meniju imaš dvije glavne stvari:

**Kreni u boj.** Neprijatelji dolaze jedan za drugim, a ti pišeš odgovore. Ne moraš sve napisati
odjednom. Možeš slati rečenicu po rečenicu i svaka rečenica rani neprijatelja onoliko koliko
novih ključnih pojmova sadrži. Kad misliš da si gotov, pritisni prazan `ENTER` za završni udarac.
Rečenica u kojoj nema ničeg novog košta te jedan život, pa pazi šta pišeš. Zadnjih nekoliko
pitanja svakog nivoa su borba sa bossom.

**Vježbalište.** Ovdje nema smrti ni XP bodova. Udaraš lutku, možeš zaviriti u odgovor sa `F1`
i ostaješ na istom pitanju dok sam ne odlučiš preći dalje sa `TAB`. Ako hoćeš strožije,
`F2` uključuje režim u kojem ne možeš dalje dok ne pogodiš baš sve pojmove.

### Kontrole

| Gdje | Šta |
|---|---|
| Meniji | strelice i `ENTER`, a može i miš |
| Boj | piši odgovor, `ENTER` udara, prazan `ENTER` je završni udarac, `ESC` pauza |
| Vježbalište | `F1` pokaži odgovor, `F2` strogi režim, `TAB` sljedeće pitanje, `Shift+TAB` prethodno |
| Pisanje | strelice pomjeraju kursor, `Shift+ENTER` novi red, točkić miša skrola tekst |
| Spremanja | `DEL` briše izabrano spremanje |
| Bilo gdje | `F11` cijeli ekran, `L` mijenja jezik |

### Kako se ocjenjuje odgovor

Svako pitanje ima nekoliko ključnih pojmova. Ako ih tvoj odgovor sadrži dovoljno, odgovor je
tačan. Nije bitno pišeš li velikim ili malim slovima, a ni kvačice nisu obavezne, tako da je
`cvor` isto što i `čvor`. Poslije svakog odgovora vidiš zelenim šta si pogodio, a crvenim šta ti je
promaklo.

## Napredak

**XP i nivoi.** Svaki tačan odgovor donosi 40 XP, a boss duplo. Djelimičan odgovor donosi dio
toga, zavisno od toga koliko si pojmova pogodio.

**Oklopi.** Kako napreduješ, otključavaš nove izglede viteza na nivoima 10, 30, 60, 100 i 150.
Kad pređeš tamnicu, dobiješ i **anđeoskog viteza** koji lebdi, ima velika krila i umjesto mača
gađa zrakom svjetlosti.

**Dostignuća.** Ima ih devet. Neka dobiješ za pobijeđene neprijatelje, jedno za nivo pređen bez
ijedne greške, jedno kad poraziš sve bossove, a neka su skrivena.

**Tajne.** U igri je sakriveno nekoliko iznenađenja. Neću ništa otkrivati, ali isplati se
pažljivo razgledati vježbalište i glavni meni.

## Svoja pitanja

Pitanja su obični tekstualni fajlovi u folderu `pitanja`, po jedan za svaki nivo. Možeš ih
mijenjati ili dodavati nova, samo ga sačuvaj u UTF8 formatu. Igra ih učita kad je sljedeći put pokreneš,
a svako novo pitanje postaje novi neprijatelj.

Jedno pitanje izgleda ovako:

```
P: DNS?
K: Prevodi ime domene u IP adresu = prevo | preved | pretvar
K: DNS server = server
K: IP adresa = ip
PRAG: 2
O: DNS (Domain Name System) je servis koji ...
---
```

`P` je pitanje, svaki `K` je jedan ključni pojam (iza znaka `=` idu riječi koje se priznaju),
`PRAG` kaže koliko pojmova treba za tačan odgovor, `O` je tačan odgovor, a `---` označava kraj pitanja.

## Zasluge

Fontovi su *Press Start 2P* (CodeMan38) i *VT323* (Peter Hull), oba pod SIL Open Font License.
Njihove licence su u folderu `licence`. Igra koristi biblioteku [raylib](https://www.raylib.com).

## Licenca

Igra je **besplatna za igranje**, ali je moje vlasništvo i sva prava su zadržana. Izvorni kod nije
javan. Igru ne smiješ prodavati, naplaćivati pristup njoj, objavljivati je na drugim mjestima,
rastavljati program niti je predstavljati kao svoju. Sve detalje nađeš u fajlu `LICENSE`.

*English:* the game is **free to play**, but it belongs to me and all rights are reserved.
The source code is not public. You may not sell it, charge for it, upload it anywhere else,
reverse engineer it or present it as your own. See `LICENSE` for the full terms.
