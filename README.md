# Studious Knight

A little pixel game I made so studying for exams would hurt a bit less. You play as a knight travelling through four realms, and every enemy you run into is an exam question. Answer it right and your knight cuts it down. Get it wrong and it hits back, you lose a life and the game shows you what the answer should have been.

You can play it in Bosnian or English.

**Just so you know:** for now the only subject in the game is Računarske mreže (Computer Networks), and the questions themselves are in Bosnian.

## Getting it running

1. Download the repo (the green **Code** button, then **Download ZIP**).
2. Unzip it wherever you want.
3. Open `StudiousKnight.exe`.

The `pitanja` folder has to stay right next to the .exe, that's where all the questions live. It runs on Windows and there's nothing else to install.

If Windows throws a blue "Windows protected your PC" screen at you, click **More info** and then **Run anyway**. It does that with every new program it hasn't seen before.

## The four levels

| Level | Material | Map | What you're fighting |
|---|---|---|---|
| 1 | First midterm | Valley of Tombstones | orcs, and the Orc Warlord at the end |
| 2 | Integral exam | Blood Swamp | swamp orcs, and the Bog Chieftain at the end |
| 3 | Second midterm | Dungeon of Bones | skeletons, and the Lich King at the end |
| 4 | The whole course | Volcanic Realm | demons and a hellhound, and the Demon King at the end |

In battle they unlock one at a time, you have to beat a level to open the next one. The training yard has all four open from the start.

## How it plays

First you pick one of three tarot cards. An empty card means a new save, so you type a name and choose a knight or a lady knight. This is also where you can turn on **hardcore**. If a hardcore knight dies, their card turns into a memorial and that save can't be played anymore.

**Battle.** Enemies come at you one after another and you type out your answers. You don't have to write it all in one go. Send it a sentence at a time and every sentence hurts the enemy depending on how many new key points it hits. When you're done, an empty ENTER is your final blow. Watch out though, a sentence with nothing new in it costs you a life. The last few questions of each level are a boss fight.

**Training yard.** No dying and no XP here. You beat up a training dummy, can peek at the answer with F1 and stay on the same question until you hit TAB. If you want it tougher, F2 won't let you move on until you get every single key point.

### Controls

| Where | What |
|---|---|
| Menus | arrow keys and ENTER, the mouse works too |
| Battle | type your answer, ENTER attacks, empty ENTER is the final blow, ESC pauses |
| Training yard | F1 shows the answer, F2 strict mode, TAB next question, Shift+TAB previous one |
| Typing | arrow keys move the cursor, Shift+ENTER new line, mouse wheel scrolls |
| Save files | DEL deletes the selected save |
| Anywhere | F11 fullscreen, L switches the language |

### How your answers get graded

Every question has a handful of key points. Hit enough of them and you got it right. Capital letters don't matter and neither do the marks on letters, so `cvor` counts as `čvor`. After every answer you see what you got in green and what you missed in red.

## Leveling up

**XP.** You get XP per question and every level costs the same amount. Beat the first three levels perfectly and you'll have exactly enough for every armor. A half right answer gets you part of the XP.

The game also remembers your best score on every question. Replaying a level only pays you for what you got better at, and a question you already know gives you just 1 XP. The idea is that you get rewarded for stuff you actually learned.

**Armor.** New looks for your knight unlock at levels 10, 30, 60, 100 and 150. Get through the dungeon and you also unlock the **Angelic Knight**, who floats, has huge wings and shoots beams of light instead of swinging a sword.

**Achievements.** There are nine. Some are for how many enemies you've beaten, one is for clearing a level without a single mistake, one is for taking down every boss and a couple are secret.

**Secrets.** There are a few surprises hidden around the game. Not telling, but poke around the main menu and the training yard.

## Adding your own questions

The questions are plain text files in the `pitanja` folder, one per level. You can change them or add new ones, just save the file as UTF8. The game loads them the next time you start it and every new question becomes a new enemy.

A question looks like this:

```
P: DNS?
K: Prevodi ime domene u IP adresu = prevo | preved | pretvar
K: DNS server = server
K: IP adresa = ip
PRAG: 2
O: DNS (Domain Name System) je servis koji ...
---
```

`P` is the question, every `K` is a key point (the words after `=` are the ones that count), `PRAG` is how many key points you need, `O` is the correct answer and `---` ends the question.

## Credits

The fonts are *Press Start 2P* by CodeMan38 and *VT323* by Peter Hull, both under the SIL Open Font License, and their licenses are in the `licence` folder. Built with [raylib](https://www.raylib.com). The pixel art, maps and sounds were all made for this game.

## License

It's free to play. Please don't sell it, charge for it, reupload it somewhere else or pass it off as your own. The full details are in `LICENSE`.

Good luck on your exams :)
